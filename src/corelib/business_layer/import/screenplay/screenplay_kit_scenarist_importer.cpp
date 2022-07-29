#include "screenplay_kit_scenarist_importer.h"

#include "screenlay_import_options.h"

#include <business_layer/import/text/simple_text_markdown_importer.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QDomDocument>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTextDocument>
#include <QVariantMap>
#include <QXmlStreamWriter>


namespace BusinessLayer {

namespace {

const QString kSqlDriver = "QSQLITE";
const QString kConnectionName = "import_database";

/**
 * @brief Типы документов из КИТа
 */
enum Type {
    Script,
    TitlePage,
    Synopsis,
    ResearchRoot,
    Folder,
    Text,
    Url,
    ImagesGallery,
    Image,
    MindMap,
    Logline,
    Versions,
    CharactersRoot = 100,
    Character,
    LocationsRoot = 200,
    Location
};

/**
 * @brief Получить простой текст из Html
 */
QString htmlToPlain(const QString& _html)
{
    QTextDocument document;
    document.setHtml(_html);
    return document.toPlainText().replace("\n", "\n\n");
}

/**
 * @brief Считать данные о персонаже
 */
QString readCharacter(const QString& _characterName, const QString& _kitCharacterXml)
{
    QDomDocument kitDocument;
    kitDocument.setContent(_kitCharacterXml);
    const auto kitCharacter = kitDocument.firstChildElement();

    QString characterXml;
    QXmlStreamWriter writer(&characterXml);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Character));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    auto writeTag = [&writer](const QString& _tag, const QString& _content) {
        writer.writeStartElement(_tag);
        writer.writeCDATA(_content);
        writer.writeEndElement();
    };
    writeTag("name", _characterName);
    const auto realName = kitCharacter.firstChildElement("real_name").text();
    if (!realName.isEmpty()) {
        writeTag("real_name", realName);
    }
    const auto description = htmlToPlain(kitCharacter.firstChildElement("description").text());
    if (!description.isEmpty()) {
        writeTag("long_description", description);
    }

    writer.writeEndDocument();

    return characterXml;
}

/**
 * @brief Считать данные о локации
 */
QString readLocation(const QString& _locationName, const QString& _kitLocationXml)
{
    QString locationXml;
    QXmlStreamWriter writer(&locationXml);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Location));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    auto writeTag = [&writer](const QString& _tag, const QString& _content) {
        writer.writeStartElement(_tag);
        writer.writeCDATA(_content);
        writer.writeEndElement();
    };
    writeTag("name", _locationName);
    const auto description = htmlToPlain(_kitLocationXml);
    if (!description.isEmpty()) {
        writeTag("long_description", description);
    }

    writer.writeEndDocument();

    return locationXml;
}

/**
 * @brief Сформировать простой текстовый документ из текстового документа КИТа
 */
QString readPlainTextDocument(const QString& _sourceDocument)
{
    const auto sourceDocumentPlainText = htmlToPlain(_sourceDocument);
    const auto document = SimpleTextMarkdownImporter().importDocument(sourceDocumentPlainText);
    return document.text;
}

/**
 * @brief Сформировать документ сценария из xml сценария КИТа
 */
ScreenplayAbstractImporter::Screenplay readScreenplay(const QString& _kitScreenplayXml)
{
    ScreenplayAbstractImporter::Screenplay screenplay;

    //
    // Читаем XML
    //
    QDomDocument kitDocument;
    kitDocument.setContent(_kitScreenplayXml);
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&screenplay.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // scenario - текст сценария
    //
    QDomElement rootElement = kitDocument.documentElement();
    QDomNode paragraph = rootElement.firstChild();
    bool alreadyInScene = false;
    while (!paragraph.isNull()) {
        //
        // Определим тип блока
        //
        const QString paragraphType = paragraph.nodeName();
        auto blockType = TextParagraphType::Action;
        if (paragraphType == "scene_heading") {
            blockType = TextParagraphType::SceneHeading;
        } else if (paragraphType == "scene_characters") {
            blockType = TextParagraphType::SceneCharacters;
        } else if (paragraphType == "action") {
            blockType = TextParagraphType::Action;
        } else if (paragraphType == "character") {
            blockType = TextParagraphType::Character;
        } else if (paragraphType == "parenthetical") {
            blockType = TextParagraphType::Parenthetical;
        } else if (paragraphType == "dialog") {
            blockType = TextParagraphType::Dialogue;
        } else if (paragraphType == "transition") {
            blockType = TextParagraphType::Transition;
        } else if (paragraphType == "note") {
            blockType = TextParagraphType::Shot;
        } else if (paragraphType == "noprintable_text") {
            blockType = TextParagraphType::InlineNote;
        } else if (paragraphType == "folder_header") {
            blockType = TextParagraphType::SequenceHeading;
        } else if (paragraphType == "folder_footer") {
            blockType = TextParagraphType::SequenceFooter;
        } else if (paragraphType == "scene_description") {
            //
            // TODO:
            //
            paragraph = paragraph.nextSibling();
            continue;
        } else if (paragraphType == "lyrics") {
            blockType = TextParagraphType::Lyrics;
        }

        //
        // Получим текст блока
        //
        QString paragraphText;
        QVector<ScreenplayTextModelTextItem::ReviewMark> reviewMarks;
        QVector<ScreenplayTextModelTextItem::TextFormat> formats;
        {
            QDomElement textNode = paragraph.firstChildElement("v");
            if (!textNode.isNull()) {
                //
                // ... читаем текст
                //
                paragraphText = textNode.text();
                //
                // ... читаем редакторские заметки
                //
                auto reviewsNode = paragraph.firstChildElement("reviews");
                if (!reviewsNode.isNull()) {
                    auto reviewNode = reviewsNode.firstChildElement("review");
                    while (!reviewNode.isNull()) {
                        ScreenplayTextModelTextItem::ReviewMark reviewMark;
                        reviewMark.from = reviewNode.attribute("from").toInt();
                        reviewMark.length = reviewNode.attribute("length").toInt();
                        if (reviewNode.attribute("color") != "#000000") {
                            reviewMark.textColor = reviewNode.attribute("color");
                        }
                        if (reviewNode.attribute("bgcolor") != "#000000") {
                            reviewMark.backgroundColor = reviewNode.attribute("bgcolor");
                        }
                        reviewMark.isDone = reviewNode.attribute("done") == "true";
                        auto reviewCommentNode = reviewNode.firstChildElement("review_comment");
                        while (!reviewCommentNode.isNull()) {
                            reviewMark.comments.append(
                                { reviewCommentNode.attribute("author"),
                                  {},
                                  reviewCommentNode.attribute("date"),
                                  TextHelper::fromHtmlEscaped(
                                      reviewCommentNode.attribute("comment")) });

                            reviewCommentNode
                                = reviewCommentNode.nextSiblingElement("review_comment");
                        }
                        if (reviewMark.comments.isEmpty()) {
                            reviewMark.comments.append(
                                ScreenplayTextModelTextItem::ReviewComment());
                        }
                        reviewMarks.append(reviewMark);

                        reviewNode = reviewNode.nextSiblingElement("review");
                    }
                }
                //
                // ... читаем форматирование
                //
                auto formatsNode = paragraph.firstChildElement("formatting");
                if (!formatsNode.isNull()) {
                    auto formatNode = formatsNode.firstChildElement("format");
                    while (!formatNode.isNull()) {
                        ScreenplayTextModelTextItem::TextFormat format;
                        format.from = formatNode.attribute("from").toInt();
                        format.length = formatNode.attribute("length").toInt();
                        format.isBold = formatNode.attribute("bold") == "true";
                        format.isItalic = formatNode.attribute("italic") == "true";
                        format.isUnderline = formatNode.attribute("underline") == "true";
                        formats.append(format);

                        formatNode = formatNode.nextSiblingElement("format");
                    }
                }
            }

            //
            // Корректируем при необходимости
            //
            if (blockType == TextParagraphType::Parenthetical) {
                if (!paragraphText.isEmpty() && paragraphText.front() == '(') {
                    paragraphText.remove(0, 1);
                }
                if (!paragraphText.isEmpty() && paragraphText.back() == ')') {
                    paragraphText.chop(1);
                }
            }
        }

        //
        // Формируем блок сценария
        //
        switch (blockType) {
        case TextParagraphType::SequenceHeading: {
            if (alreadyInScene) {
                writer.writeEndElement(); // контент предыдущей сцены
                writer.writeEndElement(); // предыдущая сцена
            }

            alreadyInScene = false; // вышли из сцены

            writer.writeStartElement(toString(TextFolderType::Sequence));
            writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
            writer.writeStartElement(xml::kContentTag);
            break;
        }

        case TextParagraphType::SequenceFooter: {
            if (alreadyInScene) {
                writer.writeEndElement(); // контент предыдущей сцены
                writer.writeEndElement(); // предыдущая сцена
            }

            alreadyInScene = false; // вышли из сцены

            writer.writeEndElement(); // контент текущей папки
            writer.writeEndElement(); // текущая папка
            break;
        }

        case TextParagraphType::SceneHeading: {
            if (alreadyInScene) {
                writer.writeEndElement(); // контент предыдущей сцены
                writer.writeEndElement(); // предыдущая сцена
            }

            alreadyInScene = true; // вошли в новую сцену

            writer.writeStartElement(toString(TextGroupType::Scene));
            writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
            writer.writeStartElement(xml::kContentTag);
            break;
        }

        default:
            break;
        }
        writer.writeStartElement(toString(blockType));
        writer.writeStartElement(xml::kValueTag);
        writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
        writer.writeEndElement(); // value
        //
        // Пишем редакторские заметки
        //
        if (!reviewMarks.isEmpty()) {
            writer.writeStartElement(xml::kReviewMarksTag);
            for (const auto& reviewMark : std::as_const(reviewMarks)) {
                writer.writeStartElement(xml::kReviewMarkTag);
                writer.writeAttribute(xml::kFromAttribute, QString::number(reviewMark.from));
                writer.writeAttribute(xml::kLengthAttribute, QString::number(reviewMark.length));
                writer.writeAttribute(xml::kBackgroundColorAttribute,
                                      reviewMark.backgroundColor.name());
                for (const auto& comment : std::as_const(reviewMark.comments)) {
                    writer.writeStartElement(xml::kCommentTag);
                    writer.writeAttribute(xml::kAuthorAttribute, comment.author);
                    writer.writeAttribute(xml::kDateAttribute, comment.date);
                    writer.writeCDATA(TextHelper::toHtmlEscaped(comment.text));
                    writer.writeEndElement(); // comment
                }
                writer.writeEndElement();
            }
            writer.writeEndElement(); // review marks
        }
        //
        // Пишем форматирование
        //
        if (!formats.isEmpty()) {
            writer.writeStartElement(xml::kFormatsTag);
            for (const auto& format : std::as_const(formats)) {
                writer.writeStartElement(xml::kFormatTag);
                //
                // Данные пользовательского форматирования
                //
                writer.writeAttribute(xml::kFromAttribute, QString::number(format.from));
                writer.writeAttribute(xml::kLengthAttribute, QString::number(format.length));
                if (format.isBold) {
                    writer.writeAttribute(xml::kBoldAttribute, "true");
                }
                if (format.isItalic) {
                    writer.writeAttribute(xml::kItalicAttribute, "true");
                }
                if (format.isUnderline) {
                    writer.writeAttribute(xml::kUnderlineAttribute, "true");
                }
                //
                writer.writeEndElement(); // format
            }
            writer.writeEndElement(); // formats
        }
        writer.writeEndElement(); // block type

        //
        // Переходим к следующему
        //
        paragraph = paragraph.nextSibling();
    }
    writer.writeEndDocument();

    return screenplay;
}

} // namespace

ScreenplayAbstractImporter::Documents ScreenplayKitScenaristImporter::importDocuments(
    const ScreenplayImportOptions& _options) const
{
    Documents result;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(kSqlDriver, kConnectionName);
        database.setDatabaseName(_options.filePath);
        if (database.open()) {
            //
            // Загрузим данные о прсонажах
            //
            if (_options.importCharacters) {
                QSqlQuery charactersQuery(database);
                charactersQuery.prepare(
                    "SELECT name, description FROM research WHERE type = ? ORDER by sort_order");
                charactersQuery.addBindValue(Character);
                charactersQuery.exec();
                while (charactersQuery.next()) {
                    const auto name = charactersQuery.value("name").toString();
                    const auto content
                        = readCharacter(name, charactersQuery.value("description").toString());
                    result.characters.append({ name, content });
                }
            }

            //
            // Загрузим данные о локациях
            //
            if (_options.importLocations) {
                QSqlQuery locationsQuery(database);
                locationsQuery.prepare("SELECT * FROM research WHERE type = ? ORDER by sort_order");
                locationsQuery.addBindValue(Location);
                locationsQuery.exec();
                while (locationsQuery.next()) {
                    const auto name = locationsQuery.value("name").toString();
                    const auto content
                        = readLocation(name, locationsQuery.value("description").toString());
                    result.locations.append({ name, content });
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(kConnectionName);

    return result;
}

QVector<ScreenplayAbstractImporter::Screenplay> ScreenplayKitScenaristImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    QVector<Screenplay> result;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(kSqlDriver, kConnectionName);
        database.setDatabaseName(_options.filePath);
        if (database.open()) {
            QSqlQuery query(database);

            //
            // Читаем сценарий
            //
            QString screenplayName = QFileInfo(_options.filePath).completeBaseName();
            {
                query.exec("SELECT text FROM scenario WHERE is_draft = 0");
                query.next();
                const auto kitScreenplayXml = query.record().value("text").toString();
                auto screenplay = readScreenplay(kitScreenplayXml);

                //
                // Читаем данные
                //
                query.exec("SELECT data_name, data_value FROM scenario_data");
                while (query.next()) {
                    const auto dataName = query.record().value("data_name").toString();
                    const auto dataValue = query.record().value("data_value").toString();
                    if (dataValue.isNull() || dataValue.isEmpty()) {
                        continue;
                    }

                    if (dataName == "name") {
                        screenplayName = dataValue;
                        screenplay.name = dataValue;
                    } else if (dataName == "header") {
                        screenplay.header = dataValue;
                    } else if (dataName == "footer") {
                        screenplay.footer = dataValue;
                    } else if (dataName == "logline") {
                        screenplay.logline = htmlToPlain(dataValue);
                    } else if (dataName == "synopsis") {
                        screenplay.synopsis = readPlainTextDocument(dataValue);
                    }
                }

                result.append(screenplay);
            }

            //
            // Читаем черновик
            //
            {
                query.exec("SELECT text FROM scenario WHERE is_draft = 1");
                query.next();
                const auto kitScreenplayXml = query.record().value("text").toString();
                const auto defaultKitScreenplay
                    = "<?xml version=\"1.0\"?>\n"
                      "<scenario version=\"1.0\">\n"
                      "<scene_heading uuid=\"{000000-0000000-000000}\">\n"
                      "<v><![CDATA[]]></v>\n"
                      "</scene_heading>\n"
                      "</scenario>\n";
                if (kitScreenplayXml != defaultKitScreenplay) {
                    auto screenplay = readScreenplay(kitScreenplayXml);
                    screenplay.name = QString("%1 (%2)").arg(
                        screenplayName,
                        //: Draft screenplay imported from KIT Scenarist file
                        QApplication::translate("BusinessLayer::KitScenaristImporter", "draft"));
                    result.append(screenplay);
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(kConnectionName);

    return result;
}

} // namespace BusinessLayer

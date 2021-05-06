#include "kit_scenarist_importer.h"

#include "screenlay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>

#include <domain/document_object.h>

#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QDomDocument>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariantMap>
#include <QXmlStreamWriter>


namespace BusinessLayer
{

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
 * @brief Сформировать документ сценария из xml сценария КИТа
 */
AbstractScreenplayImporter::Screenplay readScreenplay(const QString& _kitScreenplayXml) {
    AbstractScreenplayImporter::Screenplay screenplay;

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
    writer.writeAttribute(xml::kMimeTypeAttribute, Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
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
        auto blockType = ScreenplayParagraphType::Action;
        if (paragraphType == "scene_heading") {
            blockType = ScreenplayParagraphType::SceneHeading;
        } else if (paragraphType == "scene_characters") {
            blockType = ScreenplayParagraphType::SceneCharacters;
        } else if (paragraphType == "action") {
            blockType = ScreenplayParagraphType::Action;
        } else if (paragraphType == "character") {
            blockType = ScreenplayParagraphType::Character;
        } else if (paragraphType == "parenthetical") {
            blockType = ScreenplayParagraphType::Parenthetical;
        } else if (paragraphType == "dialog") {
            blockType = ScreenplayParagraphType::Dialogue;
        } else if (paragraphType == "transition") {
            blockType = ScreenplayParagraphType::Transition;
        } else if (paragraphType == "note") {
            blockType = ScreenplayParagraphType::Shot;
        } else if (paragraphType == "noprintable_text") {
            blockType = ScreenplayParagraphType::InlineNote;
        } else if (paragraphType == "folder_header") {
            blockType = ScreenplayParagraphType::FolderHeader;
        } else if (paragraphType == "folder_footer") {
            blockType = ScreenplayParagraphType::FolderFooter;
        } else if (paragraphType == "scene_description") {
            //
            // TODO:
            //
            paragraph = paragraph.nextSibling();
            continue;
        } else if (paragraphType == "lyrics") {
            blockType = ScreenplayParagraphType::Lyrics;
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
                            reviewMark.comments.append({ reviewCommentNode.attribute("author"),
                                                         reviewCommentNode.attribute("date"),
                                                         TextHelper::toHtmlEscaped(reviewCommentNode.attribute("comment")) });

                            reviewCommentNode = reviewCommentNode.nextSiblingElement("review_comment");
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
            if (blockType == ScreenplayParagraphType::Parenthetical) {
                if (!paragraphText.isEmpty()
                    && paragraphText.front() == "(") {
                    paragraphText.remove(0, 1);
                }
                if (!paragraphText.isEmpty()
                    && paragraphText.back() == ")") {
                    paragraphText.chop(1);
                }
            }
        }

        //
        // Формируем блок сценария
        //
        switch (blockType) {
            case ScreenplayParagraphType::FolderHeader: {
                if (alreadyInScene) {
                    writer.writeEndElement(); // контент предыдущей сцены
                    writer.writeEndElement(); // предыдущая сцена
                }

                alreadyInScene = false; // вышли из сцены

                writer.writeStartElement(xml::kFolderTag);
                writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
                writer.writeStartElement(xml::kContentTag);
                break;
            }

            case ScreenplayParagraphType::FolderFooter: {
                if (alreadyInScene) {
                    writer.writeEndElement(); // контент предыдущей сцены
                    writer.writeEndElement(); // предыдущая сцена
                }

                alreadyInScene = false; // вышли из сцены

                writer.writeEndElement(); // контент текущей папки
                writer.writeEndElement(); // текущая папка
                break;
            }

            case ScreenplayParagraphType::SceneHeading: {
                if (alreadyInScene) {
                    writer.writeEndElement(); // контент предыдущей сцены
                    writer.writeEndElement(); // предыдущая сцена
                }

                alreadyInScene = true; // вошли в новую сцену

                writer.writeStartElement(xml::kSceneTag);
                writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
                writer.writeStartElement(xml::kContentTag);
                break;
            }

            default: break;
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
                writer.writeAttribute(xml::kBackgroundColorAttribute, reviewMark.backgroundColor.name());
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
            for (const auto& format : std::as_const(formats)){
                writer.writeStartElement(xml::kFormatTag);
                //
                // Данные пользовательского форматирования
                //
                writer.writeAttribute(xml::kFromAttribute, QString::number(format.from));
                writer.writeAttribute(xml::kLengthAttribute, QString::number(format.length));
                writer.writeAttribute(xml::kBoldAttribute, format.isBold ? "true" : "false");
                writer.writeAttribute(xml::kItalicAttribute, format.isItalic ? "true" : "false");
                writer.writeAttribute(xml::kUnderlineAttribute, format.isUnderline ? "true" : "false");
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

AbstractScreenplayImporter::Documents KitScenaristImporter::importDocuments(const ScreenplayImportOptions& _options) const
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
                charactersQuery.prepare("SELECT * FROM research WHERE type = ? ORDER by sort_order");
                charactersQuery.addBindValue(Character);
                charactersQuery.exec();
                while (charactersQuery.next()) {
                    const auto name = charactersQuery.value("name").toString();
                    result.characters.append({name, {}});
                }
            }

            //
            // Загрузим данные о локациях
            //
            if (_options.importLocations) {
                QSqlQuery charactersQuery(database);
                charactersQuery.prepare("SELECT * FROM research WHERE type = ? ORDER by sort_order");
                charactersQuery.addBindValue(Location);
                charactersQuery.exec();
                while (charactersQuery.next()) {
                    const auto name = charactersQuery.value("name").toString();
                    result.locations.append({name, {}});
                }
            }
        }
    }
    QSqlDatabase::removeDatabase(kConnectionName);

    return result;
}

QVector<AbstractScreenplayImporter::Screenplay> KitScenaristImporter::importScreenplays(const ScreenplayImportOptions& _options) const
{
    QVector<Screenplay> result;

    {
        QSqlDatabase database = QSqlDatabase::addDatabase(kSqlDriver, kConnectionName);
        database.setDatabaseName(_options.filePath);
        if (database.open()) {
            QSqlQuery query(database);

            //
            // Читаем название
            //
            QString name = QFileInfo(_options.filePath).completeBaseName();
            {
                query.exec("SELECT data_value FROM scenario_data WHERE data_name = 'name'");
                query.next();
                const auto screenplayName = query.record().value("data_value").toString();
                if (!screenplayName.isEmpty()) {
                    name = screenplayName;
                }
            }

            //
            // Читаем сценарий
            //
            {
                query.exec("SELECT text FROM scenario WHERE is_draft = 0");
                query.next();
                const auto kitScreenplayXml = query.record().value("text").toString();
                auto screenplay = readScreenplay(kitScreenplayXml);
                screenplay.name = name;
                result.append(screenplay);
            }

            //
            // Читаем черновик
            //
            {
                query.exec("SELECT text FROM scenario WHERE is_draft = 1");
                query.next();
                const auto kitScreenplayXml = query.record().value("text").toString();
                const auto defaultKitScreenplay = "<?xml version=\"1.0\"?>\n"
                                                  "<scenario version=\"1.0\">\n"
                                                  "<scene_heading uuid=\"{000000-0000000-000000}\">\n"
                                                  "<v><![CDATA[]]></v>\n"
                                                  "</scene_heading>\n"
                                                  "</scenario>\n";
                if (kitScreenplayXml != defaultKitScreenplay) {
                    auto screenplay = readScreenplay(kitScreenplayXml);
                    screenplay.name = QString("%1 (%2)").arg(name,
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

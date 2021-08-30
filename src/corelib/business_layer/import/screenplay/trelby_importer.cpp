#include "trelby_importer.h"

#include "screenlay_import_options.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>
#include <domain/document_object.h>

#include <QFile>
#include <QFileInfo>
#include <QXmlStreamWriter>

#include <set>


namespace BusinessLayer {

AbstractScreenplayImporter::Documents TrelbyImporter::importDocuments(
    const ScreenplayImportOptions& _options) const
{
    //
    // Открываем файл
    //
    QFile trelbyFile(_options.filePath);
    if (!trelbyFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Читаем plain text
    //
    const QStringList paragraphs = QString(trelbyFile.readAll()).split("\n");
    QString paragraphText;
    const QStringList blockChecker = { ">", "&", "|", "." };
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    for (const QString& paragraph : paragraphs) {
        const QString paragraphType = paragraph.left(2);

        //
        // Если строка пуста, или не является текстовым блоком, пропускаем её
        //
        if (paragraphType.isEmpty() || !blockChecker.contains(paragraphType[0])) {
            continue;
        }

        //
        // Определим тип блока
        //
        auto blockType = ScreenplayParagraphType::Undefined;
        if (paragraphType.endsWith("\\")) {
            blockType = ScreenplayParagraphType::SceneHeading;
        } else if (paragraphType.endsWith("_")) {
            blockType = ScreenplayParagraphType::Character;
        }

        //
        // Получим текст блока
        //
        if (!paragraphText.isEmpty()) {
            paragraphText += " ";
        }
        paragraphText += paragraph.mid(2);

        //
        // Если дошли до последней строки блока
        //
        if (paragraphType.startsWith(".")) {
            switch (blockType) {
            case ScreenplayParagraphType::SceneHeading: {
                if (!_options.importLocations) {
                    break;
                }

                const auto locationName = ScreenplaySceneHeadingParser::location(paragraphText);
                if (locationName.isEmpty()) {
                    break;
                }

                locationNames.emplace(locationName);
                break;
            }

            case ScreenplayParagraphType::Character: {
                if (!_options.importCharacters) {
                    break;
                }

                const auto characterName = ScreenplayCharacterParser::name(paragraphText);
                if (characterName.isEmpty()) {
                    break;
                }

                characterNames.emplace(characterName);
                break;
            }

            default:
                break;
            }

            //
            // И очищаем текст
            //
            paragraphText.clear();
        }
    }

    Documents documents;
    for (const auto& characterName : characterNames) {
        documents.characters.append({ characterName, {} });
    }
    for (const auto& locationName : locationNames) {
        documents.locations.append({ locationName, {} });
    }
    return documents;
}

QVector<AbstractScreenplayImporter::Screenplay> TrelbyImporter::importScreenplays(
    const ScreenplayImportOptions& _options) const
{
    if (_options.importScreenplay == false) {
        return {};
    }

    Screenplay result;
    result.name = QFileInfo(_options.filePath).completeBaseName();

    //
    // Открываем файл
    //
    QFile trelbyFile(_options.filePath);
    if (!trelbyFile.open(QIODevice::ReadOnly)) {
        return { result };
    }

    //
    // Читаем plain text
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&result.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Текст сценария
    //
    const QStringList paragraphs = QString(trelbyFile.readAll()).split("\n");
    QString paragraphText;
    const QStringList blockChecker = { ">", "&", "|", "." };
    bool alreadyInScene = false;
    for (const QString& paragraph : paragraphs) {
        const QString paragraphType = paragraph.left(2);

        //
        // Если строка пуста, или не является текстовым блоком, пропускаем её
        //
        if (paragraphType.isEmpty() || !blockChecker.contains(paragraphType[0])) {
            continue;
        }

        //
        // Определим тип блока
        //
        auto blockType = ScreenplayParagraphType::Action;
        if (paragraphType.endsWith("\\")) {
            blockType = ScreenplayParagraphType::SceneHeading;
        } else if (paragraphType.endsWith(".")) {
            blockType = ScreenplayParagraphType::Action;
        } else if (paragraphType.endsWith("_")) {
            blockType = ScreenplayParagraphType::Character;
        } else if (paragraphType.endsWith("(")) {
            blockType = ScreenplayParagraphType::Parenthetical;
        } else if (paragraphType.endsWith(":")) {
            blockType = ScreenplayParagraphType::Dialogue;
        } else if (paragraphType.endsWith("/")) {
            blockType = ScreenplayParagraphType::Transition;
        } else if (paragraphType.endsWith("=")) {
            blockType = ScreenplayParagraphType::Shot;
        } else if (paragraphType.endsWith("%")) {
            blockType = ScreenplayParagraphType::InlineNote;
        }

        //
        // Получим текст блока
        //
        if (!paragraphText.isEmpty()) {
            paragraphText += " ";
        }
        paragraphText += paragraph.mid(2);

        //
        // Корректируем при необходимости
        //
        if (blockType == ScreenplayParagraphType::Parenthetical) {
            if (!paragraphText.isEmpty() && paragraphText.front() == "(") {
                paragraphText.remove(0, 1);
            }
            if (!paragraphText.isEmpty() && paragraphText.back() == ")") {
                paragraphText.chop(1);
            }
        }

        //
        // Если дошли до последней строки блока
        //
        if (paragraphType.startsWith(".")) {
            //
            // Формируем блок сценария
            //
            if (blockType == ScreenplayParagraphType::SceneHeading) {
                if (alreadyInScene) {
                    writer.writeEndElement(); // контент предыдущей сцены
                    writer.writeEndElement(); // предыдущая сцена
                }
                alreadyInScene = true;

                writer.writeStartElement(xml::kSceneTag);
                writer.writeAttribute(xml::kUuidAttribute, QUuid::createUuid().toString());
                writer.writeStartElement(xml::kContentTag);
            }
            writer.writeStartElement(toString(blockType));
            writer.writeStartElement(xml::kValueTag);
            writer.writeCDATA(paragraphText);
            writer.writeEndElement(); // value
            writer.writeEndElement(); // block type

            //
            // И очищаем текст
            //
            paragraphText.clear();
        }
    }
    writer.writeEndDocument();

    return { result };
}

} // namespace BusinessLayer

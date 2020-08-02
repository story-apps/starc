#include "celtx_importer.h"

#include "import_options.h"

#include <qgumbodocument.h>
#include <qgumbonode.h>

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_xml.h>
#include <business_layer/templates/screenplay_template.h>

#include <domain/document_object.h>

#include <qtzip/QtZipReader>

#include <QFileInfo>
#include <QXmlStreamWriter>

#include <set>


namespace BusinessLayer
{

namespace {

/**
 * @brief Считать текст файла-сценария из архива celtx-файла
 */
QString readScript(const QString& _filePath) {
    //
    // Открыть архив и извлечь содержимое файла, название которого начинается со "script"
    //

    QFile celtxFile(_filePath);
    if (!celtxFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    QtZipReader zip(&celtxFile);
    if (!zip.isReadable()) {
        return {};
    }

    QString scriptHtml;
    for (const QString& fileName : zip.fileList()) {
        if (fileName.startsWith("script")) {
            scriptHtml = zip.fileData(fileName);
            break;
        }
    }

    return scriptHtml;
}

}

AbstractImporter::Documents CeltxImporter::importDocuments(const ImportOptions& _options) const
{
    //
    // Открываем файл
    //
    const auto scriptHtml = readScript(_options.filePath);
    if (scriptHtml.isEmpty()) {
        return {};
    }

    //
    // Читаем html text
    //
    std::set<QString> characterNames;
    std::set<QString> locationNames;
    const auto scriptDocument = QGumboDocument::parse(scriptHtml.toUtf8());
    const auto rootNode = scriptDocument.rootNode();
    const auto pNodes = rootNode.getElementsByTagName(HtmlTag::P);
    bool alreadyInScene = false;
    for (const auto& pNode : pNodes) {
        const QString pNodeClass = pNode.getAttribute("class");
        auto blockType = ScreenplayParagraphType::Undefined;
        if (pNodeClass == "sceneheading") {
            blockType = ScreenplayParagraphType::SceneHeading;
        } else if (pNodeClass == "character") {
            blockType = ScreenplayParagraphType::Character;
        }

        //
        // Сформируем текст
        //
        QString paragraphText = pNode.innerText().simplified();

        switch (blockType) {
            case ScreenplayParagraphType::SceneHeading: {
                if (!_options.importLocations) {
                    break;
                }

                const auto locationName = SceneHeadingParser::location(paragraphText);
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

                const auto characterName = CharacterParser::name(paragraphText);
                if (characterName.isEmpty()) {
                    break;
                }

                characterNames.emplace(characterName);
                break;
            }

            default: break;
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

QVector<AbstractImporter::Screenplay> CeltxImporter::importScreenplays(const ImportOptions& _options) const
{
    if (_options.importScreenplay == false) {
        return {};
    }

    Screenplay result;
    result.name = QFileInfo(_options.filePath).completeBaseName();

    //
    // Открываем файл
    //
    const auto scriptHtml = readScript(_options.filePath);
    if (scriptHtml.isEmpty()) {
        return { result };
    }

    //
    // Читаем html text
    //
    // ... и пишем в сценарий
    //
    QXmlStreamWriter writer(&result.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute, Domain::mimeTypeFor(Domain::DocumentObjectType::ScreenplayText));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    //
    // Получим список строк текста
    //
    const auto scriptDocument = QGumboDocument::parse(scriptHtml.toUtf8());
    const auto rootNode = scriptDocument.rootNode();
    const auto pNodes = rootNode.getElementsByTagName(HtmlTag::P);
    bool alreadyInScene = false;
    for (const auto& pNode : pNodes) {
        const QString pNodeClass = pNode.getAttribute("class");
        auto blockType = ScreenplayParagraphType::UnformattedText;
        if (pNodeClass == "sceneheading") {
            blockType = ScreenplayParagraphType::SceneHeading;
        } else if (pNodeClass == "action") {
            blockType = ScreenplayParagraphType::Action;
        } else if (pNodeClass == "character") {
            blockType = ScreenplayParagraphType::Character;
        } else if (pNodeClass == "parenthetical") {
            blockType = ScreenplayParagraphType::Parenthetical;
        } else if (pNodeClass == "dialog") {
            blockType = ScreenplayParagraphType::Dialogue;
        } else if (pNodeClass == "transition") {
            blockType = ScreenplayParagraphType::Transition;
        } else if (pNodeClass == "shot") {
            blockType = ScreenplayParagraphType::Shot;
        }

        //
        // Сформируем текст
        //
        QString paragraphText = pNode.innerText().simplified();

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
    }
    writer.writeEndDocument();

    return { result };
}

} // namespace BusinessLayer

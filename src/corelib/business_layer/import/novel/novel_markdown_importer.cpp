#include "novel_markdown_importer.h"

#include "novel_import_options.h"

#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/simple_text_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QRegularExpression>
#include <QXmlStreamWriter>

namespace BusinessLayer {

NovelAbstractImporter::Document NovelMarkdownImporter::importNovels(
    const NovelImportOptions& _options) const
{
    //
    // Открываем файл
    //
    QFile textFile(_options.filePath);
    if (!textFile.open(QIODevice::ReadOnly)) {
        return {};
    }

    //
    // Импортируем
    //
    Document textDocument = importNovel(textFile.readAll());
    if (textDocument.name.isEmpty()) {
        textDocument.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return textDocument;
}

NovelAbstractImporter::Document NovelMarkdownImporter::importNovel(const QString& _text) const
{
    if (_text.simplified().isEmpty()) {
        return {};
    }

    Document textDocument;

    //
    // Читаем plain text
    //
    // ... и пишем в документ
    //
    QXmlStreamWriter writer(&textDocument.text);
    writer.writeStartDocument();
    writer.writeStartElement(xml::kDocumentTag);
    writer.writeAttribute(xml::kMimeTypeAttribute,
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Novel));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    const QStringList paragraphs = QString(_text).remove('\r').split('\n');
    for (const auto& paragraph : paragraphs) {
        if (paragraph.simplified().isEmpty()) {
            continue;
        }

        if (paragraph.startsWith("#")) {
            auto paragraphText = paragraph;
            paragraphText.remove(QRegularExpression("^#{1,}\\s{0,}"));

            writer.writeStartElement(toString(TextParagraphType::SceneHeading));
            writer.writeStartElement(xml::kValueTag);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraphText));
            writer.writeEndElement();
            writer.writeEndElement();
        } else {
            writer.writeStartElement(toString(TextParagraphType::Text));
            writer.writeStartElement(xml::kValueTag);
            writer.writeCDATA(TextHelper::toHtmlEscaped(paragraph));
            writer.writeEndElement();
            writer.writeEndElement();
        }
    }
    writer.writeEndDocument();

    return textDocument;
}

} // namespace BusinessLayer

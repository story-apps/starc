#include "markdown_improter.h"

#include "text_import_options.h"

#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/text_template.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QXmlStreamWriter>

namespace BusinessLayer {

AbstractTextImporter::Document MarkdownImporter::importDocument(
    const TextImportOptions& _options) const
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
    Document textDocument = importDocument(textFile.readAll());
    if (textDocument.name.isEmpty()) {
        textDocument.name = QFileInfo(_options.filePath).completeBaseName();
    }

    return textDocument;
}

AbstractTextImporter::Document MarkdownImporter::importDocument(const QString& _text) const
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
                          Domain::mimeTypeFor(Domain::DocumentObjectType::Text));
    writer.writeAttribute(xml::kVersionAttribute, "1.0");

    const QStringList paragraphs = QString(_text).split("\n");
    for (const auto& paragraph : paragraphs) {
        writer.writeStartElement(toString(TextParagraphType::Text));
        writer.writeStartElement(xml::kValueTag);
        writer.writeCDATA(TextHelper::toHtmlEscaped(paragraph));
        writer.writeEndElement();
        writer.writeEndElement();
    }
    writer.writeEndDocument();

    return textDocument;
}

} // namespace BusinessLayer

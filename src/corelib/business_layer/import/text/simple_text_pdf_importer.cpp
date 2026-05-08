#include "simple_text_pdf_importer.h"

#include "TableExtraction.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QFileInfo>
#include <QTextBlock>
#include <QTextDocument>
#include <QXmlStreamWriter>


namespace BusinessLayer {

SimpleTextPdfImporter::SimpleTextPdfImporter()
    : AbstractSimpleTextImporter()
    , AbstractDocumentImporter()
{
}

SimpleTextPdfImporter::~SimpleTextPdfImporter() = default;


AbstractSimpleTextImporter::SimpleText SimpleTextPdfImporter::importSimpleText(
    const ImportOptions& _options) const
{
    SimpleText simpleText;
    simpleText.name = QFileInfo(_options.filePath).completeBaseName();

    if (_options.importText == false) {
        return { simpleText };
    }

    //
    // Преобразуем заданный документ в QTextDocument и парсим его
    //
    if (QTextDocument document; documentForImport(_options.filePath, document)) {
        simpleText.text = parseDocument(_options, document);
    }

    return { simpleText };
}

bool SimpleTextPdfImporter::documentForImport(const QString& _filePath,
                                              QTextDocument& _document) const
{
    QFile documentFile(_filePath);
    if (documentFile.open(QIODevice::ReadOnly)) {
        //
        // Используем TableExtraction, чтобы извлечь не только текст, но и линии
        //
        TableExtraction tableExtractor;
        tableExtractor.ExtractTables(_filePath.toStdString(), 0, -1, false);
        tableExtractor.GetResultsAsDocument(_document);
        return true;
    }
    return false;
}

TextParagraphType SimpleTextPdfImporter::typeForTextCursor(const QTextCursor& _cursor,
                                                           TextParagraphType _lastBlockType,
                                                           int _prevEmptyLines,
                                                           int _minLeftMargin) const
{
    Q_UNUSED(_cursor)
    Q_UNUSED(_lastBlockType)
    Q_UNUSED(_prevEmptyLines)
    Q_UNUSED(_minLeftMargin)

    return TextParagraphType::Text;
}

void SimpleTextPdfImporter::writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const
{
    const QTextBlock currentBlock = _cursor.block();
    if (!currentBlock.textFormats().isEmpty()) {
        _writer.writeStartElement(xml::kReviewMarksTag);
        for (const auto& range : currentBlock.textFormats()) {
            if (range.format.hasProperty(QTextFormat::BackgroundBrush)
                || range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                _writer.writeStartElement(xml::kReviewMarkTag);
                _writer.writeAttribute(xml::kFromAttribute, QString::number(range.start));
                _writer.writeAttribute(xml::kLengthAttribute, QString::number(range.length));
                if (range.format.hasProperty(QTextFormat::ForegroundBrush)) {
                    _writer.writeAttribute(xml::kColorAttribute,
                                           range.format.foreground().color().name());
                }
                if (range.format.hasProperty(QTextFormat::BackgroundBrush)) {
                    _writer.writeAttribute(xml::kBackgroundColorAttribute,
                                           range.format.background().color().name());
                }
                //
                // Пишем пустой комментарий
                //
                _writer.writeStartElement(xml::kCommentTag);
                _writer.writeAttribute(
                    xml::kAuthorAttribute,
                    DataStorageLayer::StorageFacade::settingsStorage()->accountName());
                _writer.writeAttribute(xml::kDateAttribute,
                                       QDateTime::currentDateTime().toString(Qt::ISODate));
                _writer.writeCDATA(QString());
                _writer.writeEndElement(); // comment
                //
                _writer.writeEndElement(); // review mark
            }
        }
        _writer.writeEndElement(); // review marks
    }
}

} // namespace BusinessLayer

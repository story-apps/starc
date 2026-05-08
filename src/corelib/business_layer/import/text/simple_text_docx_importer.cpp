#include "simple_text_docx_importer.h"

#include <business_layer/import/import_options.h>
#include <business_layer/model/text/text_model_xml.h>
#include <business_layer/templates/text_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/text_helper.h>

#include <QFileInfo>
#include <QTextBlock>
#include <QTextDocument>

#include <format_helpers.h>
#include <format_manager.h>
#include <format_reader.h>


namespace BusinessLayer {

SimpleTextDocxImporter::SimpleTextDocxImporter()
    : AbstractSimpleTextImporter()
    , AbstractDocumentImporter()
{
}

SimpleTextDocxImporter::~SimpleTextDocxImporter() = default;


AbstractSimpleTextImporter::SimpleText SimpleTextDocxImporter::importSimpleText(
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

bool SimpleTextDocxImporter::documentForImport(const QString& _filePath,
                                               QTextDocument& _document) const
{
    QFile documentFile(_filePath);
    if (documentFile.open(QIODevice::ReadOnly)) {
        QScopedPointer<FormatReader> reader(FormatManager::createReader(&documentFile));
        reader->read(&documentFile, &_document);
        return true;
    }
    return false;
}

TextParagraphType SimpleTextDocxImporter::typeForTextCursor(const QTextCursor& _cursor,
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

void SimpleTextDocxImporter::writeReviewMarks(QXmlStreamWriter& _writer, QTextCursor& _cursor) const
{
    const QTextBlock currentBlock = _cursor.block();
    if (!currentBlock.textFormats().isEmpty()) {
        _writer.writeStartElement(xml::kReviewMarksTag);
        for (const auto& range : currentBlock.textFormats()) {
            if (range.format.boolProperty(Docx::IsForeground)
                || range.format.boolProperty(Docx::IsBackground)
                || range.format.boolProperty(Docx::IsHighlight)
                || range.format.boolProperty(Docx::IsComment)) {
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
                // ... комментарии
                //
                QStringList authors = range.format.property(Docx::CommentsAuthors).toStringList();
                if (authors.isEmpty()) {
                    authors.append(
                        DataStorageLayer::StorageFacade::settingsStorage()->accountName());
                }
                QStringList dates = range.format.property(Docx::CommentsDates).toStringList();
                if (dates.isEmpty()) {
                    dates.append(QDateTime::currentDateTime().toString(Qt::ISODate));
                }
                QStringList comments = range.format.property(Docx::Comments).toStringList();
                if (comments.isEmpty()) {
                    comments.append(QString());
                }
                for (int commentIndex = 0; commentIndex < comments.size(); ++commentIndex) {
                    _writer.writeStartElement(xml::kCommentTag);
                    _writer.writeAttribute(xml::kAuthorAttribute, authors.at(commentIndex));
                    _writer.writeAttribute(xml::kDateAttribute, dates.at(commentIndex));
                    _writer.writeCDATA(TextHelper::toHtmlEscaped(comments.at(commentIndex)));
                    _writer.writeEndElement(); // comment
                }
                //
                _writer.writeEndElement(); // review mark
            }
        }
        _writer.writeEndElement(); // review marks
    }
}

} // namespace BusinessLayer

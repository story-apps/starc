#include "abstract_exporter.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/text/text_model.h>
#include <business_layer/templates/text_template.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>

#include <QGuiApplication>
#include <QTextBlock>


namespace BusinessLayer {

TextDocument* AbstractExporter::prepareDocument(AbstractModel* _model,
                                                const ExportOptions& _exportOptions) const
{
    auto textModel = qobject_cast<TextModel*>(_model);
    Q_ASSERT(textModel);

    //
    // Настраиваем документ
    //
    PageTextEdit textEdit;
    textEdit.setUsePageMode(true);
    textEdit.setPageSpacing(0);
    auto textDocument = createDocument(_exportOptions);
    textEdit.setDocument(textDocument);
    //
    // ... параметры страницы
    //
    const auto& exportTemplate = documentTemplate(_exportOptions);
    textEdit.setPageFormat(exportTemplate.pageSizeId());
    textEdit.setPageMarginsMm(exportTemplate.pageMargins());
    textEdit.setPageNumbersAlignment(exportTemplate.pageNumbersAlignment());
    //
    // ... формируем текст сценария
    //
    textDocument->setModel(textModel, false);
    //
    // ... отсоединяем документ от модели, что изменения в документе не привели к изменениям модели
    //
    textDocument->disconnect(_model);
    //
    // ... корректируем текст сценария
    //
    TextCursor cursor(textDocument);
    cursor.beginEditBlock();
    //
    // ... вставляем титульную страницу
    //
    if (_exportOptions.includeTiltePage) {
        //
        // Переносим основной текст и данные на следующую страницу
        //
        TextBlockData* firstBlockUserData = nullptr;
        if (cursor.block().userData() != nullptr) {
            firstBlockUserData
                = new TextBlockData(static_cast<TextBlockData*>(cursor.block().userData()));
            cursor.block().setUserData(nullptr);
        }
        cursor.insertBlock(cursor.blockFormat(), cursor.blockCharFormat());
        auto blockFormat = cursor.blockFormat();
        blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
        blockFormat.setTopMargin(0);
        cursor.setBlockFormat(blockFormat);
        cursor.block().setUserData(firstBlockUserData);

        //
        // Собственно добавляем текст титульной страницы
        //
        auto titlePageText = new SimpleTextDocument;
        titlePageText->setModel(textModel->titlePageModel(), false);
        //
        cursor.movePosition(TextCursor::PreviousBlock);
        auto block = titlePageText->begin();
        while (block.isValid()) {
            //
            // Донастроим стиль блока
            //
            auto blockFormat = block.blockFormat();
            //
            // ... сбросим тип
            //
            blockFormat.setProperty(TextBlockStyle::PropertyType,
                                    static_cast<int>(TextParagraphType::Undefined));
            //
            // ... и уравняем отступы
            //
            if (exportTemplate.pageMargins().left() < exportTemplate.pageMargins().right()) {
                blockFormat.setLeftMargin(MeasurementHelper::mmToPx(
                    exportTemplate.pageMargins().right() - exportTemplate.pageMargins().left()));
            } else if (exportTemplate.pageMargins().right() < exportTemplate.pageMargins().left()) {
                blockFormat.setRightMargin(MeasurementHelper::mmToPx(
                    exportTemplate.pageMargins().left() - exportTemplate.pageMargins().right()));
            }
            //
            // ... вставляем блок
            //
            if (cursor.atStart()) {
                cursor.setBlockFormat(blockFormat);
                cursor.setBlockCharFormat(block.charFormat());
            } else {
                cursor.insertBlock(blockFormat, block.charFormat());
            }
            //
            // ... вставляем текст
            //
            const auto formats = block.textFormats();
            for (const auto& format : formats) {
                cursor.insertText(block.text().mid(format.start, format.length), format.format);
            }
            //
            // ... если первый блок пуст, добавим пробел, чтобы избежать косяка с сохранением в PDF
            //
            if (cursor.atStart()) {
                cursor.insertText(" ");
            }

            block = block.next();
        }

        //
        // Убираем нижний отступ у последнего блока титульной страницы
        //
        blockFormat = cursor.blockFormat();
        blockFormat.setBottomMargin(0);
        cursor.setBlockFormat(blockFormat);

        //
        // Переходим к тексту сценария
        //
        cursor.movePosition(TextCursor::NextBlock);
        cursor.movePosition(TextCursor::StartOfBlock);
    }
    //
    // ... вставляем синопсис
    //
    if (_exportOptions.includeSynopsis) {
        //
        // Переносим основной текст и данные на следующую страницу
        //
        TextBlockData* firstBlockUserData = nullptr;
        if (cursor.block().userData() != nullptr) {
            firstBlockUserData
                = new TextBlockData(static_cast<TextBlockData*>(cursor.block().userData()));
            cursor.block().setUserData(nullptr);
        }
        cursor.insertBlock(cursor.blockFormat(), cursor.blockCharFormat());
        auto blockFormat = cursor.blockFormat();
        blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
        blockFormat.setTopMargin(0);
        cursor.setBlockFormat(blockFormat);
        cursor.block().setUserData(firstBlockUserData);

        //
        // Собственно добавляем текст синопсиса
        //
        auto synopsisText = new TextDocument;
        synopsisText->setModel(textModel->synopsisModel(), false);
        //
        cursor.movePosition(TextCursor::PreviousBlock);
        auto block = synopsisText->begin();
        while (block.isValid()) {
            //
            // Донастроим стиль блока
            //
            auto blockFormat = block.blockFormat();
            //
            // ... сбросим тип
            //
            blockFormat.setProperty(TextBlockStyle::PropertyType,
                                    static_cast<int>(TextParagraphType::Undefined));
            //
            // ... вставляем блок
            //
            if (cursor.atStart()) {
                cursor.setBlockFormat(blockFormat);
                cursor.setBlockCharFormat(block.charFormat());
            } else {
                cursor.insertBlock(blockFormat, block.charFormat());
            }
            //
            // ... вставляем текст
            //
            const auto formats = block.textFormats();
            for (const auto& format : formats) {
                cursor.insertText(block.text().mid(format.start, format.length), format.format);
            }
            //
            // ... если первый блок пуст, добавим пробел, чтобы избежать косяка с сохранением в PDF
            //
            if (cursor.atStart()) {
                cursor.insertText(" ");
            }

            block = block.next();
        }

        //
        // Убираем нижний отступ у поледнего блока синопсиса
        //
        blockFormat = cursor.blockFormat();
        blockFormat.setBottomMargin(0);
        cursor.setBlockFormat(blockFormat);

        //
        // Переходим к тексту сценария
        //
        cursor.movePosition(TextCursor::NextBlock);
        cursor.movePosition(TextCursor::StartOfBlock);
    }
    //
    // ... корректируем сценарий, если он нужен
    //
    if (_exportOptions.includeText) {
        //
        // ... для первого блока убираем принудительный перенос страницы,
        //     если он есть и если не печатается титульная страница
        //
        if (cursor.atStart()
            && cursor.block().blockFormat().pageBreakPolicy()
                == QTextFormat::PageBreak_AlwaysBefore) {
            auto blockFormat = cursor.blockFormat();
            blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_Auto);
            cursor.setBlockFormat(blockFormat);
        }
        //
        do {
            const auto blockType = TextBlockStyle::forBlock(cursor.block());

            //
            // Если не нужно печатать папки, то удаляем их
            //
            if (!_exportOptions.includeFolders) {
                if (blockType == TextParagraphType::SequenceHeading
                    || blockType == TextParagraphType::SequenceFooter) {
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    if (cursor.hasSelection()) {
                        cursor.deleteChar();
                    }
                    cursor.deleteChar();
                    continue;
                }
            }

            //
            // Подставляем текст для пустых завершающих блоков
            //
            if (blockType == TextParagraphType::ActFooter
                || blockType == TextParagraphType::SequenceFooter
                || blockType == TextParagraphType::PartFooter
                || blockType == TextParagraphType::ChapterFooter) {
                if (cursor.block().text().isEmpty()) {
                    auto headerBlock = cursor.block().previous();
                    int openedFolders = 0;
                    while (headerBlock.isValid()) {
                        const auto headerBlockType = TextBlockStyle::forBlock(headerBlock);
                        if (headerBlockType == TextParagraphType::ActHeading
                            || headerBlockType == TextParagraphType::SequenceHeading
                            || headerBlockType == TextParagraphType::PartHeading
                            || headerBlockType == TextParagraphType::ChapterHeading) {
                            if (openedFolders > 0) {
                                --openedFolders;
                            } else {
                                break;
                            }
                        } else if (headerBlockType == TextParagraphType::ActFooter
                                   || headerBlockType == TextParagraphType::SequenceFooter
                                   || headerBlockType == TextParagraphType::PartFooter
                                   || headerBlockType == TextParagraphType::ChapterFooter) {
                            ++openedFolders;
                        }

                        headerBlock = headerBlock.previous();
                    }

                    const auto footerText = QString("%1 %2").arg(
                        QGuiApplication::translate("KeyProcessingLayer::FolderFooterHandler",
                                                   "End of"),
                        headerBlock.text());
                    cursor.insertText(footerText);
                    cursor.movePosition(QTextCursor::StartOfBlock);
                }
            }

            //
            // Если не нужно печатать заметки по тексту, то удаляем их
            //
            if (!_exportOptions.includeInlineNotes && blockType == TextParagraphType::InlineNote) {
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }
                cursor.deleteChar();
                continue;
            }

            //
            // Обрабатываем блок в наследниках
            //
            const auto skipMovement = prepareBlock(_exportOptions, cursor);
            if (skipMovement) {
                continue;
            }

            //
            // Если не нужно печатать редакторские заметки, убираем их
            //
            if (!_exportOptions.includeReviewMarks) {
                const auto blockPosition = cursor.block().position();
                for (const auto& formatRange : cursor.block().textFormats()) {
                    if (formatRange.format.boolProperty(TextBlockStyle::PropertyIsReviewMark)
                        == false) {
                        continue;
                    }

                    auto blockFormat = cursor.blockCharFormat();
                    blockFormat.setFontWeight(formatRange.format.fontWeight());
                    blockFormat.setFontItalic(formatRange.format.fontItalic());
                    blockFormat.setFontUnderline(formatRange.format.fontUnderline());
                    cursor.setPosition(blockPosition + formatRange.start);
                    cursor.setPosition(blockPosition + formatRange.start + formatRange.length,
                                       QTextCursor::KeepAnchor);
                    cursor.setCharFormat(blockFormat);
                }
                cursor.movePosition(QTextCursor::EndOfBlock);
            }

            //
            // Переходим к следующему блоку
            //
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);
        } while (!cursor.atEnd());
    }
    //
    // ... а если не нужен, удаляем его
    //
    else {
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        cursor.deleteChar();
    }

    cursor.endEditBlock();

    return textDocument;
}

bool AbstractExporter::prepareBlock(const ExportOptions& _exportOptions, TextCursor& _cursor) const
{
    Q_UNUSED(_exportOptions)
    Q_UNUSED(_cursor)

    return false;
}

} // namespace BusinessLayer

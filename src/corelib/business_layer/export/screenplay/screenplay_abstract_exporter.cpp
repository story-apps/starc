#include "screenplay_abstract_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/screenplay/text/screenplay_text_cursor.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/document/text/text_document.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>

#include <QGuiApplication>
#include <QTextBlock>


namespace BusinessLayer {

ScreenplayTextDocument* ScreenplayAbstractExporter::prepareDocument(
    ScreenplayTextModel* _model, const ScreenplayExportOptions& _exportOptions) const
{
    //
    // Настраиваем документ
    //
    PageTextEdit textEdit;
    textEdit.setUsePageMode(true);
    textEdit.setPageSpacing(0);
    auto screenplayText = new ScreenplayTextDocument;
    textEdit.setDocument(screenplayText);
    //
    // ... параметры страницы
    //
    const auto exportTemplate = TemplatesFacade::screenplayTemplate(_exportOptions.templateId);
    textEdit.setPageFormat(exportTemplate.pageSizeId());
    textEdit.setPageMarginsMm(exportTemplate.pageMargins());
    textEdit.setPageNumbersAlignment(exportTemplate.pageNumbersAlignment());
    //
    // ... формируем текст сценария
    //
    screenplayText->setTemplateId(_exportOptions.templateId);
    screenplayText->setModel(_model, false);
    //
    // ... отсоединяем документ от модели, что изменения в документе не привели к изменениям модели
    //
    screenplayText->disconnect();
    //
    // ... корректируем текст сценария
    //
    ScreenplayTextCursor cursor(screenplayText);
    //
    // ... вставляем титульную страницу
    //
    if (_exportOptions.printTiltePage) {
        //
        // Переносим основной текст на следующую страницу
        //
        cursor.insertBlock(cursor.blockFormat(), cursor.blockCharFormat());
        auto blockFormat = cursor.blockFormat();
        blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
        blockFormat.setTopMargin(0);
        cursor.setBlockFormat(blockFormat);

        //
        // Собственно добавляем текст титульной страницы
        //
        auto titlePageText = new SimpleTextDocument;
        titlePageText->setModel(_model->titlePageModel(), false);
        //
        cursor.movePosition(ScreenplayTextCursor::Start);
        auto block = titlePageText->begin();
        while (block.isValid()) {
            //
            // Донастроим стиль блока
            //
            auto blockFormat = block.blockFormat();
            //
            // ... сбросим тип
            //
            blockFormat.setProperty(ScreenplayBlockStyle::PropertyType,
                                    static_cast<int>(ScreenplayParagraphType::Undefined));
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
        // Убираем нижний отступ у поледнего блока титульной страницы
        //
        blockFormat = cursor.blockFormat();
        blockFormat.setBottomMargin(0);
        cursor.setBlockFormat(blockFormat);

        //
        // Переходим к тексту сценария
        //
        cursor.movePosition(ScreenplayTextCursor::NextBlock);
        cursor.movePosition(ScreenplayTextCursor::StartOfBlock);
    }
    //
    // ... для первого блока убираем принудительный перенос страницы,
    //     если он есть и если не печатается титульная страница
    //
    else if (cursor.block().blockFormat().pageBreakPolicy()
             == QTextFormat::PageBreak_AlwaysBefore) {
        auto blockFormat = cursor.blockFormat();
        blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_Auto);
        cursor.setBlockFormat(blockFormat);
    }
    //
    do {
        const auto blockType = ScreenplayBlockStyle::forBlock(cursor.block());

        //
        // Если не нужно печатать папки, то удаляем их
        //
        if (!_exportOptions.printFolders) {
            if (blockType == ScreenplayParagraphType::FolderHeader
                || blockType == ScreenplayParagraphType::FolderFooter) {
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }
                cursor.deleteChar();
                continue;
            }
        }
        //
        // В противном случае подставляем текст для пустых завершающих блоков
        //
        else if (blockType == ScreenplayParagraphType::FolderFooter) {
            if (cursor.block().text().isEmpty()) {
                auto headerBlock = cursor.block().previous();
                int openedFolders = 0;
                while (headerBlock.isValid()) {
                    const auto headerBlockType = ScreenplayBlockStyle::forBlock(headerBlock);
                    if (headerBlockType == ScreenplayParagraphType::FolderHeader) {
                        if (openedFolders > 0) {
                            --openedFolders;
                        } else {
                            break;
                        }
                    } else if (headerBlockType == ScreenplayParagraphType::FolderFooter) {
                        ++openedFolders;
                    }

                    headerBlock = headerBlock.previous();
                }

                const auto footerText = QString("%1 %2").arg(
                    QGuiApplication::translate("KeyProcessingLayer::FolderFooterHandler", "END OF"),
                    headerBlock.text());
                cursor.insertText(footerText);
            }
        }

        //
        // Если не нужно печатать заметки по тексту, то удаляем их
        //
        if (!_exportOptions.printInlineNotes && blockType == ScreenplayParagraphType::InlineNote) {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (cursor.hasSelection()) {
                cursor.deleteChar();
            }
            cursor.deleteChar();
            continue;
        }

        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    } while (!cursor.atEnd());

    return screenplayText;
}

} // namespace BusinessLayer

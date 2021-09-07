#include "screenplay_abstract_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/screenplay/text/screenplay_text_cursor.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>

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
    screenplayText->disconnect(_model);
    //
    // ... корректируем текст сценария
    //
    ScreenplayTextCursor cursor(screenplayText);
    //
    // ... для первого блока убираем принудительный перенос страницы, если есть
    //
    if (cursor.block().blockFormat().pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore) {
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

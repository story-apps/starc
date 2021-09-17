#include "comic_book_abstract_exporter.h"

#include "comic_book_export_options.h"

#include <business_layer/document/comic_book/text/comic_book_text_cursor.h>
#include <business_layer/document/comic_book/text/comic_book_text_document.h>
#include <business_layer/document/text/text_document.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_metrics.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>

#include <QGuiApplication>
#include <QTextBlock>


namespace BusinessLayer {

ComicBookTextDocument* ComicBookAbstractExporter::prepareDocument(
    ComicBookTextModel* _model, const ComicBookExportOptions& _exportOptions) const
{
    //
    // Настраиваем документ
    //
    PageTextEdit textEdit;
    textEdit.setUsePageMode(true);
    textEdit.setPageSpacing(0);
    auto comicBookText = new ComicBookTextDocument;
    textEdit.setDocument(comicBookText);
    //
    // ... параметры страницы
    //
    const auto exportTemplate = TemplatesFacade::comicBookTemplate(_exportOptions.templateId);
    textEdit.setPageFormat(exportTemplate.pageSizeId());
    textEdit.setPageMarginsMm(exportTemplate.pageMargins());
    textEdit.setPageNumbersAlignment(exportTemplate.pageNumbersAlignment());
    //
    // ... формируем текст сценария
    //
    comicBookText->setTemplateId(_exportOptions.templateId);
    comicBookText->setModel(_model, false);
    //
    // ... отсоединяем документ от модели, что изменения в документе не привели к изменениям модели
    //
    comicBookText->disconnect();
    //
    // ... корректируем текст сценария
    //
    ComicBookTextCursor cursor(comicBookText);
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
        cursor.movePosition(ComicBookTextCursor::Start);
        auto block = titlePageText->begin();
        while (block.isValid()) {
            //
            // Донастроим стиль блока
            //
            auto blockFormat = block.blockFormat();
            //
            // ... сбросим тип
            //
            blockFormat.setProperty(ComicBookBlockStyle::PropertyType,
                                    static_cast<int>(ComicBookParagraphType::Undefined));
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
        cursor.movePosition(ComicBookTextCursor::NextBlock);
        cursor.movePosition(ComicBookTextCursor::StartOfBlock);
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
        const auto blockType = ComicBookBlockStyle::forBlock(cursor.block());

        //
        // Если не нужно печатать папки, то удаляем их
        //
        if (!_exportOptions.printFolders) {
            if (blockType == ComicBookParagraphType::FolderHeader
                || blockType == ComicBookParagraphType::FolderFooter) {
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
        else if (blockType == ComicBookParagraphType::FolderFooter) {
            if (cursor.block().text().isEmpty()) {
                auto headerBlock = cursor.block().previous();
                int openedFolders = 0;
                while (headerBlock.isValid()) {
                    const auto headerBlockType = ComicBookBlockStyle::forBlock(headerBlock);
                    if (headerBlockType == ComicBookParagraphType::FolderHeader) {
                        if (openedFolders > 0) {
                            --openedFolders;
                        } else {
                            break;
                        }
                    } else if (headerBlockType == ComicBookParagraphType::FolderFooter) {
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
        if (!_exportOptions.printInlineNotes && blockType == ComicBookParagraphType::InlineNote) {
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

    return comicBookText;
}

} // namespace BusinessLayer

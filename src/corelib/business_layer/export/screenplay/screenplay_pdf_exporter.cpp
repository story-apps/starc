#include "screenplay_pdf_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/screenplay/text/screenplay_text_corrector.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QLocale>
#include <QPainter>
#include <QPdfWriter>
#include <QTextBlock>
#include <QtMath>

#include <cmath>


namespace BusinessLayer {

namespace {

/**
 * @brief Напечатать страницу документа
 * @note Адаптация функции QTextDocument.cpp::anonymous::printPage
 */
static void printPage(int _pageNumber, QPainter* _painter, const QTextDocument* _document,
                      const QRectF& _body, const ScreenplayTemplate& _template,
                      const ScreenplayExportOptions& _exportOptions)
{
    const qreal pageYPos = (_pageNumber - 1) * _body.height();

    _painter->save();
    _painter->translate(_body.left(), _body.top() - pageYPos);
    QRectF currentPageRect(0, pageYPos, _body.width(), _body.height());
    QAbstractTextDocumentLayout* layout = _document->documentLayout();
    QAbstractTextDocumentLayout::PaintContext ctx;
    _painter->setClipRect(currentPageRect);
    ctx.clip = currentPageRect;
    // don't use the system palette text as default text color, on HP/UX
    // for example that's white, and white text on white paper doesn't
    // look that nice
    ctx.palette.setColor(QPalette::Text, Qt::black);
    layout->draw(_painter, ctx);

    //
    // Печатаем декорации текста
    //
    {
        _painter->save();
        const QRectF fullWidthPageRect(0, pageYPos, _body.width(), _body.height());
        _painter->setClipRect(fullWidthPageRect);

        const int blockPos = layout->hitTest(QPointF(0, pageYPos), Qt::FuzzyHit);
        QTextBlock block = _document->findBlock(std::max(0, blockPos));
        while (block.isValid()) {
            const QRectF blockRect = layout->blockBoundingRect(block);
            if (blockRect.bottom() > pageYPos + _body.height()
                    - MeasurementHelper::mmToPx(_template.pageMargins().bottom())) {
                break;
            }

            //
            // Покажем номер сцены, если необходимо
            //
            const auto paragraphType = TextBlockStyle::forBlock(block);
            if (paragraphType == TextParagraphType::SceneHeading && !block.text().isEmpty()
                && _exportOptions.showScenesNumbers) {
                const auto blockData = static_cast<TextBlockData*>(block.userData());
                if (blockData != nullptr) {
                    _painter->setFont(block.charFormat().font());
                    //
                    const auto sceneItem
                        = static_cast<ScreenplayTextModelSceneItem*>(blockData->item()->parent());
                    const int distanceBetweenSceneNumberAndText = 10;

                    if (_exportOptions.showScenesNumbersOnLeft) {
                        const QRectF leftSceneNumberRect(
                            0,
                            blockRect.top() <= pageYPos ? (
                                pageYPos + MeasurementHelper::mmToPx(_template.pageMargins().top()))
                                                        : blockRect.top(),
                            MeasurementHelper::mmToPx(_template.pageMargins().left())
                                - distanceBetweenSceneNumberAndText,
                            blockRect.height());
                        _painter->drawText(leftSceneNumberRect, Qt::AlignRight | Qt::AlignTop,
                                           sceneItem->number()->text);
                    }

                    if (_exportOptions.showScenesNumbersOnRight) {
                        const QRectF rightSceneNumberRect(
                            _body.width()
                                - MeasurementHelper::mmToPx(_template.pageMargins().right())
                                + distanceBetweenSceneNumberAndText,
                            blockRect.top() <= pageYPos ? (
                                pageYPos + MeasurementHelper::mmToPx(_template.pageMargins().top()))
                                                        : blockRect.top(),
                            MeasurementHelper::mmToPx(_template.pageMargins().right())
                                - distanceBetweenSceneNumberAndText,
                            blockRect.height());
                        _painter->drawText(rightSceneNumberRect, Qt::AlignLeft | Qt::AlignTop,
                                           sceneItem->number()->text);
                    }
                }
            }
            //
            // Печатаем номер диалога, если необходимо, а также автоматические (ПРОД)
            //
            else if (paragraphType == TextParagraphType::Character && !block.text().isEmpty()) {
                //
                // Номера реплик
                //
                if (_exportOptions.showDialoguesNumbers) {
                    const auto blockData = static_cast<TextBlockData*>(block.userData());
                    if (blockData != nullptr) {
                        _painter->setFont(block.charFormat().font());
                        //
                        const auto textItem
                            = static_cast<ScreenplayTextModelTextItem*>(blockData->item());
                        if (textItem && textItem->number().has_value()) {
                            const QString dialogueNumber = textItem->number()->text;
                            const int numberDelta
                                = _painter->fontMetrics().horizontalAdvance(dialogueNumber);
                            QRectF dialogueNumberRect;
                            if (QLocale().textDirection() == Qt::LeftToRight) {
                                if (block.blockFormat().leftMargin() > numberDelta) {
                                    dialogueNumberRect = QRectF(
                                        MeasurementHelper::mmToPx(_template.pageMargins().left()),
                                        blockRect.top() <= pageYPos
                                            ? (pageYPos
                                               + MeasurementHelper::mmToPx(
                                                   _template.pageMargins().top()))
                                            : blockRect.top(),
                                        numberDelta, blockRect.height());
                                } else {
                                    const int distanceBetweenSceneNumberAndText = 10;
                                    dialogueNumberRect = QRectF(
                                        0,
                                        blockRect.top() <= pageYPos
                                            ? (pageYPos
                                               + MeasurementHelper::mmToPx(
                                                   _template.pageMargins().top()))
                                            : blockRect.top(),
                                        MeasurementHelper::mmToPx(_template.pageMargins().left())
                                            - distanceBetweenSceneNumberAndText,
                                        blockRect.height());
                                }
                            } else {
                                if (block.blockFormat().rightMargin() > numberDelta) {
                                    dialogueNumberRect = QRectF(
                                        MeasurementHelper::mmToPx(_template.pageMargins().left())
                                            + _body.width() - numberDelta,
                                        blockRect.top() <= pageYPos
                                            ? (pageYPos
                                               + MeasurementHelper::mmToPx(
                                                   _template.pageMargins().top()))
                                            : blockRect.top(),
                                        numberDelta, blockRect.height());
                                } else {
                                    const int distanceBetweenSceneNumberAndText = 10;
                                    dialogueNumberRect = QRectF(
                                        MeasurementHelper::mmToPx(_template.pageMargins().left())
                                            + _body.width() + distanceBetweenSceneNumberAndText,
                                        blockRect.top() <= pageYPos
                                            ? (pageYPos
                                               + MeasurementHelper::mmToPx(
                                                   _template.pageMargins().top()))
                                            : blockRect.top(),
                                        MeasurementHelper::mmToPx(_template.pageMargins().right())
                                            - distanceBetweenSceneNumberAndText,
                                        blockRect.height());
                                }
                            }
                            _painter->drawText(dialogueNumberRect, Qt::AlignRight | Qt::AlignTop,
                                               dialogueNumber);
                        }
                    }
                }

                //
                // Автоматические (ПРОД)
                //
                if (block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCharacterContinued)
                    && !block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
                    _painter->setFont(block.charFormat().font());

                    const auto continuedTerm
                        = BusinessLayer::ScreenplayTextCorrector::continuedTerm();

                    //
                    // Почему-то если взять просто ширину последней строки текста, то получается
                    // слишком широко в некоторых случаях, так, что постфикс рисуется очень далеко
                    // от текста. Поэтому решил брать текст последней строки, добавлять к нему
                    // постфикс, считать их совместную ширину и брать её, как конечную точку
                    //
                    const auto lastLineText = TextHelper::lastLineText(
                                                  block.text(), _painter->font(), blockRect.width())
                        + continuedTerm;
                    const auto correctedBlockRect
                        = QRectF({ block.blockFormat().leftMargin()
                                       + MeasurementHelper::mmToPx(_template.pageMargins().left()),
                                   blockRect.top() },
                                 blockRect.size());
                    const QPoint bottomRight
                        = QPoint(blockRect.left() + block.blockFormat().leftMargin()
                                     + TextHelper::fineTextWidthF(lastLineText, _painter->font()),
                                 correctedBlockRect.bottom());
                    const QPoint topLeft = QPoint(
                        bottomRight.x()
                            - TextHelper::fineTextWidthF(continuedTerm, _painter->font()),
                        correctedBlockRect.bottom()
                            - _painter->fontMetrics().boundingRect(continuedTerm).height());
                    const QRect postfixRect(topLeft, bottomRight);
                    _painter->drawText(postfixRect, Qt::AlignLeft | Qt::AlignBottom, continuedTerm);
                }
            }

            //
            // Прорисовка префикса/постфикса для блока текста, если это не пустая декорация
            //
            if (!block.text().isEmpty()
                || !block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
                const auto correctedBlockRect
                    = QRectF({ block.blockFormat().leftMargin()
                                   + MeasurementHelper::mmToPx(_template.pageMargins().left()),
                               blockRect.top() },
                             blockRect.size());
                //
                // ... префикс
                //
                if (block.charFormat().hasProperty(TextBlockStyle::PropertyPrefix)) {
                    _painter->setFont(block.charFormat().font());

                    const auto prefix
                        = block.charFormat().stringProperty(TextBlockStyle::PropertyPrefix);
                    auto prefixRect = blockRect;
                    prefixRect.setWidth(TextHelper::fineTextWidthF(prefix, _painter->font()));
                    prefixRect.moveLeft(prefixRect.left() + block.blockFormat().leftMargin()
                                        - prefixRect.width());
                    prefixRect.setHeight(_painter->fontMetrics().boundingRect(prefix).height());
                    _painter->drawText(prefixRect, Qt::AlignLeft | Qt::AlignBottom, prefix);
                }
                //
                // ... постфикс
                //
                if (block.charFormat().hasProperty(TextBlockStyle::PropertyPostfix)) {
                    _painter->setFont(block.charFormat().font());

                    const auto postfix
                        = block.charFormat().stringProperty(TextBlockStyle::PropertyPostfix);

                    //
                    // Почему-то если взять просто ширину последней строки текста, то получается
                    // слишком широко в некоторых случаях, так, что постфикс рисуется очень далеко
                    // от текста. Поэтому решил брать текст последней строки, добавлять к нему
                    // постфикс, считать их совместную ширину и брать её, как конечную точку
                    //
                    const auto lastLineText = TextHelper::lastLineText(
                                                  block.text(), _painter->font(), blockRect.width())
                        + postfix;
                    const QPoint bottomRight
                        = QPoint(blockRect.left() + block.blockFormat().leftMargin()
                                     + TextHelper::fineTextWidthF(lastLineText, _painter->font()),
                                 correctedBlockRect.bottom());
                    const QPoint topLeft = QPoint(
                        bottomRight.x() - TextHelper::fineTextWidthF(postfix, _painter->font()),
                        correctedBlockRect.bottom()
                            - _painter->fontMetrics().boundingRect(postfix).height());
                    const QRect postfixRect(topLeft, bottomRight);
                    _painter->drawText(postfixRect, Qt::AlignLeft | Qt::AlignBottom, postfix);
                }
            }

            block = block.next();
        }
        _painter->restore();
    }

    //
    // Рисуем нумерацию страниц
    //
    {
        //
        // На титульной и на первой странице сценария
        //
        if ((_exportOptions.includeTiltePage && _pageNumber < 3) || _pageNumber == 1) {
            //
            // ... не печатаем номер
            //
        }
        //
        // Печатаем номера страниц
        //
        else {
            _painter->save();
            _painter->setFont(
                _template.paragraphStyle(TextParagraphType::Action).charFormat().font());

            //
            // Середины верхнего и нижнего полей
            //
            qreal headerY = pageYPos + MeasurementHelper::mmToPx(_template.pageMargins().top()) / 2;
            qreal footerY = pageYPos + currentPageRect.height()
                - MeasurementHelper::mmToPx(_template.pageMargins().bottom()) / 2;

            //
            // Области для прорисовки текста на полях
            //
            QRectF headerRect(MeasurementHelper::mmToPx(_template.pageMargins().left()), headerY,
                              currentPageRect.width()
                                  - MeasurementHelper::mmToPx(_template.pageMargins().left())
                                  - MeasurementHelper::mmToPx(_template.pageMargins().right()),
                              20);
            QRectF footerRect(MeasurementHelper::mmToPx(_template.pageMargins().left()), footerY,
                              currentPageRect.width()
                                  - MeasurementHelper::mmToPx(_template.pageMargins().left())
                                  - MeasurementHelper::mmToPx(_template.pageMargins().right()),
                              20);

            //
            // Определяем где положено находиться нумерации
            //
            QRectF numberingRect;
            if (_template.pageNumbersAlignment().testFlag(Qt::AlignTop)) {
                numberingRect = headerRect;
            } else {
                numberingRect = footerRect;
            }
            Qt::Alignment numberingAlignment = Qt::AlignVCenter;
            if (_template.pageNumbersAlignment().testFlag(Qt::AlignLeft)) {
                numberingAlignment |= Qt::AlignLeft;
            } else if (_template.pageNumbersAlignment().testFlag(Qt::AlignCenter)) {
                numberingAlignment |= Qt::AlignCenter;
            } else {
                numberingAlignment |= Qt::AlignRight;
            }

            //
            // Рисуем нумерацию в положеном месте (отнимаем единицу, т.к. нумерация
            // должна следовать с единицы для первой страницы текста сценария)
            //
            int titleDelta = _exportOptions.includeTiltePage ? -1 : 0;
            _painter->setClipRect(numberingRect);
            _painter->drawText(numberingRect, numberingAlignment,
                               QString(QLocale().textDirection() == Qt::LeftToRight ? "%1." : ".%1")
                                   .arg(_pageNumber + titleDelta));
            _painter->restore();
        }
    }

    //
    // Печатаем колонтитулы, если необходимо
    //
    if (!_exportOptions.header.isEmpty() || !_exportOptions.footer.isEmpty()) {
        const bool printHeader = _pageNumber > 1 || !_exportOptions.includeTiltePage
            || _exportOptions.printHeaderOnTitlePage;
        const bool printFooter = _pageNumber > 1 || !_exportOptions.includeTiltePage
            || _exportOptions.printFooterOnTitlePage;

        _painter->save();
        _painter->setFont(_template.paragraphStyle(TextParagraphType::Action).charFormat().font());

        //
        // Середины верхнего и нижнего полей
        //
        qreal headerY = pageYPos + MeasurementHelper::mmToPx(_template.pageMargins().top()) / 2;
        qreal footerY = pageYPos + currentPageRect.height()
            - MeasurementHelper::mmToPx(_template.pageMargins().bottom()) / 2;

        //
        // Области для прорисовки текста на полях
        //
        QRectF headerRect(MeasurementHelper::mmToPx(_template.pageMargins().left()), headerY,
                          currentPageRect.width()
                              - MeasurementHelper::mmToPx(_template.pageMargins().left())
                              - MeasurementHelper::mmToPx(_template.pageMargins().right()),
                          20);
        QRectF footerRect(MeasurementHelper::mmToPx(_template.pageMargins().left()), footerY,
                          currentPageRect.width()
                              - MeasurementHelper::mmToPx(_template.pageMargins().left())
                              - MeasurementHelper::mmToPx(_template.pageMargins().right()),
                          20);

        //
        // Определяем где положено находиться нумерации
        //
        Qt::Alignment headerAlignment = Qt::AlignVCenter;
        Qt::Alignment footerAlignment = Qt::AlignVCenter;
        if (_template.pageNumbersAlignment().testFlag(Qt::AlignTop)) {
            if (_template.pageNumbersAlignment().testFlag(Qt::AlignLeft)) {
                headerAlignment |= Qt::AlignRight;
            } else {
                headerAlignment |= Qt::AlignLeft;
            }
        } else {
            if (_template.pageNumbersAlignment().testFlag(Qt::AlignLeft)) {
                footerAlignment |= Qt::AlignRight;
            } else {
                footerAlignment |= Qt::AlignLeft;
            }
        }

        //
        // Рисуем колонтитулы
        //
        _painter->setClipRect(headerRect);
        if (printHeader) {
            _painter->drawText(headerRect, headerAlignment, _exportOptions.header);
        }
        _painter->setClipRect(footerRect);
        if (printFooter) {
            _painter->drawText(footerRect, footerAlignment, _exportOptions.footer);
        }
        _painter->restore();
    }

    _painter->restore();

    //
    // Рисуем водяные знаки
    //
    if (!_exportOptions.watermark.isEmpty()) {
        const QString watermark = "  " + _exportOptions.watermark + "    ";
        _painter->save();

        //
        // Рассчитаем какого размера нужен шрифт
        //
        QFont font;
        font.setBold(true);
        font.setPixelSize(400);
        const int maxWidth = static_cast<int>(sqrt(pow(_body.height(), 2) + pow(_body.width(), 2)));
        QFontMetrics fontMetrics(font);
        while (fontMetrics.horizontalAdvance(watermark) > maxWidth) {
            font.setPixelSize(font.pixelSize() - 2);
            fontMetrics = QFontMetrics(font);
        }

        _painter->setFont(font);
        _painter->setPen(_exportOptions.watermarkColor);

        //
        // Рисуем водяной знак
        //
        _painter->rotate(qRadiansToDegrees(atan(_body.height() / _body.width())));
        const int delta = fontMetrics.height() / 4;
        _painter->drawText(delta, delta, watermark);

        //
        // TODO: Рисуем мусор на странице, чтобы текст нельзя было вытащить
        //

        _painter->restore();
    }
}

/**
 * @brief Напечатать документ
 * @note Адаптация функции QTextDocument::print
 */
static void printDocument(QTextDocument* _document, QPdfWriter* _printer,
                          const ScreenplayTemplate& _template,
                          const ScreenplayExportOptions& _exportOptions)
{
    QPainter painter(_printer);
    // Check that there is a valid device to print to.
    if (!painter.isActive())
        return;
    QScopedPointer<QTextDocument> clonedDoc;
    (void)_document->documentLayout(); // make sure that there is a layout
    QRectF body = QRectF(QPointF(0, 0), _document->pageSize());

    {
        qreal sourceDpiX = painter.device()->logicalDpiX();
        qreal sourceDpiY = sourceDpiX;
        QPaintDevice* dev = _document->documentLayout()->paintDevice();
        if (dev) {
            sourceDpiX = dev->logicalDpiX();
            sourceDpiY = dev->logicalDpiY();
        }
        const qreal dpiScaleX = qreal(_printer->logicalDpiX()) / sourceDpiX;
        const qreal dpiScaleY = qreal(_printer->logicalDpiY()) / sourceDpiY;
        // scale to dpi
        painter.scale(dpiScaleX, dpiScaleY);
        QSizeF scaledPageSize = _document->pageSize();
        scaledPageSize.rwidth() *= dpiScaleX;
        scaledPageSize.rheight() *= dpiScaleY;
        const QSizeF printerPageSize(painter.viewport().size());
        // scale to page
        painter.scale(printerPageSize.width() / scaledPageSize.width(),
                      printerPageSize.height() / scaledPageSize.height());
    }

    int docCopies = 1;
    int pageCopies = 1;
    int fromPage = 1;
    int toPage = _document->pageCount();
    bool ascending = true;
    // paranoia check
    fromPage = qMax(1, fromPage);
    toPage = qMin(_document->pageCount(), toPage);
    for (int i = 0; i < docCopies; ++i) {
        int page = fromPage;
        while (true) {
            for (int j = 0; j < pageCopies; ++j) {
                printPage(page, &painter, _document, body, _template, _exportOptions);
                if (j < pageCopies - 1)
                    _printer->newPage();
            }
            if (page == toPage)
                break;
            if (ascending)
                ++page;
            else
                --page;
            _printer->newPage();
        }
        if (i < docCopies - 1)
            _printer->newPage();
    }
}

} // namespace


void ScreenplayPdfExporter::exportTo(ScreenplayTextModel* _model,
                                     const ScreenplayExportOptions& _exportOptions) const
{
    //
    // Настраиваем документ
    //
    QScopedPointer<ScreenplayTextDocument> screenplayText(prepareDocument(_model, _exportOptions));

    //
    // Настраиваем принтер
    //
    const auto& exportTemplate = TemplatesFacade::screenplayTemplate(_exportOptions.templateId);
    QPdfWriter printer(_exportOptions.filePath);
    printer.setPageSize(QPageSize(exportTemplate.pageSizeId()));
    printer.setPageMargins({});

    //
    // Допишем параметры сценария в параметры экспорта
    //
    auto exportOptions = _exportOptions;
    exportOptions.header = _model->informationModel()->header();
    exportOptions.footer = _model->informationModel()->footer();

    //
    // Печатаем документ
    //
    printDocument(screenplayText.data(), &printer, exportTemplate, exportOptions);
}

} // namespace BusinessLayer

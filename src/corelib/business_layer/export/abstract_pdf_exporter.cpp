#include "abstract_pdf_exporter.h"

#include <business_layer/document/text/text_document.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractTextDocumentLayout>
#include <QLocale>
#include <QPainter>
#include <QPdfWriter>
#include <QTextBlock>
#include <QtMath>


namespace BusinessLayer {

class AbstractPdfExporter::Implementation
{
public:
    explicit Implementation(AbstractPdfExporter* _q);

    /**
     * @brief Напечатать страницу документа
     * @note Адаптация функции QTextDocument.cpp::anonymous::printPage
     */
    void printPage(int _pageNumber, QPainter* _painter, const QTextDocument* _document,
                   const QRectF& _body, const TextTemplate& _template,
                   const ExportOptions& _exportOptions) const;

    /**
     * @brief Напечатать документ
     * @note Адаптация функции QTextDocument::print
     */
    void printDocument(QTextDocument* _document, QPdfWriter* _printer,
                       const TextTemplate& _template, const ExportOptions& _exportOptions) const;


    AbstractPdfExporter* q = nullptr;
};

AbstractPdfExporter::Implementation::Implementation(AbstractPdfExporter* _q)
    : q(_q)
{
}

void AbstractPdfExporter::Implementation::printPage(int _pageNumber, QPainter* _painter,
                                                    const QTextDocument* _document,
                                                    const QRectF& _body,
                                                    const TextTemplate& _template,
                                                    const ExportOptions& _exportOptions) const
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

            const auto paragraphType = TextBlockStyle::forBlock(block);
            QRectF blockRect = layout->blockBoundingRect(block);
            if (q->documentTemplate(_exportOptions).paragraphStyle(paragraphType).lineSpacingType()
                != TextBlockStyle::LineSpacingType::SingleLineSpacing) {
                blockRect.setTop((int)blockRect.top() + block.blockFormat().lineHeight()
                                 - TextHelper::fineLineSpacing(block.charFormat().font()));
            }
            if (blockRect.bottom()
                > pageYPos + _body.height()
                    - MeasurementHelper::mmToPx(
                        q->documentTemplate(_exportOptions).pageMargins().bottom())) {
                break;
            }

            q->printBlockDecorations(_painter, pageYPos, _body, paragraphType, blockRect, block,
                                     _exportOptions);

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
            _painter->setFont(_template.defaultFont());

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

void AbstractPdfExporter::Implementation::printDocument(QTextDocument* _document,
                                                        QPdfWriter* _printer,
                                                        const TextTemplate& _template,
                                                        const ExportOptions& _exportOptions) const
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


// ****


AbstractPdfExporter::AbstractPdfExporter()
    : d(new Implementation(this))
{
}

AbstractPdfExporter::~AbstractPdfExporter() = default;

void AbstractPdfExporter::exportTo(TextModel* _model, ExportOptions& _exportOptions) const
{
    //
    // Настраиваем документ
    //
    QScopedPointer<TextDocument> textDocument(prepareDocument(_model, _exportOptions));

    //
    // Настраиваем принтер
    //
    const auto& exportTemplate = documentTemplate(_exportOptions);
    QPdfWriter printer(_exportOptions.filePath);
    printer.setPageSize(QPageSize(exportTemplate.pageSizeId()));
    printer.setPageMargins({});

    //
    // Допишем параметры сценария в параметры экспорта
    //
    updateExportOptions(_model, _exportOptions);

    //
    // Печатаем документ
    //
    d->printDocument(textDocument.data(), &printer, exportTemplate, _exportOptions);
}

} // namespace BusinessLayer

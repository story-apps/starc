#include "page_metrics.h"

#include <utils/helpers/measurement_helper.h>

#include <QApplication>
#include <QScreen>
#include <QSizeF>
#include <QString>


class PageMetrics::Implementation
{
public:
    /**
     * @brief Формат страницы
     */
    QPageSize::PageSizeId pageFormat;

    /**
     * @brief Размеры в миллиметрах
     */
    QSizeF mmPageSize;
    QMarginsF mmPageMargins;

    /**
     * @brief Размеры в пикселах
     */
    QSizeF pxPageSize;
    QMarginsF pxPageMargins;
};


// ****


PageMetrics::PageMetrics(QPageSize::PageSizeId _pageFormat, const QMarginsF& _mmPageMargins)
    : d(new Implementation)
{
    update(_pageFormat, _mmPageMargins);
}

PageMetrics::~PageMetrics() = default;

void PageMetrics::update(QPageSize::PageSizeId _pageFormat, const QMarginsF& _mmPageMargins,
                         const QMarginsF& _pxPageMargins)
{
    //
    // Должно быть задано только одно из измерений, либо оба пустые
    //
    Q_ASSERT((_mmPageMargins.isNull() && _pxPageMargins.isNull())
             || (_mmPageMargins.isNull() && !_pxPageMargins.isNull())
             || (!_mmPageMargins.isNull() && _pxPageMargins.isNull()));

    d->pageFormat = _pageFormat;

    const bool x = true, y = false;
    d->mmPageSize = QPageSize(d->pageFormat).rect(QPageSize::Millimeter).size();
    d->pxPageSize = QSizeF(MeasurementHelper::mmToPx(d->mmPageSize.width(), x),
                           MeasurementHelper::mmToPx(d->mmPageSize.height(), y));

    if (!_mmPageMargins.isNull()) {
        d->mmPageMargins = _mmPageMargins;
        d->pxPageMargins = QMarginsF(MeasurementHelper::mmToPx(d->mmPageMargins.left(), x),
                                     MeasurementHelper::mmToPx(d->mmPageMargins.top(), y),
                                     MeasurementHelper::mmToPx(d->mmPageMargins.right(), x),
                                     MeasurementHelper::mmToPx(d->mmPageMargins.bottom(), y));
    } else if (!_pxPageMargins.isNull()) {
        d->pxPageMargins = _pxPageMargins;
        d->mmPageMargins = QMarginsF(MeasurementHelper::pxToMm(d->pxPageMargins.left(), x),
                                     MeasurementHelper::pxToMm(d->pxPageMargins.top(), y),
                                     MeasurementHelper::pxToMm(d->pxPageMargins.right(), x),
                                     MeasurementHelper::pxToMm(d->pxPageMargins.bottom(), y));
    } else {
        d->mmPageMargins = {};
        d->pxPageMargins = {};
    }
}

QPageSize::PageSizeId PageMetrics::pageFormat() const
{
    return d->pageFormat;
}

const QSizeF& PageMetrics::mmPageSize() const
{
    return d->mmPageSize;
}

const QMarginsF& PageMetrics::mmPageMargins() const
{
    return d->mmPageMargins;
}

const QSizeF& PageMetrics::pxPageSize() const
{
    return d->pxPageSize;
}

const QMarginsF& PageMetrics::pxPageMargins() const
{
    return d->pxPageMargins;
}

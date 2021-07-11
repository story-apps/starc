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


qreal PageMetrics::mmToPx(qreal _mm, bool _x)
{
    return MeasurementHelper::mmToPx(_mm, _x);
}

qreal PageMetrics::pxToMm(qreal _px, bool _x)
{
    return MeasurementHelper::pxToMm(_px, _x);
}

qreal PageMetrics::ptToPx(qreal _pt, bool _x)
{
    return MeasurementHelper::ptToPx(_pt, _x);
}

qreal PageMetrics::pxToPt(qreal _px, bool _x)
{
    return MeasurementHelper::pxToPt(_px, _x);
}

QPageSize::PageSizeId PageMetrics::pageSizeIdFromString(const QString& _from)
{
    if (_from == "A4") {
        return QPageSize::A4;
    } else if (_from == "Letter") {
        return QPageSize::Letter;
    } else {
        Q_ASSERT_X(0, Q_FUNC_INFO, qPrintable("Undefined page size: " + _from));
        return QPageSize::A4;
    }
}

QString PageMetrics::stringFromPageSizeId(QPageSize::PageSizeId _pageSize)
{
    switch (_pageSize) {
    case QPageSize::A4: {
        return "A4";
    }

    case QPageSize::Letter: {
        return "Letter";
    }

    default: {
        Q_ASSERT_X(0, Q_FUNC_INFO,
                   qPrintable("Undefined page size: " + QString::number(_pageSize)));
        return {};
    }
    }
}

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
    d->pxPageSize = QSizeF(mmToPx(d->mmPageSize.width(), x), mmToPx(d->mmPageSize.height(), y));

    if (!_mmPageMargins.isNull()) {
        d->mmPageMargins = _mmPageMargins;
        d->pxPageMargins
            = QMarginsF(mmToPx(d->mmPageMargins.left(), x), mmToPx(d->mmPageMargins.top(), y),
                        mmToPx(d->mmPageMargins.right(), x), mmToPx(d->mmPageMargins.bottom(), y));
    } else if (!_pxPageMargins.isNull()) {
        d->pxPageMargins = _pxPageMargins;
        d->mmPageMargins
            = QMarginsF(pxToMm(d->pxPageMargins.left(), x), pxToMm(d->pxPageMargins.top(), y),
                        pxToMm(d->pxPageMargins.right(), x), pxToMm(d->pxPageMargins.bottom(), y));
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

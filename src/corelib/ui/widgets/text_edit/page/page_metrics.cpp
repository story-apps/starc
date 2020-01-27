#include "page_metrics.h"

#include <QApplication>
#include <QScreen>


qreal PageMetrics::mmToPx(qreal _mm, bool _x)
{
    static qreal xCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchX();
        const auto pageSize = QPageSize(QPageSize::A4);
        return pageSize.sizePixels(density).width() / pageSize.size(QPageSize::Millimeter).width();
    } ();
    static qreal yCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchY();
        const auto pageSize = QPageSize(QPageSize::A4);
        return pageSize.sizePixels(density).height() / pageSize.size(QPageSize::Millimeter).height();
    } ();

    return _mm * (_x ? xCoefficient : yCoefficient);
}

qreal PageMetrics::pxToMm(qreal _px, bool _x)
{
    return _px / mmToPx(1, _x);
}

qreal PageMetrics::ptToPx(qreal _pt, bool _x)
{
    static qreal xCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchX();
        return 72 * density;
    } ();
    static qreal yCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchY();
        return 72 * density;
    } ();

    return _pt / (_x ? xCoefficient : yCoefficient);
}

qreal PageMetrics::pxToPt(qreal _px, bool _x)
{
    return _px / ptToPx(1, _x);
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
            Q_ASSERT_X(0, Q_FUNC_INFO, qPrintable("Undefined page size: " + QString::number(_pageSize)));
            return {};
        }
    }
}

PageMetrics::PageMetrics(QPageSize::PageSizeId _pageFormat, const QMarginsF& _mmPageMargins)
{
    update(_pageFormat, _mmPageMargins);
}

void PageMetrics::update(QPageSize::PageSizeId _pageFormat, const QMarginsF& _mmPageMargins)
{
    m_pageFormat = _pageFormat;

    m_mmPageSize = QPageSize(m_pageFormat).rect(QPageSize::Millimeter).size();
    m_mmPageMargins = _mmPageMargins;

    //
    // Рассчитываем значения в пикселах
    //
    const bool x = true, y = false;
    m_pxPageSize = QSizeF(mmToPx(m_mmPageSize.width(), x),
                          mmToPx(m_mmPageSize.height(), y));
    m_pxPageMargins = QMarginsF(mmToPx(m_mmPageMargins.left(), x),
                                mmToPx(m_mmPageMargins.top(), y),
                                mmToPx(m_mmPageMargins.right(), x),
                                mmToPx(m_mmPageMargins.bottom(), y));
}

QPageSize::PageSizeId PageMetrics::pageFormat() const
{
    return m_pageFormat;
}

QSizeF PageMetrics::mmPageSize() const
{
    return m_mmPageSize;
}

QMarginsF PageMetrics::mmPageMargins() const
{
    return m_mmPageMargins;
}

QSizeF PageMetrics::pxPageSize() const
{
    return m_pxPageSize;
}

QMarginsF PageMetrics::pxPageMargins() const
{
    return m_pxPageMargins;
}

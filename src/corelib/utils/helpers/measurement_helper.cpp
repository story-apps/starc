#include "measurement_helper.h"

#include <QApplication>
#include <QFontMetricsF>
#include <QPageSize>
#include <QScreen>
#include <QtMath>


qreal MeasurementHelper::mmToPx(qreal _mm, bool _x)
{
#ifndef ACCURATE_METRICS_HANDLING
    static qreal xCoefficient = [] {
        QFont courierFont("Courier New");
        courierFont.setPixelSize(ptToPx(12));
        const QFontMetricsF courierNewMetrics(courierFont);
        const qreal xCoefficient = courierNewMetrics.width("W") / 2.53;
        return xCoefficient;
    }();
    static qreal yCoefficient = [] {
        QFont courierFont("Courier New");
        courierFont.setPixelSize(ptToPx(12));
        const QFontMetricsF courierNewMetrics(courierFont);
        const qreal yCoefficient = courierNewMetrics.lineSpacing() / 4.8;
        return yCoefficient;
    }();
#else
    static qreal xCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchX();
        const auto pageSize = QPageSize(QPageSize::A4);
        return pageSize.sizePixels(density).width() / pageSize.size(QPageSize::Millimeter).width();
    }();
    static qreal yCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchY();
        const auto pageSize = QPageSize(QPageSize::A4);
        return pageSize.sizePixels(density).height()
            / pageSize.size(QPageSize::Millimeter).height();
    }();
#endif
    return _mm * (_x ? xCoefficient : yCoefficient);
}

qreal MeasurementHelper::pxToMm(qreal _px, bool _x)
{
    return _px / mmToPx(1, _x);
}

qreal MeasurementHelper::inchToPx(qreal _inch, bool _x)
{
    return mmToPx(inchToMm(_inch), _x);
}

qreal MeasurementHelper::pxToInch(qreal _px, bool _x)
{
    return mmToInch(pxToMm(_px, _x));
}

qreal MeasurementHelper::ptToPx(qreal _pt, bool _x)
{
    static qreal xCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchX();
        return 72 / density;
    }();
    static qreal yCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchY();
        return 72 / density;
    }();

    return _pt / (_x ? xCoefficient : yCoefficient);
}

int MeasurementHelper::pxToPt(qreal _px, bool _x)
{
    return qCeil(_px / ptToPx(1, _x));
}

qreal MeasurementHelper::mmToInch(qreal _mm)
{
    return _mm / 25.4;
}

qreal MeasurementHelper::inchToMm(qreal _inch)
{
    return _inch * 25.4;
}

int MeasurementHelper::mmToTwips(qreal _mm)
{
    return qCeil(56.692913386 * _mm);
}

int MeasurementHelper::pxToTwips(qreal _px, bool _x)
{
    return mmToTwips(pxToMm(_px, _x));
}

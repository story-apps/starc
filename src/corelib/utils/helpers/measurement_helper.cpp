#include "measurement_helper.h"

#include <QApplication>
#include <QScreen>
#include <QPageSize>
#include <QtMath>


qreal MeasurementHelper::mmToPx(qreal _mm, bool _x)
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

qreal MeasurementHelper::pxToMm(qreal _px, bool _x)
{
    return _px / mmToPx(1, _x);
}

qreal MeasurementHelper::ptToPx(qreal _pt, bool _x)
{
    static qreal xCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchX();
        return 72 / density;
    } ();
    static qreal yCoefficient = [] {
        const auto density = QApplication::primaryScreen()->physicalDotsPerInchY();
        return 72 / density;
    } ();

    return _pt / (_x ? xCoefficient : yCoefficient);
}

int MeasurementHelper::pxToPt(qreal _px, bool _x)
{
    return qCeil(_px / ptToPx(1, _x));
}

#include "measurement_helper.h"

#include <QtMath>


qreal MeasurementHelper::mmToPx(qreal _mm, bool _x)
{
    Q_UNUSED(_x)
    return _mm * kDocumentDpi / 25.4;
}

qreal MeasurementHelper::pxToMm(qreal _px, bool _x)
{
    Q_UNUSED(_x)
    return _px * 25.4 / kDocumentDpi;
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
    Q_UNUSED(_x)
    return _pt * kDocumentDpi / 72.0;
}

qreal MeasurementHelper::pxToPt(qreal _px, bool _x)
{
    Q_UNUSED(_x)
    return _px * 72.0 / kDocumentDpi;
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
    return qRound(_mm * 1440.0 / 25.4);
}

int MeasurementHelper::pxToTwips(qreal _px, bool _x)
{
    return qRound(pxToPt(_px, _x) * 20.0);
}

#include "measurement_helper.h"
#include "document_units.h"

#include <QtMath>

#include <cmath>

namespace {

static QFont trueCourier()
{
    QFont courierNewFont("Courier New");
    courierNewFont.setPixelSize(MeasurementHelper::ptToPx(12));
#ifndef Q_OS_WIN
    return courierNewFont;
#else
    QFont courierPrimeFont("Courier Prime");
    courierPrimeFont.setPixelSize(MeasurementHelper::ptToPx(12));

    //
    // Проверяем всё ли ок с системным Courier New, на некоторых виндах проявляется такая проблема,
    // что система вместо курьер нью отдаёт ариал и автор видит херню на экране
    //
    if (abs(QFontMetricsF(courierNewFont).horizontalAdvance("W")
            - QFontMetricsF(courierPrimeFont).horizontalAdvance("W"))
        < 1.0) {
        //
        // ... тут всё окей
        //
        return courierNewFont;
    } else {
        //
        // ... тут херня, пишем об этом в лог и будем считать за эталон Courier Prime
        //
        qCritical("Courier New failed to load for this system.");
        return courierPrimeFont;
    }
#endif
}

} // namespace


void MeasurementHelper::setAccurateMetricsHandling(bool _accurate)
{
    m_accurateMetricsHandling = _accurate;
}

qreal MeasurementHelper::mmToPx(qreal mm, bool x)
{
    Q_UNUSED(x)
    return DocumentUnits::mmToDu(mm);
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

qreal MeasurementHelper::ptToPx(qreal pt, bool x)
{
    Q_UNUSED(x)
    return DocumentUnits::ptToDu(pt);
}

int MeasurementHelper::pxToPt(qreal _px, bool _x)
{
    return qCeil(_px / ptToPx(1, _x));
}

qreal MeasurementHelper::mmToInch(qreal _mm)
{
    return _mm / DocumentUnits::MmPerInch;
}

qreal MeasurementHelper::inchToMm(qreal _inch)
{
    return _inch * DocumentUnits::MmPerInch;
}

int MeasurementHelper::mmToTwips(qreal _mm)
{
    return qCeil(56.692913386 * _mm);
}

int MeasurementHelper::pxToTwips(qreal _px, bool _x)
{
    return mmToTwips(pxToMm(_px, _x));
}

bool MeasurementHelper::m_accurateMetricsHandling = false;

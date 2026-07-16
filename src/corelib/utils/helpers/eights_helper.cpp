#include "eights_helper.h"

#include "time_helper.h"

#include <QCoreApplication>


QString EightsHelper::toString(qreal _eights)
{
    int eights = _eights;
    if (_eights - static_cast<qreal>(eights) > 0.2) {
        ++eights;
    }

    constexpr int eight = 8;
    if (eights < eight) {
        return QString("%1/%2").arg(eights).arg(eight);
    }

    return QString("%1 %2/%3").arg(eights / eight).arg(eights % eight).arg(eight);
}

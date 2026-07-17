#include "eights_helper.h"

#include "time_helper.h"

#include <QCoreApplication>


QString EightsHelper::toString(qreal _eights, bool _withPostfix)
{
    int eights = _eights;
    if (_eights - static_cast<qreal>(eights) > 0.2) {
        ++eights;
    }

    constexpr int eight = 8;
    if (eights < eight) {
        return QString("%1/%2%3").arg(eights).arg(eight).arg(
            _withPostfix ? " "
                    //: Page in pages scene lenght when lenght less then 1 page (1/8 ... 7/8)
                    + QObject::tr("page", "EightsHelper")
                         : "");
    }

    if (eights % eight == 0) {
        if (_withPostfix) {
            //: Page in pages scene lenght when lenght equal full number of pages (1 ... 99)
            return QObject::tr("%n pages", "EightsHelper", eights / eight);
        }

        return QString("%1").arg(eights / eight);
    }

    return QString("%1 %2/%3%4")
        .arg(eights / eight)
        .arg(eights % eight)
        .arg(eight)
        .arg(_withPostfix ? " "
                     //: Pages in pages scene lenght when lenght more then 1 page (1 1/8 ... 99 7/8)
                     + QObject::tr("pages", "EightsHelper")
                          : "");
}

QString EightsHelper::toStringWithPostfix(qreal _eights)
{
    return toString(_eights, true);
}

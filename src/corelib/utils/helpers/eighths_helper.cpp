#include "eighths_helper.h"

#include "time_helper.h"

#include <QCoreApplication>


QString EighthsHelper::toString(qreal _eighths, bool _withPostfix)
{
    int eighths = _eighths;
    if (_eighths - static_cast<qreal>(eighths) > 0.2) {
        ++eighths;
    }

    constexpr int eight = 8;
    if (eighths < eight) {
        return QString("%1/%2%3").arg(eighths).arg(eight).arg(
            _withPostfix ? " "
                    //: Page in pages scene lenght when lenght less then 1 page (1/8 ... 7/8)
                    + QObject::tr("page", "EightsHelper")
                         : "");
    }

    if (eighths % eight == 0) {
        if (_withPostfix) {
            //: Page in pages scene lenght when lenght equal full number of pages (1 ... 99)
            return QObject::tr("%n pages", "EightsHelper", eighths / eight);
        }

        return QString("%1").arg(eighths / eight);
    }

    return QString("%1 %2/%3%4")
        .arg(eighths / eight)
        .arg(eighths % eight)
        .arg(eight)
        .arg(_withPostfix ? " "
                     //: Pages in pages scene lenght when lenght more then 1 page (1 1/8 ... 99 7/8)
                     + QObject::tr("pages", "EightsHelper")
                          : "");
}

QString EighthsHelper::toStringWithPostfix(qreal _eighths)
{
    return toString(_eighths, true);
}

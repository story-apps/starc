#include "eighths_helper.h"

#include "time_helper.h"

#include <QCoreApplication>


QString EighthsHelper::toString(qreal _eighths, bool _withPostfix)
{
    int eighths = _eighths;
    if (_eighths - static_cast<qreal>(eighths) > 0.2) {
        ++eighths;
    }

    //: Page in pages scene duration
    const auto postfix = _withPostfix ? " " + QObject::tr("p.", "EightsHelper") : "";

    constexpr int eight = 8;
    if (eighths < eight) {
        return QString("%1/%2%3").arg(eighths).arg(eight).arg(postfix);
    }

    if (eighths % eight == 0) {
        return QString("%1%2").arg(eighths / eight).arg(postfix);
    }

    return QString("%1 %2/%3%4").arg(eighths / eight).arg(eighths % eight).arg(eight).arg(postfix);
}

QString EighthsHelper::toStringWithPostfix(qreal _eighths)
{
    return toString(_eighths, true);
}

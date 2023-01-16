#include "time_helper.h"

#include <QCoreApplication>


QString TimeHelper::toString(std::chrono::seconds _seconds)
{
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(_seconds);
    const auto seconds = _seconds - minutes;
    return QString("%1:%2").arg(minutes.count()).arg(QString("0%1").arg(seconds.count()).right(2));
}

QString TimeHelper::toString(std::chrono::milliseconds _milliseconds)
{
    return toString(std::chrono::duration_cast<std::chrono::seconds>(_milliseconds));
}

QString TimeHelper::toLongString(std::chrono::seconds _seconds)
{
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(_seconds);
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(_seconds - hours);
    if (hours.count() == 0) {
        return QCoreApplication::translate("TimeHelper", "%1min").arg(minutes.count());
    }
    return QCoreApplication::translate("TimeHelper", "%1h %2min")
        .arg(hours.count())
        .arg(QString("0%1").arg(minutes.count()).right(2));
}

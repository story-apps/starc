#include "time_helper.h"

#include <QCoreApplication>


QString TimeHelper::toString(std::chrono::seconds _seconds)
{
    const auto hours = std::chrono::duration_cast<std::chrono::hours>(_seconds);
    const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(_seconds) - hours;
    const auto seconds = _seconds - minutes;
    const auto hoursText = hours.count() > 0 ? QString("%1:").arg(hours.count()) : "";
    const auto minutesText = hoursText.isEmpty() ? QString::number(minutes.count())
                                                 : QString("0%1").arg(minutes.count()).right(2);
    const auto secondsText = QString("0%1").arg(seconds.count()).right(2);
    return QString("%1%2:%3").arg(hoursText, minutesText, secondsText);
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

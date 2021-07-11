#include "time_helper.h"

#include <QString>


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

#pragma once

#include <QDateTime>


namespace Domain {

/**
 * @brief Информация о сессии
 */
struct SessionInfo {
    QString sessionKey;
    QString deviceName;
    QString location;
    QDateTime lastUsed;
    bool isCurrentDevice = false;
};

} // namespace Domain

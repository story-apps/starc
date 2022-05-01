#pragma once

#include <QDateTime>


namespace Domain {

/**
 * @brief Тип уведомления
 */
enum class NotificationType {
    Undefined,
    UpdateDevLinux,
    UpdateDevMac,
    UpdateDevWindows32,
    UpdateDevWindows64,
    UpdateStableLinux,
    UpdateStableMac,
    UpdateStableWindows32,
    UpdateStableWindows64,
};

/**
 * @brief Уведомление о некоем событии с сервера
 */
struct Notification {
    NotificationType type;
    QString notification;
    QDateTime dateTime;
};

} // namespace Domain

#include "notifications_manager.h"

#include <domain/notification.h>
#include <utils/shugar.h>

#include <QSettings>


namespace {
const char* kLastReadNotificationDateTimeKey
    = "/managers/notifications/last-read-notification-datetime";
}

namespace ManagementLayer {

class NotificationsManager::Implementation
{
public:
    QVector<Domain::Notification> notifications;
};


// ****


NotificationsManager::NotificationsManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

NotificationsManager::~NotificationsManager() = default;

void NotificationsManager::processNotifications(const QVector<Domain::Notification>& _notifications)
{
    const QVector<Domain::NotificationType> irrelevantNotificationTypes =
#if defined(Q_OS_LINUX)
    { Domain::NotificationType::UpdateDevMac,
      Domain::NotificationType::UpdateDevWindows32,
      Domain::NotificationType::UpdateDevWindows64,
      Domain::NotificationType::UpdateStableMac,
      Domain::NotificationType::UpdateStableWindows32,
      Domain::NotificationType::UpdateStableWindows64,
    }
#elif defined(Q_OS_MACOS)
    { Domain::NotificationType::UpdateDevLinux,
      Domain::NotificationType::UpdateDevWindows32,
      Domain::NotificationType::UpdateDevWindows64,
      Domain::NotificationType::UpdateStableLinux,
      Domain::NotificationType::UpdateStableWindows32,
      Domain::NotificationType::UpdateStableWindows64,
    }
#elif defined(Q_OS_WIN64)
    { Domain::NotificationType::UpdateDevLinux,
      Domain::NotificationType::UpdateDevMac,
      Domain::NotificationType::UpdateDevWindows32,
      Domain::NotificationType::UpdateStableLinux,
      Domain::NotificationType::UpdateStableMac,
      Domain::NotificationType::UpdateStableWindows32,
    }
#elif defined(Q_OS_WIN32)
    { Domain::NotificationType::UpdateDevLinux,
      Domain::NotificationType::UpdateDevMac,
      Domain::NotificationType::UpdateDevWindows64,
      Domain::NotificationType::UpdateStableLinux,
      Domain::NotificationType::UpdateStableMac,
      Domain::NotificationType::UpdateStableWindows64,
    }
#endif
    ;

    const auto lastReadNotificationDateTime
        = QSettings().value(kLastReadNotificationDateTimeKey).toDateTime();
    bool hasUnreadNotifications = false;
    for (const auto& notification : reversed(_notifications)) {
        //
        // Пропускаем уведомления нерелевантные для текущего устройства
        //
        if (irrelevantNotificationTypes.contains(notification.type)) {
            continue;
        }

        //
        // Пропускаем уведомления, которые уже были обработаны
        //
        if (d->notifications.contains(notification)) {
            continue;
        }

        d->notifications.prepend(notification);

        if (!lastReadNotificationDateTime.isValid()
            || lastReadNotificationDateTime < notification.dateTime) {
            hasUnreadNotifications = true;
        }
    }

    emit showNotificationsRequested(d->notifications);
    emit hasUnreadNotificationsChanged(hasUnreadNotifications);
}

void NotificationsManager::markAllRead()
{
    if (d->notifications.isEmpty()) {
        return;
    }

    QSettings().setValue(kLastReadNotificationDateTimeKey, d->notifications.first().dateTime);
    emit hasUnreadNotificationsChanged(false);
}

} // namespace ManagementLayer

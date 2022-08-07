#include "notifications_manager.h"

#include <domain/starcloud_api.h>
#include <utils/shugar.h>

#include <QSettings>


namespace {
const char* kShowDevVersionsKey = "/managers/notifications/show-dev-versions";
const char* kLastReadNotificationDateTimeKey
    = "/managers/notifications/last-read-notification-datetime";
} // namespace

namespace ManagementLayer {

class NotificationsManager::Implementation
{
public:
    bool showDevVersions = false;
    QVector<Domain::Notification> notifications;
};


// ****


NotificationsManager::NotificationsManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    d->showDevVersions = QSettings().value(kShowDevVersionsKey).toBool();
}

NotificationsManager::~NotificationsManager() = default;

bool NotificationsManager::showDevversions() const
{
    return d->showDevVersions;
}

void NotificationsManager::setShowDevVersions(bool _show)
{
    if (d->showDevVersions == _show) {
        return;
    }

    d->showDevVersions = _show;
    QSettings().setValue(kShowDevVersionsKey, d->showDevVersions);
    processNotifications({});
}

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
    }

    QVector<Domain::Notification> notificationsToShow;
    for (const auto& notification : std::as_const(d->notifications)) {
        //
        // Пропускаем уведомления о дев-версиях, если они отключены
        //
        if (!d->showDevVersions
            && (notification.type == Domain::NotificationType::UpdateDevLinux
                || notification.type == Domain::NotificationType::UpdateDevMac
                || notification.type == Domain::NotificationType::UpdateDevWindows32
                || notification.type == Domain::NotificationType::UpdateDevWindows64)) {
            continue;
        }

        notificationsToShow.append(notification);

        if (!lastReadNotificationDateTime.isValid()
            || lastReadNotificationDateTime < notification.dateTime) {
            hasUnreadNotifications = true;
        }
    }

    emit showNotificationsRequested(notificationsToShow);
    emit hasUnreadNotificationsChanged(hasUnreadNotifications);
}

void NotificationsManager::markAllRead()
{
    if (d->notifications.isEmpty()) {
        return;
    }

    const auto lastReadNotificationDateTime
        = QSettings().value(kLastReadNotificationDateTimeKey).toDateTime();
    if (lastReadNotificationDateTime.isValid()
        && lastReadNotificationDateTime >= d->notifications.constFirst().dateTime) {
        return;
    }

    QSettings().setValue(kLastReadNotificationDateTimeKey, d->notifications.constFirst().dateTime);
    emit hasUnreadNotificationsChanged(false);
}

} // namespace ManagementLayer

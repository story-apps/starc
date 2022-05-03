#pragma once

#include <QObject>

namespace Domain {
struct Notification;
}


namespace ManagementLayer {

/**
 * @brief Менеджер уведомлений
 */
class NotificationsManager : public QObject
{
    Q_OBJECT

public:
    explicit NotificationsManager(QObject* _parent = nullptr);
    ~NotificationsManager() override;

    /**
     * @brief Обработать уведомления поступившие с сервера
     */
    void processNotifications(const QVector<Domain::Notification>& _notifications);

    /**
     * @brief Пометить все уведомления просмотренными
     */
    void markAllRead();

signals:
    /**
     * @brief Нужно отобразить заданный список уведомлений
     */
    void showNotificationsRequested(const QVector<Domain::Notification>& _notifications);

    /**
     * @brief Есть ли непрочитанные уведомления
     */
    void hasUnreadNotificationsChanged(bool _has);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

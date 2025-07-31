#pragma once

#include <domain/starcloud_api.h>

#include <QObject>


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
     * @brief Необходимо ли отображения дев-версий
     */
    bool showDevVersions() const;
    void setShowDevVersions(bool _show);

    /**
     * @brief Обработать уведомления поступившие с сервера
     */
    void processNotifications(const QVector<Domain::Notification>& _notifications);

    /**
     * @brief Пометить все уведомления просмотренными
     */
    void markAllRead();

    /**
     * @brief Ссылка на загрузку самой последней версии
     */
    QString lastVersionDownloadLink() const;

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

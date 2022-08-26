#pragma once

#include <QColor>
#include <QDateTime>
#include <QUuid>


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
 * @brief Тип подписки
 */
enum class SubscriptionType {
    Free = 0,
    ProMonthly = 1000,
    ProLifetime = 1100,
    TeamMonthly = 2000,
    TeamLifetime = 2100,
    Corporate = 10000,
};

/**
 * @brief Длительность подписки
 */
enum class PaymentDuration {
    Lifetime = 0,
    Month1 = 1,
    Month2 = 2,
    Month3 = 3,
    Month6 = 6,
    Month12 = 12,
    Custom = 1000,
};


/**
 * @brief Уведомление о некоем событии с сервера
 */
struct Notification {
    NotificationType type;
    QString notification;
    QDateTime dateTime;
};

inline bool operator==(const Notification& _lhs, const Notification& _rhs)
{
    return _lhs.type == _rhs.type && _lhs.notification == _rhs.notification
        && _lhs.dateTime == _rhs.dateTime;
}

/**
 * @brief Параметры оплат
 */
struct PaymentOption {
    /**
     * @brief Валидны ли параметры оплаты
     */
    bool isValid() const
    {
        return amount != -1;
    }

    /**
     * @brief Стоимость, в центах
     */
    int amount = -1;

    /**
     * @brief Стоимость со скидкой, в центах
     */
    int totalAmount = -1;

    /**
     * @brief Тип подписки
     */
    Domain::SubscriptionType subscriptionType;

    /**
     * @brief Дата окончания подписки
     */
    QDateTime subscriptionEnds;

    /**
     * @brief Флаг длительности подписки, для формирования текстов на клиенте
     */
    PaymentDuration duration;
};

/**
 * @brief Информация о подписке
 */
struct SubscriptionInfo {
    SubscriptionType type = SubscriptionType::Free;
    QDateTime end;
};

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

/**
 * @brief Информация об аккаунте
 */
struct AccountInfo {
    QString email;
    QString name;
    QString description;
    bool newsletterSubscribed = false;
    QString newsletterLanguage;
    QByteArray avatar;
    quint64 cloudStorageSizeUsed = 0;
    quint64 cloudStorageSize = 0;
    QVector<SubscriptionInfo> subscriptions;
    QVector<PaymentOption> paymentOptions;
    QVector<SessionInfo> sessions;
};

/**
 * @brief Соавторы проекта
 */
struct ProjectCollaboratorInfo {
    QString name;
    QString email;
    int role = 0;
    QColor color;
};

inline bool operator==(const ProjectCollaboratorInfo& _lhs, const ProjectCollaboratorInfo& _rhs)
{
    return _lhs.name == _rhs.name && _lhs.email == _rhs.email && _lhs.role == _rhs.role
        && _lhs.color == _rhs.color;
}

/**
 * @brief Информация о проекте
 */
struct ProjectInfo {
    int id = 0;
    QString name;
    QString logline;
    QByteArray poster;
    int accountRole = 0;
    QDateTime lastEditTime;
    QVector<ProjectCollaboratorInfo> collaborators;
};

/**
 * @brief Информация об изменении документа
 */
struct DocumentChangeInfo {
    QUuid uuid;
    QByteArray redoPatch;
    QDateTime changedAt;
};

/**
 * @brief Информация о документе
 */
struct DocumentInfo {
    QUuid uuid;
    int type = 0;
    QByteArray content;
    QDateTime changedAt;
    QVector<DocumentChangeInfo> changes;
};

} // namespace Domain

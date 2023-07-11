#pragma once

#include <QColor>
#include <QDateTime>
#include <QUuid>


namespace Domain {

const int kInvalidId = -1;

/**
 * @brief Тип уведомления
 */
enum class NotificationType {
    Undefined,
    //
    UpdateDevLinux,
    UpdateDevMac,
    UpdateDevWindows32,
    UpdateDevWindows64,
    UpdateStableLinux,
    UpdateStableMac,
    UpdateStableWindows32,
    UpdateStableWindows64,
    //
    ProSubscriptionEnds,
    TeamSubscriptionEnds,
    //
    CreditsAdded,
};

/**
 * @brief Тип подписки
 */
enum class SubscriptionType {
    Undefined = -1,
    Free = 0,
    ProMonthly = 1000,
    ProLifetime = 1100,
    CloudMonthly = 2000,
    CloudLifetime = 2100,
    Studio = 10000,

    //
    // Подписки на конкретные модули (для коммерческих клиентов и для внутреннего использования)
    //
    Features = 100000,
    ScreenplayBreakdownFeature = 100001,
};

/**
 * @brief Тип платежа
 */
enum class PaymentType {
    Subscription = 0,
    Credits = 1,
    Merchandize = 1000,
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
     * @brief Скидка в процентах
     */
    int discount = -1;

    /**
     * @brief Стоимость со скидкой, в центах
     */
    int totalAmount = -1;

    /**
     * @brief Тип платежа
     */
    Domain::PaymentType type;

    /**
     * @brief Флаг длительности подписки, для формирования текстов на клиенте
     */
    PaymentDuration duration;

    //
    // Параметры подписки
    //

    /**
     * @brief Тип подписки
     */
    Domain::SubscriptionType subscriptionType;

    /**
     * @brief Дата окончания подписки
     */
    QDateTime subscriptionEnds;

    //
    // Параметры кредитов
    //

    /**
     * @brief Количество кредитов
     */
    int credits = 0;
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
    int credits = 0;
    QVector<PaymentOption> paymentOptions;
    QVector<SessionInfo> sessions;

    bool isValid() const
    {
        return !email.isEmpty();
    }
};

/**
 * @brief Участник команды
 */
struct TeamMemberInfo {
    QString name;
    QString email;
    int role = 0;
    bool hasAccessToAllProjects = true;
    bool allowGrantAccessToProjects = true;
    QString nameForTeam;
    QColor color;
};

inline bool operator==(const TeamMemberInfo& _lhs, const TeamMemberInfo& _rhs)
{
    return _lhs.name == _rhs.name && _lhs.email == _rhs.email && _lhs.role == _rhs.role
        && _lhs.hasAccessToAllProjects == _rhs.hasAccessToAllProjects
        && _lhs.allowGrantAccessToProjects == _rhs.allowGrantAccessToProjects
        && _lhs.nameForTeam == _rhs.nameForTeam && _lhs.color == _rhs.color;
}

/**
 * @brief Информация о команде
 */
struct TeamInfo {
    int id = kInvalidId;
    QString name;
    QString description;
    QByteArray avatar;
    int teamRole = 0;
    bool hasAccessToAllProjects = true;
    bool allowGrantAccessToProjects = true;
    QString nameForTeam;
    QColor color;
    QVector<TeamMemberInfo> members;

    bool isValid() const
    {
        return id != kInvalidId;
    }
};

/**
 * @brief Соавторы проекта
 */
struct ProjectCollaboratorInfo {
    QString name;
    QString email;
    int role = 0;
    QColor color;
    QHash<QUuid, int> permissions;
};

inline bool operator==(const ProjectCollaboratorInfo& _lhs, const ProjectCollaboratorInfo& _rhs)
{
    return _lhs.name == _rhs.name && _lhs.email == _rhs.email && _lhs.role == _rhs.role
        && _lhs.color == _rhs.color && _lhs.permissions == _rhs.permissions;
}

/**
 * @brief Информация о проекте
 */
struct ProjectInfo {
    int id = kInvalidId;
    QString name;
    QString logline;
    QByteArray poster;
    int accountRole = 0;
    QHash<QUuid, int> accountPermissions;
    QDateTime lastEditTime;
    QVector<ProjectCollaboratorInfo> collaborators;
    int teamId = kInvalidId;
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

/**
 * @brief Информация о положении курсора соавтора
 */
struct CursorInfo {
    QString cursorId;
    QString email;
    QString name;
    QByteArray cursorData;
    QDateTime updatedAt;
};

/**
 * @brief Статистика по сессиям
 */
struct SessionStatistics {
    QUuid uuid;
    QUuid projectUuid;
    QString projectName;
    QString deviceUuid;
    QString deviceName;
    QDateTime startDateTime;
    QDateTime endDateTime;
    int words = 0;
    int characters = 0;
};

} // namespace Domain

#pragma once

#include "payment_info.h"
#include "session_info.h"
#include "subscription_info.h"

#include <QDateTime>
#include <QVector>


namespace Domain {

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

} // namespace Domain

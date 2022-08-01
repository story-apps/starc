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
    Domain::SubscriptionType subscriptionType;
    QDateTime subscriptionEnds;
    QVector<Domain::PaymentOption> paymentOptions;
    QVector<Domain::SessionInfo> sessions;
};

} // namespace Domain

#pragma once

#include <QDateTime>


namespace Domain {

enum class SubscriptionType;

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

} // namespace Domain

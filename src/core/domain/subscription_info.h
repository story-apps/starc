#pragma once


namespace Domain {

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

} // namespace Domain

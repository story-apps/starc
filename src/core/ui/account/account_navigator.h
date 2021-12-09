#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
enum class SubscriptionType;
struct PaymentOption;
} // namespace Domain


namespace Ui {

/**
 * @brief Навигатор личного кабинета
 */
class AccountNavigator : public Widget
{
    Q_OBJECT

public:
    explicit AccountNavigator(QWidget* _parent = nullptr);
    ~AccountNavigator() override;

    /**
     * @brief Задать информацию о подписке
     */
    void setSubscriptionInfo(Domain::SubscriptionType _subscriptionType,
                             const QDateTime& _subscriptionEnds,
                             const QVector<Domain::PaymentOption>& _paymentOptions);

signals:
    /**
     * @brief Пользователь хочет перейти в отображению заданных настроек
     */
    void accountPressed();
    void subscriptionPressed();
    void sessionsPressed();

    /**
     * @brief Пользователь хочет проапгрейдить аккаунт
     */
    void upgradeToProPressed();

    /**
     * @brief Пользователь хочет выйти из личного кабинета
     */
    void logoutPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

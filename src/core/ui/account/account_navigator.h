#pragma once

#include <ui/widgets/widget/widget.h>


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
     * @brief Задать дату окончания подписки
     */
    void setSubscriptionEnd(const QString& _subscriptionEnd);

signals:
    /**
     * @brief Пользователь хочет купить лицензию
     */
    void upgradeToProPressed();

    /**
     * @brief Пользователь хочет продлить подписку
     */
    void renewSubscriptionPressed();

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

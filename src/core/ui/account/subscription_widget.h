#pragma once

#include <ui/widgets/card/card.h>

namespace Domain {
struct PaymentOption;
}


namespace Ui {

/**
 * @brief Виджет карточки подписки в кабинете
 */
class SubscriptionWidget : public Card
{
    Q_OBJECT

public:
    explicit SubscriptionWidget(QWidget* _parent = nullptr);
    ~SubscriptionWidget() override;

    /**
     * @brief Задать информацию о подписке со всеми доступными опциями для покупки
     */
    void setInfo(const QString& _name, const QString& _description);

    /**
     * @brief Задать статус активности подписки
     */
    void setStatus(bool _isActive, bool _isLifetime, const QDateTime& _activeUntil);

    /**
     * @brief Куплена ли подписка навсегда
     */
    bool isLifetime() const;

    /**
     * @brief Задать опции покупки для подписки
     */
    void setPaymentOptions(const QVector<Domain::PaymentOption>& _paymentOptions);

signals:
    /**
     * @brief Нажата кнопка попробовать
     */
    void tryPressed();

    /**
     * @brief Нажата кнопка купить/продлить
     */
    void buyPressed();

    /**
     * @brief Нажата кнопка купить навсегда
     */
    void buyLifetimePressed();

    /**
     * @brief Нажата кнопка подарить
     */
    void giftPressed();

protected:
    /**
     * @brief Обновляем переводы
     */
    void updateTranslations() override;

    /**
     * @brief Переопределяем для настройки отступов лейаута
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

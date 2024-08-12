#pragma once

#include <ui/widgets/card/card.h>


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

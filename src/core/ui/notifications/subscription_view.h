#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct Notification;
}


namespace Ui {

/**
 * @brief Виджет с информацией об истекающем сроке подписки
 */
class SubscriptionView : public Widget
{
    Q_OBJECT

public:
    explicit SubscriptionView(QWidget* _parent, const Domain::Notification& _notification);
    ~SubscriptionView() override;

signals:
    /**
     * @brief Пользователь нажал кнопку продления PRO версии
     */
    void renewProPressed();

    /**
     * @brief Пользователь нажал кнопку продления CREATOR версии
     */
    void renewCreatorPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Настроить внешний вид
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Ui

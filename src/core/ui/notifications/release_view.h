#pragma once

#include <ui/widgets/widget/widget.h>

namespace Domain {
struct Notification;
}


namespace Ui {

/**
 * @brief Виджет с информацией об обновлении
 */
class ReleaseView : public Widget
{
    Q_OBJECT

public:
    explicit ReleaseView(QWidget* _parent = nullptr);
    ~ReleaseView() override;

    /**
     * @brief Задать уведомление для отображения
     */
    void setNotification(const Domain::Notification& _notification);

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

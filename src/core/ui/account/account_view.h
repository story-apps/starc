#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Представление с данными личного кабинета
 */
class AccountView : public Widget
{
    Q_OBJECT

public:
    explicit AccountView(QWidget* _parent = nullptr);
    ~AccountView() override;

protected:
    /**
     * @brief Переопределяем для корректировки положения тулбара действий над проектами
     */
    void resizeEvent(QResizeEvent* _event) override;

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


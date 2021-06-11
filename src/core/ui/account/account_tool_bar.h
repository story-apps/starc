#pragma once

#include <ui/widgets/app_bar/app_bar.h>


namespace Ui {

/**
 * @brief Панель инструментов личного кабинета
 */
class AccountToolBar : public AppBar
{
    Q_OBJECT

public:
    explicit AccountToolBar(QWidget* _parent = nullptr);

signals:
    /**
     * @brief Пользователь хочет выйти из личного кабинета
     */
    void backPressed();

protected:
    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;
};

} // namespace Ui

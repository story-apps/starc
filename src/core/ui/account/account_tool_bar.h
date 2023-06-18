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

    /**
     * @brief Пользователь переключился в режим аккаунта
     */
    void accountPressed();

    /**
     * @brief Пользователь переключился в режим команд
     */
    void teamPressed();

protected:
    /**
     * @brief Обновить переводы
     */
    void updateTranslations() override;

    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;
};

} // namespace Ui

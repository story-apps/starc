#pragma once

#include <ui/widgets/app_bar/app_bar.h>


namespace Ui {

/**
 * @brief Панель инструментов настроек
 */
class SettingsToolBar : public AppBar
{
    Q_OBJECT

public:
    explicit SettingsToolBar(QWidget* _parent = nullptr);

signals:
    /**
     * @brief Пользователь хочет выйти из личного кабинета
     */
    void backPressed();

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

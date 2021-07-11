#pragma once

#include <ui/widgets/app_bar/app_bar.h>


namespace Ui {

/**
 * @brief Панель инструментов списка проектов
 */
class ProjectsToolBar : public AppBar
{
    Q_OBJECT

public:
    explicit ProjectsToolBar(QWidget* _parent = nullptr);

signals:
    /**
     * @brief Пользователь хочет открыть меню
     */
    void menuPressed();

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

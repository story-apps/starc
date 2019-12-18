#pragma once

#include <ui/widgets/app_bar/app_bar.h>


namespace Ui
{

/**
 * @brief Панель инструментов проекта
 */
class ProjectToolBar : public AppBar
{
    Q_OBJECT
public:
    explicit ProjectToolBar(QWidget* _parent = nullptr);

signals:
    /**
     * @brief Пользователь хочет открыть меню
     */
    void menuPressed();

protected:
    /**
     * @brief Обновляем виджет при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;
};

} // namespace Ui

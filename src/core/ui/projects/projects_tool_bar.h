#pragma once

#include <ui/widgets/widget/widget.h>


namespace Ui
{

/**
 * @brief Панель инструментов списка проектов
 */
class ProjectsToolBar : public Widget
{
    Q_OBJECT

public:
    explicit ProjectsToolBar(QWidget* _parent = nullptr);

protected:
    /**
     * @brief Обновляем навигатор при изменении дизайн системы
     */
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;
};

} // namespace Ui

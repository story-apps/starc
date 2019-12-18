#pragma once

#include <ui/widgets/stack_widget/stack_widget.h>

namespace ManagementLayer {
    class Project;
    class ProjectsModel;
}


namespace Ui
{

/**
 * @brief Представление проекта
 */
class ProjectView : public Widget
{
    Q_OBJECT
public:
    explicit ProjectView(QWidget* _parent = nullptr);

signals:

public slots:
};

} // namespace Ui

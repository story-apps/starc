#include "projects_view.h"

#include <ui/design_system/design_system.h>


namespace Ui
{

ProjectsView::ProjectsView(QWidget* _parent)
    : StackWidget(_parent)
{
    designSystemChangeEvent(nullptr);
}

void ProjectsView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setBackgroundColor(DesignSystem::color().surface());
}

} // namespace Ui

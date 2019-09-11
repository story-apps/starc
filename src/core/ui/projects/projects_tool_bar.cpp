#include "projects_tool_bar.h"

#include <ui/design_system/design_system.h>


namespace Ui
{

ProjectsToolBar::ProjectsToolBar(QWidget* _parent)
    : Widget(_parent)
{
    designSystemChangeEvent(nullptr);
}

void ProjectsToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setBackgroundColor(DesignSystem::color().primary());
}

} // namespace Ui

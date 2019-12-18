#include "project_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>


namespace Ui
{

ProjectToolBar::ProjectToolBar(QWidget* _parent)
    : AppBar(_parent)
{
    QAction* menuAction = new QAction(this);
    menuAction->setText("\uf35c");
    addAction(menuAction);
    connect(menuAction, &QAction::triggered, this, &ProjectToolBar::menuPressed);

    designSystemChangeEvent(nullptr);
}

void ProjectToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

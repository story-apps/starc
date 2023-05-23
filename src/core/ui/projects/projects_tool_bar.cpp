#include "projects_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>


namespace Ui {

ProjectsToolBar::ProjectsToolBar(QWidget* _parent)
    : AppBar(_parent)
{
    QAction* menuAction = new QAction(this);
    menuAction->setText(u8"\U000f035c");
    addAction(menuAction);
    connect(menuAction, &QAction::triggered, this, &ProjectsToolBar::menuPressed);
}

void ProjectsToolBar::updateTranslations()
{
    actions().at(0)->setToolTip(tr("Show main menu"));
}

void ProjectsToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

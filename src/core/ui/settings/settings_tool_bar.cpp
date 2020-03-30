#include "settings_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>


namespace Ui
{

SettingsToolBar::SettingsToolBar(QWidget* _parent)
    : AppBar(_parent)
{
    QAction* backAction = new QAction(this);
    backAction->setIconText(u8"\uf04d");
    addAction(backAction);
    connect(backAction, &QAction::triggered, this, &SettingsToolBar::backPressed);

    designSystemChangeEvent(nullptr);
}

void SettingsToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

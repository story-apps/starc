#include "account_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>


namespace Ui {

AccountToolBar::AccountToolBar(QWidget* _parent)
    : AppBar(_parent)
{
    auto backAction = new QAction(this);
    backAction->setIconText(u8"\U000F004D");
    addAction(backAction);
    connect(backAction, &QAction::triggered, this, &AccountToolBar::backPressed);
}

void AccountToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

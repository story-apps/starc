#include "account_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>


namespace Ui
{

AccountToolBar::AccountToolBar(QWidget* _parent)
    : AppBar(_parent)
{
    QAction* backAction = new QAction(this);
    backAction->setText("\uf04d");
    addAction(backAction);
    connect(backAction, &QAction::triggered, this, &AccountToolBar::backPressed);

    designSystemChangeEvent(nullptr);
}

void AccountToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

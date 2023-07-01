#include "account_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>


namespace Ui {

namespace {
enum {
    kBackActionIndex = 0,
    kAccountActionIndex,
    kTeamActionIndex,
};
}

AccountToolBar::AccountToolBar(QWidget* _parent)
    : AppBar(_parent)
{
    auto backAction = new QAction(this);
    backAction->setIconText(u8"\U000F004D");
    addAction(backAction);
    connect(backAction, &QAction::triggered, this, &AccountToolBar::backPressed);

    auto accountAction = new QAction(this);
    accountAction->setCheckable(true);
    accountAction->setChecked(true);
    accountAction->setIconText(u8"\U000F0013");
    addAction(accountAction);
    connect(accountAction, &QAction::toggled, this, [this](bool _checked) {
        if (_checked) {
            emit accountPressed();
        }
    });

    auto teamAction = new QAction(this);
    teamAction->setCheckable(true);
    teamAction->setIconText(u8"\U000f0b58");
    addAction(teamAction);
    connect(teamAction, &QAction::toggled, this, [this](bool _checked) {
        if (_checked) {
            emit teamPressed();
        }
    });

    auto actionGroup = new QActionGroup(this);
    actionGroup->addAction(accountAction);
    actionGroup->addAction(teamAction);
}

void AccountToolBar::updateTranslations()
{
    actions().at(kBackActionIndex)->setToolTip(tr("Go back to the previous screen"));
    actions().at(kAccountActionIndex)->setToolTip(tr("Manage account"));
    actions().at(kTeamActionIndex)->setToolTip(tr("Manage teams"));
}

void AccountToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

#include "settings_tool_bar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/app_bar/app_bar.h>

#include <QAction>


namespace Ui {

class SettingsToolBar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    AppBar* toolBar = nullptr;
};

SettingsToolBar::Implementation::Implementation(QWidget* _parent)
    : toolBar(new AppBar(_parent))
{
}


// ****


SettingsToolBar::SettingsToolBar(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    showDefaultPage();

    QAction* backAction = new QAction(this);
    backAction->setIconText(u8"\U000f004d");
    d->toolBar->addAction(backAction);
    connect(backAction, &QAction::triggered, this, &SettingsToolBar::backPressed);

    designSystemChangeEvent(nullptr);
}

SettingsToolBar::~SettingsToolBar() = default;

void SettingsToolBar::showDefaultPage()
{
    setCurrentWidget(d->toolBar);
}

void SettingsToolBar::updateTranslations()
{
    d->toolBar->actions().at(0)->setToolTip(tr("Go back to the previous screen"));
}

void SettingsToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);
    setBackgroundColor(DesignSystem::color().primary());
    d->toolBar->setBackgroundColor(DesignSystem::color().primary());
    d->toolBar->setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

#include "session_statistics_tool_bar.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/app_bar/app_bar.h>

#include <QAction>


namespace Ui {

class SessionStatisticsToolBar::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    AppBar* toolBar = nullptr;
};

SessionStatisticsToolBar::Implementation::Implementation(QWidget* _parent)
    : toolBar(new AppBar(_parent))
{
}


// ****


SessionStatisticsToolBar::SessionStatisticsToolBar(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    showDefaultPage();

    QAction* backAction = new QAction(this);
    backAction->setIconText(u8"\U000F004D");
    d->toolBar->addAction(backAction);
    connect(backAction, &QAction::triggered, this, &SessionStatisticsToolBar::backPressed);
}

SessionStatisticsToolBar::~SessionStatisticsToolBar() = default;

void SessionStatisticsToolBar::showDefaultPage()
{
    setCurrentWidget(d->toolBar);
}

void SessionStatisticsToolBar::updateTranslations()
{
    d->toolBar->actions().at(0)->setToolTip(tr("Go back to the previous screen"));
}

void SessionStatisticsToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);
    setBackgroundColor(DesignSystem::color().primary());
    d->toolBar->setBackgroundColor(DesignSystem::color().primary());
    d->toolBar->setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

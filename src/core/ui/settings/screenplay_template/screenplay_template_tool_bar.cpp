#include "screenplay_template_tool_bar.h"

#include <ui/design_system/design_system.h>

#include <QAction>
#include <QActionGroup>


namespace Ui {

class ScreenplayTemplateToolBar::Implementation
{
public:
    explicit Implementation(QObject* _parent);

    QAction* backAction = nullptr;
    QAction* pageSettingsAction = nullptr;
    QAction* titlePageAction = nullptr;
    QAction* paragraphAction = nullptr;
};

ScreenplayTemplateToolBar::Implementation::Implementation(QObject* _parent)
    : backAction(new QAction(_parent))
    , pageSettingsAction(new QAction(_parent))
    , titlePageAction(new QAction(_parent))
    , paragraphAction(new QAction(_parent))
{
    backAction->setIconText(u8"\U000F004D");

    pageSettingsAction->setIconText(u8"\U000F0224");
    pageSettingsAction->setCheckable(true);
    pageSettingsAction->setChecked(true);

    titlePageAction->setIconText(u8"\U000F00BE");
    titlePageAction->setCheckable(true);
    titlePageAction->setChecked(false);

    paragraphAction->setIconText(u8"\U000F09AA");
    paragraphAction->setCheckable(true);
    paragraphAction->setChecked(false);

    auto actionGroup = new QActionGroup(_parent);
    actionGroup->addAction(pageSettingsAction);
    actionGroup->addAction(titlePageAction);
    actionGroup->addAction(paragraphAction);
}


// ****


ScreenplayTemplateToolBar::ScreenplayTemplateToolBar(QWidget* _parent)
    : AppBar(_parent)
    , d(new Implementation(this))
{
    addAction(d->backAction);
    connect(d->backAction, &QAction::triggered, this, &ScreenplayTemplateToolBar::backPressed);

    addAction(d->pageSettingsAction);
    connect(d->pageSettingsAction, &QAction::toggled, this,
            &ScreenplayTemplateToolBar::pageSettingsPressed);

    addAction(d->titlePageAction);
    connect(d->titlePageAction, &QAction::toggled, this,
            &ScreenplayTemplateToolBar::titlePageSettingsPressed);

    addAction(d->paragraphAction);
    connect(d->paragraphAction, &QAction::toggled, this,
            &ScreenplayTemplateToolBar::paragraphSettingsPressed);
}

ScreenplayTemplateToolBar::~ScreenplayTemplateToolBar() = default;

void ScreenplayTemplateToolBar::checkPageSettings()
{
    QSignalBlocker signalBlocker(this);
    d->pageSettingsAction->setChecked(true);
}

void ScreenplayTemplateToolBar::setTitlePageVisible(bool _visible)
{
    d->titlePageAction->setVisible(_visible);
}

void ScreenplayTemplateToolBar::updateTranslations()
{
    d->backAction->setToolTip(tr("Go back to the previous screen"));
    d->pageSettingsAction->setToolTip(tr("Template page settings"));
    d->titlePageAction->setToolTip(tr("Title page template"));
    d->paragraphAction->setToolTip(tr("Template paragraphs settings"));
}

void ScreenplayTemplateToolBar::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AppBar::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());
    setTextColor(DesignSystem::color().onPrimary());
}

} // namespace Ui

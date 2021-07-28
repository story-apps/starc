#include "screenplay_template_manager.h"

#include <ui/settings/screenplay_template/screenplay_template_navigator.h>
#include <ui/settings/screenplay_template/screenplay_template_page_view.h>
#include <ui/settings/screenplay_template/screenplay_template_paragraphs_view.h>
#include <ui/settings/screenplay_template/screenplay_template_tool_bar.h>

namespace ManagementLayer {

class ScreenplayTemplateManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Ui::ScreenplayTemplateToolBar* toolBar = nullptr;
    Ui::ScreenplayTemplateNavigator* navigator = nullptr;
    Ui::ScreenplayTemplatePageView* pageView = nullptr;
    Widget* titlePageView = nullptr;
    Ui::ScreenplayTemplateParagraphsView* paragraphsView = nullptr;
};

ScreenplayTemplateManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new Ui::ScreenplayTemplateToolBar(_parent))
    , navigator(new Ui::ScreenplayTemplateNavigator(_parent))
    , pageView(new Ui::ScreenplayTemplatePageView(_parent))
    , titlePageView(new Widget(_parent))
    , paragraphsView(new Ui::ScreenplayTemplateParagraphsView(_parent))
{
    toolBar->hide();
    navigator->hide();
    pageView->hide();
    titlePageView->hide();
    paragraphsView->hide();
}


// ****


ScreenplayTemplateManager::ScreenplayTemplateManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(_parentWidget))
{
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::backPressed, this,
            &ScreenplayTemplateManager::closeRequested);
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::pageSettingsPressed, this,
            [this] { emit showViewRequested(d->pageView); });
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::titlePageSettingsPressed, this,
            [this] { emit showViewRequested(d->titlePageView); });
    connect(d->toolBar, &Ui::ScreenplayTemplateToolBar::paragraphSettingsPressed, this,
            [this] { emit showViewRequested(d->paragraphsView); });
    connect(d->navigator, &Ui::ScreenplayTemplateNavigator::mmCheckedChanged, d->pageView,
            &Ui::ScreenplayTemplatePageView::setUseMm);
}

ScreenplayTemplateManager::~ScreenplayTemplateManager() = default;

QWidget* ScreenplayTemplateManager::toolBar() const
{
    return d->toolBar;
}

QWidget* ScreenplayTemplateManager::navigator() const
{
    return d->navigator;
}

QWidget* ScreenplayTemplateManager::view() const
{
    return d->pageView;
}

} // namespace ManagementLayer

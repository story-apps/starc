#include "onboarding_manager.h"

#include <ui/onboarding/onboarding_navigator.h>
#include <ui/onboarding/onboarding_tool_bar.h>
#include <ui/onboarding/onboarding_view.h>


namespace ManagementLayer
{

class OnboardingManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Ui::OnboardingToolBar* toolBar = nullptr;
    Ui::OnboardingNavigator* navigator = nullptr;
    Ui::OnboardingView* view = nullptr;
};

OnboardingManager::Implementation::Implementation(QWidget* _parent)
    : toolBar(new Ui::OnboardingToolBar(_parent)),
      navigator(new Ui::OnboardingNavigator(_parent)),
      view(new Ui::OnboardingView(_parent))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}


//****


OnboardingManager::OnboardingManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->navigator, &Ui::OnboardingNavigator::currentIndexChanged, this, [this] (int _currentIndex) {
        switch (_currentIndex) {
            case 0: {
                d->view->showLanguagePage();
                break;
            }

            case 1: {
                d->view->showThemePage();
                break;
            }
        }
    });
    connect(d->view, &Ui::OnboardingView::languageChanged, this, &OnboardingManager::languageChanged);
    connect(d->view, &Ui::OnboardingView::showThemePageRequested, d->navigator, &Ui::OnboardingNavigator::showThemeStep);
    connect(d->view, &Ui::OnboardingView::themeChanged, this, &OnboardingManager::themeChanged);
    connect(d->view, &Ui::OnboardingView::scaleFactorChanged, this, &OnboardingManager::scaleFactorChanged);
    connect(d->view, &Ui::OnboardingView::skipOnboardingRequested, this, &OnboardingManager::finished);
    connect(d->view, &Ui::OnboardingView::finishOnboardingRequested, this, &OnboardingManager::finished);
}

QWidget* OnboardingManager::toolBar() const
{
    return d->toolBar;
}

QWidget* OnboardingManager::navigator() const
{
    return d->navigator;
}

QWidget* OnboardingManager::view() const
{
    return d->view;
}

OnboardingManager::~OnboardingManager() = default;

} // namespace ManagementLayer

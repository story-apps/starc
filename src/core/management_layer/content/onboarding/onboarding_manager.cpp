#include "onboarding_manager.h"

#include <ui/onboarding/onboarding_navigator.h>
#include <ui/onboarding/onboarding_tool_bar.h>
#include <ui/onboarding/onboarding_view.h>


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

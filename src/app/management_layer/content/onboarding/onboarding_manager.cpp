#include "onboarding_manager.h"

#include <ui/models/onboarding_view_model.h>

#include <QQmlContext>


class OnboardingManager::Implementation
{
public:
    Implementation();

    QScopedPointer<OnboardingViewModel> model;
};

OnboardingManager::Implementation::Implementation()
    : model(new OnboardingViewModel)
{
}


//****


QString OnboardingManager::toolBar()
{
    return "qrc:/views/onboarding/OnboardingToolBar.qml";
}

QString OnboardingManager::navigator()
{
    return "qrc:/views/onboarding/OnboardingNavigator.qml";
}

QString OnboardingManager::view()
{
    return "qrc:/views/onboarding/OnboardingView.qml";
}

OnboardingManager::OnboardingManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

void OnboardingManager::setupContext(QQmlContext* _rootContext)
{
    _rootContext->setContextProperty("OnboardingViewModel", d->model.data());
}

OnboardingManager::~OnboardingManager() = default;

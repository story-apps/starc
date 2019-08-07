#include "application_manager.h"

#include "content/onboarding/onboarding_manager.h"
#include "content/splash/splash_manager.h"

#include <ui/models/application_view_model.h>

#include <QTimer>
#include <QQmlContext>


class ApplicationManager::Implementation
{
public:
    Implementation();

    QScopedPointer<ApplicationViewModel> model;

    QScopedPointer<OnboardingManager> onboardingManager;
};

ApplicationManager::Implementation::Implementation()
    : model(new ApplicationViewModel(SplashManager::toolBar(), SplashManager::navigator(), SplashManager::view())),
      onboardingManager(new OnboardingManager)
{
}


// ****


ApplicationManager::ApplicationManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
    QTimer::singleShot(0, this, [this] {
        d->model->setCurrentToolBar(OnboardingManager::toolBar());
        d->model->setCurrentNavigator(OnboardingManager::navigator());
        d->model->setCurrentView(OnboardingManager::view());
    });
}

void ApplicationManager::setupContext(QQmlContext* _rootContext)
{
    _rootContext->setContextProperty("ApplicationViewModel", d->model.data());

    d->onboardingManager->setupContext(_rootContext);
}

ApplicationManager::~ApplicationManager() = default;

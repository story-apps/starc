#include "application_manager.h"

#include "content/onboarding/onboarding_manager.h"
#include "content/splash/splash_manager.h"

#include <ui/application_view.h>

#include <QFontDatabase>
#include <QTimer>
#include <QWidget>


namespace ManagementLayer {

class ApplicationManager::Implementation
{
public:
    Implementation();

    /**
     * @brief Показать контент приложения
     */
    void showContent();

    QScopedPointer<Ui::ApplicationView> applicationView;

    QScopedPointer<OnboardingManager> onboardingManager;
};

ApplicationManager::Implementation::Implementation()
    : applicationView(new Ui::ApplicationView),
      onboardingManager(new OnboardingManager(nullptr, applicationView.data()))
{
}

void ApplicationManager::Implementation::showContent()
{
    //
    // TODO: Если это первый запуск приложения, то покажем онбординг
    //

    applicationView->showContent(onboardingManager->toolBar(),
                                 onboardingManager->navigator(),
                                 onboardingManager->view());

    //
    // TODO: В противном случае показываем недавние проекты
    //
}


// ****


ApplicationManager::ApplicationManager(QObject* _parent)
    : QObject(_parent),
      IApplicationManager(),
      d(new Implementation)
{
    //
    // Загрузим шрифты в базу шрифтов программы, если их там ещё нет
    //
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/fonts/materialdesignicons.ttf");
}

ApplicationManager::~ApplicationManager() = default;

void ApplicationManager::exec()
{
    d->applicationView->resize(400, 400);
    d->applicationView->show();

    //
    // Показываем содержимое, после того, как на экране отображится приложения,
    // чтобы у пользователя возник эффект моментального запуска
    //
    QTimer::singleShot(0, this, [this] { d->showContent(); });
}

} // namespace ManagementLayer

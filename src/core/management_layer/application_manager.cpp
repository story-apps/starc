#include "application_manager.h"

#include "content/onboarding/onboarding_manager.h"
#include "content/splash/splash_manager.h"

#include <ui/application_view.h>

#include <QApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QTimer>
#include <QTranslator>
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

    /**
     * @brief Обновить перевод
     */
    void updateTranslation(QLocale::Language _language);

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

void ApplicationManager::Implementation::updateTranslation(QLocale::Language _language)
{
    //
    // Определим файл перевода
    //
    const QLocale::Language currentLanguage = _language != QLocale::AnyLanguage
                                        ? _language
                                        : QLocale::system().language();
    QString translation;
    switch (currentLanguage) {
        default:
        case QLocale::English: {
            translation = "en_EN";
            break;
        }

        case QLocale::Russian: {
            translation = "ru_RU";
            break;
        }
    }

    QLocale::setDefault(QLocale(currentLanguage));

    //
    // Подключим файл переводов программы
    //
    static QTranslator* appTranslator = new QTranslator;
    QApplication::removeTranslator(appTranslator);
    appTranslator->load(":/translations/" + translation + ".qm");
    QApplication::installTranslator(appTranslator);

    //
    // Для языков, которые пишутся справа-налево настроим соответствующее выравнивание интерфейса
    //
    if (currentLanguage == QLocale::Persian
        || currentLanguage == QLocale::Hebrew) {
        QApplication::setLayoutDirection(Qt::RightToLeft);
    } else {
        QApplication::setLayoutDirection(Qt::LeftToRight);
    }
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

    initConnections();
}

ApplicationManager::~ApplicationManager() = default;

void ApplicationManager::exec()
{
    d->applicationView->resize(600, 400);
    d->applicationView->show();

    //
    // Показываем содержимое, после того, как на экране отображится приложения,
    // чтобы у пользователя возник эффект моментального запуска
    //
    QTimer::singleShot(0, this, [this] { d->showContent(); });
}

void ApplicationManager::initConnections()
{
    connect(d->onboardingManager.data(), &OnboardingManager::languageChanged, this,
            [this] (QLocale::Language _language) { d->updateTranslation(_language); });
}

} // namespace ManagementLayer

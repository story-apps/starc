#include "application_manager.h"

#include "custom_events.h"

#include "content/account/account_manager.h"
#include "content/onboarding/onboarding_manager.h"
#include "content/projects/projects_manager.h"
#include "content/settings/settings_manager.h"

#ifdef CLOUD_SERVICE_MANAGER
#include <shredder/starc/cloud_service_manager.h>
#endif

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/application_style.h>
#include <ui/application_view.h>
#include <ui/projects/create_project_dialog.h>
#include <ui/menu_view.h>

#include <ui/design_system/design_system.h>

#include <utils/3rd_party/WAF/Animation/Animation.h>

#include <QApplication>
#include <QFontDatabase>
#include <QLocale>
#include <QStyleFactory>
#include <QTimer>
#include <QTranslator>
#include <QVariant>
#include <QWidget>


namespace ManagementLayer {

class ApplicationManager::Implementation
{
public:
    explicit Implementation(ApplicationManager* _q);
    ~Implementation();

    /**
     * @brief Получить значение параметра настроек
     */
    QVariant settingsValue(const QString& _key) const;
    QVariantMap settingsValues(const QString& _key) const;

    /**
     * @brief Показать контент приложения
     */
    void showContent();

    /**
     * @brief Показать меню приложения
     */
    void showMenu();

    /**
     * @brief Показать личный кабинет
     */
    void showAccount();

    /**
     * @brief Показать страницу проектов
     */
    void showProjects();

    /**
     * @brief Показать страницу настроек
     */
    void showSettings();

    /**
     * @brief Показать предыдущий контент
     */
    void showLastContent();

    /**
     * @brief Установить перевод
     */
    void setTranslation(QLocale::Language _language);

    /**
     * @brief Установить тему
     */
    void setTheme(Ui::ApplicationTheme _theme);

    /**
     * @brief Установить коэффициент масштабирования
     */
    void setScaleFactor(qreal _scaleFactor);

    /**
     * @brief Создать новый проект
     */
    void createStory();


    ApplicationManager* q = nullptr;

    Ui::ApplicationView* applicationView = nullptr;
    Ui::MenuView* menuView = nullptr;
    struct LastContent {
        QWidget* toolBar = nullptr;
        QWidget* navigator = nullptr;
        QWidget* view = nullptr;
    } lastContent;

    QScopedPointer<AccountManager> accountManager;
    QScopedPointer<OnboardingManager> onboardingManager;
    QScopedPointer<ProjectsManager> projectsManager;
    QScopedPointer<SettingsManager> settingsManager;
#ifdef CLOUD_SERVICE_MANAGER
    QScopedPointer<CloudServiceManager> cloudServiceManager;
#endif

private:
    template<typename Manager>
    void saveLastContent(Manager* _manager);
};

ApplicationManager::Implementation::Implementation(ApplicationManager* _q)
    : q(_q),
      applicationView(new Ui::ApplicationView),
      menuView(new Ui::MenuView(applicationView)),
      accountManager(new AccountManager(nullptr, applicationView)),
      onboardingManager(new OnboardingManager(nullptr, applicationView)),
      projectsManager(new ProjectsManager(nullptr, applicationView)),
      settingsManager(new SettingsManager(nullptr, applicationView))
#ifdef CLOUD_SERVICE_MANAGER
    , cloudServiceManager(new CloudServiceManager)
#endif
{
    applicationView->setAccountBar(accountManager->accountBar());
}

ApplicationManager::Implementation::~Implementation()
{
    applicationView->disconnect();
    menuView->disconnect();
    accountManager->disconnect();
    onboardingManager->disconnect();
    projectsManager->disconnect();
#ifdef CLOUD_SERVICE_MANAGER
    cloudServiceManager->disconnect();
#endif
}

QVariant ApplicationManager::Implementation::settingsValue(const QString& _key) const
{
    return DataStorageLayer::StorageFacade::settingsStorage()->value(
                _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
}

QVariantMap ApplicationManager::Implementation::settingsValues(const QString& _key) const
{
    return DataStorageLayer::StorageFacade::settingsStorage()->values(
                _key, DataStorageLayer::SettingsStorage::SettingsPlace::Application);
}

void ApplicationManager::Implementation::showContent()
{
    //
    // Если это первый запуск приложения, то покажем онбординг
    //
    if (settingsValue(DataStorageLayer::kApplicationConfiguredKey).toBool() == false) {
        applicationView->showContent(onboardingManager->toolBar(),
                                     onboardingManager->navigator(),
                                     onboardingManager->view());
    }
    //
    // В противном случае показываем недавние проекты
    //
    else {
        //
        // Сперва проекты нужно загрузить
        //
        projectsManager->loadProjects();
        //
        // ... а затем уже отобразить
        //
        showProjects();

        //
        // Сохраняем последнее отображаемое представление, чтобы можно было вернуться к нему
        //
        saveLastContent(projectsManager.data());

#ifdef CLOUD_SERVICE_MANAGER
        //
        // Если менеджер облака доступен, отобржаем панель для работы с личным кабинетом
        //
        accountManager->accountBar()->show();
#endif
    }
}

void ApplicationManager::Implementation::showMenu()
{
    menuView->setFixedWidth(std::max(applicationView->navigationPanelWidth(),
                                     static_cast<int>(Ui::DesignSystem::drawer().width())));
    WAF::Animation::sideSlideIn(menuView);
}

void ApplicationManager::Implementation::showAccount()
{
    applicationView->showContent(accountManager->toolBar(),
                                 accountManager->navigator(),
                                 accountManager->view());
    accountManager->accountBar()->hide();
}

void ApplicationManager::Implementation::showProjects()
{
    applicationView->showContent(projectsManager->toolBar(),
                                 projectsManager->navigator(),
                                 projectsManager->view());
}

void ApplicationManager::Implementation::showSettings()
{
    applicationView->showContent(settingsManager->toolBar(),
                                 settingsManager->navigator(),
                                 settingsManager->view());
}

void ApplicationManager::Implementation::showLastContent()
{
    if (lastContent.toolBar == nullptr
        || lastContent.navigator == nullptr
        || lastContent.view == nullptr) {
        return;
    }

    applicationView->showContent(lastContent.toolBar, lastContent.navigator, lastContent.view);
}

void ApplicationManager::Implementation::setTranslation(QLocale::Language _language)
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
    static QTranslator* appTranslator = [] {
        //
        // ... небольшой workaround для того, чтобы при запуске приложения кинуть событие о смене языка
        //
        QTranslator* translator = new QTranslator;
        QApplication::installTranslator(translator);
        return translator;
    } ();
    //
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

void ApplicationManager::Implementation::setTheme(Ui::ApplicationTheme _theme)
{
    Ui::DesignSystem::setTheme(_theme);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::setScaleFactor(qreal _scaleFactor)
{
    Ui::DesignSystem::setScaleFactor(_scaleFactor);
    QApplication::postEvent(q, new DesignSystemChangeEvent);
}

void ApplicationManager::Implementation::createStory()
{
    Ui::CreateProjectDialog* dlg = new Ui::CreateProjectDialog(applicationView);
    dlg->showDialog();
}

template<typename Manager>
void ApplicationManager::Implementation::saveLastContent(Manager* _manager)
{
    lastContent.toolBar = _manager->toolBar();
    lastContent.navigator = _manager->navigator();
    lastContent.view = _manager->view();
}


// ****


ApplicationManager::ApplicationManager(QObject* _parent)
    : QObject(_parent),
      IApplicationManager(),
      d(new Implementation(this))
{
    QApplication::setStyle(new ApplicationStyle(QStyleFactory::create("Fusion")));

    //
    // Загрузим шрифты в базу шрифтов программы, если их там ещё нет
    //
    QFontDatabase fontDatabase;
    fontDatabase.addApplicationFont(":/fonts/materialdesignicons");
    fontDatabase.addApplicationFont(":/fonts/roboto-black");
    fontDatabase.addApplicationFont(":/fonts/roboto-bold");
    fontDatabase.addApplicationFont(":/fonts/roboto-medium");
    fontDatabase.addApplicationFont(":/fonts/roboto-regular");
    fontDatabase.addApplicationFont(":/fonts/roboto-thin");

    initConnections();
}

ApplicationManager::~ApplicationManager() = default;

void ApplicationManager::exec()
{
    //
    // Установим размер экрана по-умолчанию, на случай, если это первый запуск
    //
    d->applicationView->resize(1024, 640);
    //
    // ... затем пробуем загрузить геометрию и состояние приложения
    //
    d->setTranslation(d->settingsValue(DataStorageLayer::kApplicationLanguagedKey).value<QLocale::Language>());
    d->setTheme(static_cast<Ui::ApplicationTheme>(d->settingsValue(DataStorageLayer::kApplicationThemeKey).toInt()));
    d->setScaleFactor(d->settingsValue(DataStorageLayer::kApplicationScaleFactorKey).toReal());
    d->applicationView->restoreState(d->settingsValues(DataStorageLayer::kApplicationViewStateKey));

    //
    // Покажем интерфейс
    //
    d->applicationView->show();

    //
    // Показываем содержимое, после того, как на экране отображится приложения,
    // чтобы у пользователя возник эффект моментального запуска
    //
    QTimer::singleShot(0, this, [this] {
        d->showContent();
#ifdef CLOUD_SERVICE_MANAGER
        d->cloudServiceManager->start();
#endif
    });
}

bool ApplicationManager::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
        case static_cast<QEvent::Type>(EventType::IdleEvent): {
            //
            // Этот слот нельзя вызывать напрямую, иначе падаем с assert на iOS
            //
//            QTimer::singleShot(0, this, &ApplicationManager::saveCurrentProject);

            _event->accept();
            return true;
        }

        case static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent): {
            //
            // Уведомляем все виджеты о том, что сменилась дизайн система
            //
            for (auto widget : d->applicationView->findChildren<QWidget*>()) {
                QApplication::sendEvent(widget, _event);
            }
            QApplication::sendEvent(d->applicationView, _event);

            _event->accept();
            return true;
        }

        default: {
            return QObject::event(_event);
        }
    }
}

void ApplicationManager::initConnections()
{
    //
    // Представление приложения
    //
    connect(d->applicationView, &Ui::ApplicationView::closeRequested, this, [this]
    {
        //
        // TODO: Сохранение, все дела
        //
        d->projectsManager->saveProjects();

        //
        // Сохраняем состояние приложения
        //
        DataStorageLayer::StorageFacade::settingsStorage()->setValues(
                    DataStorageLayer::kApplicationViewStateKey,
                    d->applicationView->saveState(),
                    DataStorageLayer::SettingsStorage::SettingsPlace::Application);

        //
        // Выходим
        //
        QApplication::processEvents();
        QApplication::quit();
    });
    connect(d->menuView, &Ui::MenuView::storiesPressed, this, [this] { d->showProjects(); });
    connect(d->menuView, &Ui::MenuView::createStoryPressed, d->projectsManager.data(), &ProjectsManager::createStoryRequested);
    connect(d->menuView, &Ui::MenuView::settingsPressed, this, [this] { d->showSettings(); });

    //
    // Менеджер посадки
    //
    connect(d->onboardingManager.data(), &OnboardingManager::languageChanged, this,
            [this] (QLocale::Language _language) { d->setTranslation(_language); });
    connect(d->onboardingManager.data(), &OnboardingManager::themeChanged, this,
            [this] (Ui::ApplicationTheme _theme) { d->setTheme(_theme); });
    connect(d->onboardingManager.data(), &OnboardingManager::scaleFactorChanged, this,
            [this] (qreal _scaleFactor) { d->setScaleFactor(_scaleFactor); });
    connect(d->onboardingManager.data(), &OnboardingManager::finished, this,
            [this]
    {
        auto setSettingsValue = [] (const QString& _key, const QVariant& _value) {
            DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                        _key,
                        _value,
                        DataStorageLayer::SettingsStorage::SettingsPlace::Application);
        };
        setSettingsValue(DataStorageLayer::kApplicationConfiguredKey, true);
        setSettingsValue(DataStorageLayer::kApplicationLanguagedKey, QLocale::system().language());
        setSettingsValue(DataStorageLayer::kApplicationThemeKey, static_cast<int>(Ui::DesignSystem::theme()));
        setSettingsValue(DataStorageLayer::kApplicationScaleFactorKey, Ui::DesignSystem::scaleFactor());
        d->showContent();
    });

    //
    // Менеджер аккаунта
    //
    connect(d->accountManager.data(), &AccountManager::showAccountRequired, this, [this] { d->showAccount(); });
    connect(d->accountManager.data(), &AccountManager::closeAccountRequired, this, [this] {
        d->accountManager->accountBar()->show();
        d->showLastContent();
    });

    //
    // Менеджер проектов
    //
    connect(d->projectsManager.data(), &ProjectsManager::menuRequested, this, [this] { d->showMenu(); });
    connect(d->projectsManager.data(), &ProjectsManager::createStoryRequested, this, [this] { d->createStory(); });

#ifdef CLOUD_SERVICE_MANAGER
    //
    // Менеджер облака
    //
    // ... поймали/потеряли связь
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::connected,
            d->accountManager.data(), &AccountManager::notifyConnected);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::disconnected,
            d->accountManager.data(), &AccountManager::notifyDisconnected);
    //
    // ... авторизация/регистрация
    //
    {
        //
        // проверка регистрация или вход
        //
        connect(d->accountManager.data(), &AccountManager::emailEntered,
                d->cloudServiceManager.data(), &CloudServiceManager::canLogin);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationAllowed,
                d->accountManager.data(), &AccountManager::allowRegistration);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::loginAllowed,
                d->accountManager.data(), &AccountManager::allowLogin);

        //
        // регистрация
        //
        connect(d->accountManager.data(), &AccountManager::registrationRequired,
                d->cloudServiceManager.data(), &CloudServiceManager::registerAccount);
        connect(d->accountManager.data(), &AccountManager::registrationConfirmationCodeEntered,
                d->cloudServiceManager.data(), &CloudServiceManager::confirmRegistration);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationConfiramtionCodeSended,
                d->accountManager.data(), &AccountManager::prepareToEnterRegistrationConfirmationCode);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationConfirmationError,
                d->accountManager.data(), &AccountManager::setRegistrationConfirmationError);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationCompleted,
                d->accountManager.data(), &AccountManager::login);

        //
        // восстановление пароля
        //
        connect(d->accountManager.data(), &AccountManager::restorePasswordRequired,
                d->cloudServiceManager.data(), &CloudServiceManager::restorePassword);
        connect(d->accountManager.data(), &AccountManager::passwordRestoringConfirmationCodeEntered,
                d->cloudServiceManager.data(), &CloudServiceManager::confirmPasswordRestoring);
        connect(d->accountManager.data(), &AccountManager::changePasswordRequested,
                d->cloudServiceManager.data(), &CloudServiceManager::changePassword);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::passwordRestoringConfirmationCodeSended,
                d->accountManager.data(), &AccountManager::prepareToEnterRestorePasswordConfirmationCode);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::passwordRestoringConfirmationSucceed,
                d->accountManager.data(), &AccountManager::allowChangePassword);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::registrationConfirmationError,
                d->accountManager.data(), &AccountManager::setRestorePasswordConfirmationError);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::passwordChanged,
                d->accountManager.data(), &AccountManager::login);

        //
        // авторизация
        //
        connect(d->accountManager.data(), &AccountManager::loginRequired,
                d->cloudServiceManager.data(), &CloudServiceManager::login);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::loginPasswordError,
                d->accountManager.data(), &AccountManager::setLoginPasswordError);
        connect(d->cloudServiceManager.data(), &CloudServiceManager::loginCompleted,
                d->accountManager.data(), &AccountManager::completeLogin);
    }

    //
    // Получены параметры об аккаунте
    //
    connect(d->cloudServiceManager.data(), &CloudServiceManager::accountParametersLoaded,
            d->accountManager.data(), &AccountManager::setAccountParameters);

    //
    // Изменение параметров аккаунта
    //
    connect(d->accountManager.data(), &AccountManager::changeUserNameRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setUserName);
    connect(d->accountManager.data(), &AccountManager::changeReceiveEmailNotificationsRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setNeedNotify);
    connect(d->accountManager.data(), &AccountManager::changeAvatarRequested,
            d->cloudServiceManager.data(), &CloudServiceManager::setAvatar);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::userNameChanged,
            d->accountManager.data(), &AccountManager::setUserName);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::receiveEmailNotificationsChanged,
            d->accountManager.data(), &AccountManager::setReceiveEmailNotifications);
    connect(d->cloudServiceManager.data(), &CloudServiceManager::avatarChanged,
            d->accountManager.data(), qOverload<const QByteArray&>(&AccountManager::setAvatar));
#endif
}

} // namespace ManagementLayer

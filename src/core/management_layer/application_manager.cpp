#include "application_manager.h"

#include "custom_events.h"

#include "content/onboarding/onboarding_manager.h"
#include "content/projects/projects_manager.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/application_style.h>
#include <ui/application_view.h>
#include <ui/design_system/design_system.h>

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


    ApplicationManager* q = nullptr;

    Ui::ApplicationView* applicationView = nullptr;

    QScopedPointer<OnboardingManager> onboardingManager;
    QScopedPointer<ProjectsManager> projectsManager;
};

ApplicationManager::Implementation::Implementation(ApplicationManager* _q)
    : q(_q),
      applicationView(new Ui::ApplicationView),
      onboardingManager(new OnboardingManager(nullptr, applicationView)),
      projectsManager(new ProjectsManager(nullptr, applicationView))
{
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
        applicationView->showContent(projectsManager->toolBar(),
                                     projectsManager->navigator(),
                                     projectsManager->view());
    }
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
        d->setTranslation(QLocale::AnyLanguage);
        d->showContent();
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

}

} // namespace ManagementLayer

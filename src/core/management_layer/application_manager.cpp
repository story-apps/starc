#include "application_manager.h"

#include "custom_events.h"

#include "content/onboarding/onboarding_manager.h"

#include <ui/application_view.h>
#include <ui/design_system/design_system.h>

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

    /**
     * @brief Обновить тему
     */
    void updateTheme(Ui::ApplicationTheme _theme);

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

void ApplicationManager::Implementation::updateTheme(Ui::ApplicationTheme _theme)
{

    QColor primary;
    QColor primaryDark;
    QColor secondary;
    QColor background;
    QColor surface;
    QColor error;
    QColor shadow;
    QColor onPrimary;
    QColor onSecondary;
    QColor onBackground;
    QColor onSurface;
    QColor onError;

    switch (_theme) {
        case Ui::ApplicationTheme::DarkAndLight: {
            primary = "#323740";
            primaryDark = "#22252b";
            secondary = "#448AFF";
            background = "#FFFFFF";
            surface = "#FFFFFF";
            error = "#B00020";
            shadow = [] { QColor color = "#000000";
                          color.setAlphaF(0.3);
                          return color; } ();
            onPrimary = "#FFFFFF";
            onSecondary = "#FFFFFF";
            onBackground = "#000000";
            onSurface = "#000000";
            onError = "#FFFFFF";
            break;
        }

        case Ui::ApplicationTheme::Dark: {
            primary = "#1F1F1F";
            primaryDark = "#0A0A0A";
            secondary = "#448AFF";
            background = "#121212";
            surface = "#121212";
            error = "#CF6679";
            shadow = [] { QColor color = "#000000";
                          color.setAlphaF(0.68);
                          return color; } ();
            onPrimary = "#FFFFFF";
            onSecondary = "#FFFFFF";
            onBackground = "#FFFFFF";
            onSurface = "#FFFFFF";
            onError = "#000000";
            break;
        }

        case Ui::ApplicationTheme::Light: {
            primary = "#E4E4E4";
            primaryDark = "#C8C8C8";
            secondary = "#448AFF";
            background = "#FFFFFF";
            surface = "#FFFFFF";
            error = "#B00020";
            shadow = [] { QColor color = "#000000";
                          color.setAlphaF(0.36);
                          return color; } ();
            onPrimary = "#38393A";
            onSecondary = "#FFFFFF";
            onBackground = "#000000";
            onSurface = "#000000";
            onError = "#FFFFFF";
            break;
        }

        case Ui::ApplicationTheme::Custom: {
            //
            // TODO:
            //
            break;
        }
    }


    auto color(Ui::DesignSystem::color());
    color.setPrimary(primary);
    color.setPrimaryDark(primaryDark);
    color.setSecondary(secondary);
    color.setBackground(background);
    color.setSurface(surface);
    color.setError(error);
    color.setShadow(shadow);
    color.setOnPrimary(onPrimary);
    color.setOnSecondary(onSecondary);
    color.setOnBackground(onBackground);
    color.setOnSurface(onSurface);
    color.setOnError(onError);
    Ui::DesignSystem::setColor(color);
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
    d->applicationView->resize(1024, 640);
    d->applicationView->show();

    //
    // Показываем содержимое, после того, как на экране отображится приложения,
    // чтобы у пользователя возник эффект моментального запуска
    //
    QTimer::singleShot(0, this, [this] {
        d->updateTranslation(QLocale::AnyLanguage);
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
            QApplication::sendEvent(d->applicationView.data(), _event);

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
    connect(d->onboardingManager.data(), &OnboardingManager::languageChanged, this,
            [this] (QLocale::Language _language) { d->updateTranslation(_language); });
    connect(d->onboardingManager.data(), &OnboardingManager::themeChanged, this,
            [this] (Ui::ApplicationTheme _theme)
    {
        d->updateTheme(_theme);
        QApplication::postEvent(this, new DesignSystemChangeEvent);
    });
    connect(d->onboardingManager.data(), &OnboardingManager::scaleFactorChanged, this,
            [this] (qreal _scaleFactor)
    {
        Ui::DesignSystem::setScaleFactor(_scaleFactor);
        QApplication::postEvent(this, new DesignSystemChangeEvent);
    });
}

} // namespace ManagementLayer

#include "application.h"

#include <include/custom_events.h>
#include <interfaces/management_layer/i_application_manager.h>

#include <QFileOpenEvent>
#include <QIcon>
#include <QTimer>


namespace {
/**
 * @brief Подготовить путь к файлу для сохранения
 */
static QString preparePath(const QString& _path)
{
    QString newPath = _path;
#ifdef Q_OS_MAC
    //
    // Это две разные буквы!
    // Первую даёт нам мак, когда открываешь файл через двойной щелчок на нём
    //
    newPath.replace("й", "й");
#endif
    return newPath;
}

} // namespace

class Application::Implementation
{
public:
    QObject* applicationManager = nullptr;

    QString fileToOpen;

    /**
     * @brief Таймер кратковременного простоя приложения (3 секунды)
     */
    QTimer idleTimer;

    /**
     * @brief Таймер долгосрочного простоя приложения (30 минут)
     */
    QTimer longIdleTimer;

    /**
     * @brief Нажатые в данный момент клавиши
     */
    QSet<int> pressedKeys;
};


// **


Application::Application(int& _argc, char** _argv)
    : QApplication(_argc, _argv)
    , d(new Implementation)
{
    setApplicationName("Story Architect");
    setOrganizationName("Story Apps");
    setOrganizationDomain("storyapps.dev");

    //
    // Настроим таймеры определения простоя приложения
    //
    d->idleTimer.setSingleShot(true);
    d->idleTimer.setInterval(std::chrono::seconds{ 3 });
    connect(&d->idleTimer, &QTimer::timeout, this, [this] {
        if (d->applicationManager != nullptr) {
            constexpr bool isLongIdle = false;
            postEvent(d->applicationManager, new IdleEvent(isLongIdle));
        }
    });
    d->idleTimer.start();
    //
    d->longIdleTimer.setSingleShot(true);
    d->longIdleTimer.setInterval(std::chrono::minutes{ 30 });
    connect(&d->longIdleTimer, &QTimer::timeout, this, [this] {
        if (d->applicationManager != nullptr) {
            constexpr bool isLongIdle = true;
            postEvent(d->applicationManager, new IdleEvent(isLongIdle));
        }
    });
    d->longIdleTimer.start();
}

Application::~Application()
{
    if (d->applicationManager != nullptr) {
        delete d->applicationManager;
    }
}

void Application::setApplicationManager(QObject* _manager)
{
    d->applicationManager = _manager;
}

void Application::startUp()
{
    setWindowIcon(QIcon(":/images/logo"));

    auto manager = qobject_cast<ManagementLayer::IApplicationManager*>(d->applicationManager);
    if (manager == nullptr) {
        qCritical() << "Can't start application without application manager";
        exit(1);
    }

    //
    // Получим имя файла, который пользователь возможно хочет открыть
    //
    auto arguments = this->arguments();
    //
    // ... удаляем путь до программы из списка аргументов
    //
    arguments.removeFirst();
    //
    // ... если пользователь задал файл, который нужно открыть, сохраним его
    //
    if (d->fileToOpen.isEmpty() && !arguments.isEmpty()) {
        d->fileToOpen = arguments.constFirst();
    }

    manager->exec(d->fileToOpen);
}

bool Application::notify(QObject* _object, QEvent* _event)
{
    if (_event != nullptr) {
        //
        // Работа с таймером определяющим простой приложения
        //
        switch (_event->type()) {
        case QEvent::MouseMove:
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::KeyPress:
        case QEvent::InputMethod:
        case QEvent::Wheel:
        case QEvent::Gesture: {
            d->idleTimer.start();
            d->longIdleTimer.start();
            break;
        }

        default:
            break;
        }

        //
        // Отправка событий о нажатии клавиш в менеджер приложения
        //
        switch (_event->type()) {
        case QEvent::KeyPress: {
            if (_object == d->applicationManager) {
                break;
            }

            //
            // Пропускаем зажатые клавиши
            //
            const auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (d->pressedKeys.contains(keyEvent->key())) {
                break;
            }

            d->pressedKeys.insert(keyEvent->key());

            postEvent(d->applicationManager,
                      new QKeyEvent(QEvent::KeyPress, keyEvent->key(), keyEvent->modifiers(),
                                    keyEvent->text()));
            break;
        }

        case QEvent::KeyRelease: {
            const auto keyEvent = static_cast<QKeyEvent*>(_event);
            d->pressedKeys.remove(keyEvent->key());
            break;
        }

        case QEvent::FocusIn: {
            postEvent(d->applicationManager, new FocusChangeEvent);
            break;
        }

        default:
            break;
        }
    }

    return QApplication::notify(_object, _event);
}

bool Application::event(QEvent* _event)
{
    if (_event->type() == QEvent::FileOpen) {
        QFileOpenEvent* fileOpenEvent = static_cast<QFileOpenEvent*>(_event);
        if (auto manager
            = qobject_cast<ManagementLayer::IApplicationManager*>(d->applicationManager)) {
            manager->openProject(preparePath(fileOpenEvent->file()));
        } else {
            d->fileToOpen = preparePath(fileOpenEvent->file());
        }

        return true;
    }

    return QApplication::event(_event);
}

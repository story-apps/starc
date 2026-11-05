#include "application.h"

#include <include/custom_events.h>
#include <interfaces/management_layer/i_application_manager.h>

#include <include/deep_link.h>

#include <QCryptographicHash>
#include <QDir>
#include <QFileOpenEvent>
#include <QLocalServer>
#include <QLocalSocket>
#include <QLockFile>
#include <QStandardPaths>
#include <QSaveFile>
#include <QFileInfo>
#include <QProcess>
#include <QIcon>
#include <QTimer>


namespace {
/**
 * @brief Нормализовать путь к файлу для дальнейшей работы со строками
 */
static QString normalizePath(const QString& _path)
{
#ifdef Q_OS_MAC
    //
    // В маке делаем нормализацию для имён файлов, т.к. он любит разделять некоторые буквы и далее
    // не получается их корректно сравнивать с обычными строками,
    //
    // например: "й" и "й" - это две разные буквы! Первую даёт нам мак, когда открываешь файл через
    // двойной щелчок на нём
    //
    return _path.normalized(QString::NormalizationForm_C);
#else
    return _path;
#endif
}

#ifdef Q_OS_LINUX
// An AppImage has no installer. Register its persistent outer path on first launch,
// never the temporary mount containing the running executable.
void registerAppImageLinks()
{
    auto appImage = qEnvironmentVariable("APPIMAGE");
    if (!QFileInfo(appImage).isAbsolute() || !QFileInfo(appImage).isExecutable()
        || appImage.contains(QLatin1Char('\n')) || appImage.contains(QLatin1Char('\r'))) {
        return;
    }
    appImage.replace("\\", "\\\\").replace("\"", "\\\"").replace("`", "\\`").replace("$", "\\$");
    appImage.replace("\\", "\\\\");
    appImage.replace("%", "%%");
    const auto entry = QStringLiteral(
        "[Desktop Entry]\nType=Application\nName=Story Architect\n"
        "Exec=\"%1\" %u\nIcon=starc\nTerminal=false\n"
        "Categories=Office;\nMimeType=application/x-starc;x-scheme-handler/starc;\n").arg(appImage).toUtf8();
    const auto directory = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
        + QStringLiteral("/applications");
    if (!QDir().mkpath(directory)) {
        return;
    }
    const auto path = directory + QStringLiteral("/app.starc.desktop");
    QFile existing(path);
    if (existing.open(QIODevice::ReadOnly) && existing.readAll() == entry) {
        return;
    }
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(entry) != entry.size() || !file.commit()) {
        return;
    }
    QProcess::startDetached(QStringLiteral("xdg-mime"),
                           { QStringLiteral("default"), QStringLiteral("app.starc.desktop"),
                             QStringLiteral("x-scheme-handler/starc") });
}
#endif

} // namespace

class Application::Implementation
{
public:
    QObject* applicationManager = nullptr;

    QString fileToOpen;
    bool started = false;
    QScopedPointer<QLockFile> linkServerLock;
    QLocalServer linkServer;

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
    qInfo() << "Set up window title";
    setWindowIcon(QIcon(":/images/logo"));

    qInfo() << "Set up application manager";
    auto manager = qobject_cast<ManagementLayer::IApplicationManager*>(d->applicationManager);
    if (manager == nullptr) {
        qCritical() << "Application manager is not initialized. "
                       "Can't start application without application manager";
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

    qInfo() << "Starting application";
    manager->exec(d->fileToOpen);
    d->started = true;
}

bool Application::forwardDeepLink()
{
#ifdef Q_OS_LINUX
    registerAppImageLinks();
#endif
    const auto args = arguments();
    const QString link = args.size() == 2 && DeepLink::isLink(args.at(1)) ? args.at(1) : QString();
    const auto dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(dataPath);
    const auto serverName = QStringLiteral("starc-links-")
        + QString::fromLatin1(QCryptographicHash::hash(dataPath.toUtf8(), QCryptographicHash::Sha256)
                                 .toHex().left(24));
    auto forward = [&] {
        if (link.isEmpty() || link.size() > 4096 || link.contains(QLatin1Char('\n'))) {
            return false;
        }
        QLocalSocket socket;
        socket.connectToServer(serverName);
        if (!socket.waitForConnected(1000)) {
            return false;
        }
        socket.write(link.toUtf8() + '\n');
        if (!socket.waitForBytesWritten(1000)) {
            return false;
        }
        const bool ready = socket.bytesAvailable() || socket.waitForReadyRead(3000);
        const auto reply = socket.readAll();
        return ready && reply == QByteArray("ok");
    };
    if (forward()) {
        return true;
    }
    d->linkServerLock.reset(new QLockFile(dataPath + QStringLiteral("/deep-links.lock")));
    d->linkServerLock->setStaleLockTime(0);
    if (!d->linkServerLock->tryLock(1500)) {
        // Another instance may have been starting when the first connection was attempted.
        return forward();
    }
    QLocalServer::removeServer(serverName);
    d->linkServer.setSocketOptions(QLocalServer::UserAccessOption);
    if (!d->linkServer.listen(serverName)) {
        d->linkServerLock.reset();
        return false;
    }
    connect(&d->linkServer, &QLocalServer::newConnection, this, [this] {
        while (auto socket = d->linkServer.nextPendingConnection()) {
            socket->setReadBufferSize(16385);
            connect(socket, &QLocalSocket::disconnected, socket, &QObject::deleteLater);
            QTimer::singleShot(5000, socket, [socket] { socket->disconnectFromServer(); });
            auto receive = [this, socket] {
                if (socket->bytesAvailable() > 16384) {
                    socket->disconnectFromServer();
                    return;
                }
                if (!socket->canReadLine()) {
                    return;
                }
                const auto value = QString::fromUtf8(socket->readLine()).trimmed();
                if (!DeepLink::isLink(value) || value.size() > 4096) {
                    socket->disconnectFromServer();
                    return;
                }
                if (d->started) {
                    auto manager = qobject_cast<ManagementLayer::IApplicationManager*>(d->applicationManager);
                    manager->openProject(value);
                } else {
                    d->fileToOpen = value;
                }
                socket->write("ok");
                socket->disconnectFromServer();
            };
            connect(socket, &QLocalSocket::readyRead, this, receive);
            receive();
        }
    });
    return false;
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
        const auto value = fileOpenEvent->url().isLocalFile()
            ? normalizePath(fileOpenEvent->url().toLocalFile())
            : fileOpenEvent->url().toString(QUrl::FullyEncoded);
        if (d->started) {
            auto manager = qobject_cast<ManagementLayer::IApplicationManager*>(d->applicationManager);
            manager->openProject(value);
        } else {
            d->fileToOpen = value;
        }

        return true;
    }

    return QApplication::event(_event);
}

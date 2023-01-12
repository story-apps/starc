#include "writing_session_manager.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/writing_session/writing_sprint_panel.h>

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QKeyEvent>
#include <QStandardPaths>
#include <QUuid>
#include <QWidget>


namespace ManagementLayer {

namespace {

/**
 * @brief Путь с файлом локальных сессий
 */
QString sessionsFilePath()
{
    const QString appDataFolderPath
        = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    const QString sessionsFilePath = appDataFolderPath + QDir::separator() + "sessions.csv";
    return sessionsFilePath;
}

} // namespace

class WritingSessionManager::Implementation
{
public:
    explicit Implementation(QWidget* _parentWidget);

    /**
     * @brief Сохранить информацию о текущей сессии
     */
    void saveCurrentSession();


    QUuid sessionUuid;
    QUuid projectUuid;
    QString projectName;
    QDateTime sessionStartedAt;
    bool isCountingEnabled = true;
    int words = 0;
    int characters = 0;
    bool isLastCharacterSpace = true;

    bool isSprintStarted = false;
    int sprintWords = 0;

    Ui::WritingSprintPanel* writingSprintPanel = nullptr;
};

WritingSessionManager::Implementation::Implementation(QWidget* _parentWidget)
    : writingSprintPanel(new Ui::WritingSprintPanel(_parentWidget))
{
    writingSprintPanel->hide();
}

void WritingSessionManager::Implementation::saveCurrentSession()
{
    if (sessionUuid.isNull()) {
        return;
    }

    auto sessionEndsAt = QDateTime::currentDateTimeUtc();

    //
    // FIXME: Потенциально тут в файл может записаться хрень, если перед выключением компьютера были
    //        запущены несколько копий приложения и они начнут писать одновременно
    //
    QFile sessionsFile(sessionsFilePath());
    if (sessionsFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        sessionsFile.write(
            QString("%1;%2;%3;%4;%5;%6;%7;%8;\n")
                .arg(DataStorageLayer::StorageFacade::settingsStorage()->accountEmail(),
                     sessionUuid.toString(), projectUuid.toString(), projectName,
                     sessionStartedAt.toString(Qt::ISODateWithMs),
                     sessionEndsAt.toString(Qt::ISODateWithMs))
                .arg(words)
                .arg(characters)
                .toUtf8());
        sessionsFile.close();
    }
}


// ****


WritingSessionManager::WritingSessionManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(_parentWidget))
{
    connect(d->writingSprintPanel, &Ui::WritingSprintPanel::sprintStarted, this, [this] {
        d->isSprintStarted = true;
        d->sprintWords = 0;
        d->isLastCharacterSpace = true;
    });
    connect(d->writingSprintPanel, &Ui::WritingSprintPanel::sprintFinished, this, [this] {
        d->isSprintStarted = false;
        d->writingSprintPanel->setResult(d->sprintWords);
    });
}

WritingSessionManager::~WritingSessionManager() = default;

void WritingSessionManager::addKeyPressEvent(QKeyEvent* _event)
{
    //
    // Обрабатываем событие только в случае, если
    // - виджет в фокусе
    // - можно вводить текст
    // - включён подсчёт статистики
    //
    if (QApplication::focusWidget()
        && !QApplication::focusWidget()->testAttribute(Qt::WA_InputMethodEnabled)
        && !d->isCountingEnabled) {
        return;
    }

    //
    // Пробел и переносы строк интерпретируем, как разрыв слова
    //
    if (_event->key() == Qt::Key_Space || _event->key() == Qt::Key_Tab
        || _event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return) {
        if (!d->isLastCharacterSpace) {
            d->isLastCharacterSpace = true;
            ++d->words;
            ++d->sprintWords;
        }
    }
    //
    // В остальных случаях, если есть текст, накручиваем счётчики
    //
    else if (!_event->text().isEmpty()) {
        if (d->isLastCharacterSpace) {
            d->isLastCharacterSpace = false;
        }
        d->characters += _event->text().length();
    }
}

void WritingSessionManager::startSession(const QUuid& _projectUuid, const QString& _projectName)
{
    d->sessionUuid = QUuid::createUuid();
    d->projectUuid = _projectUuid;
    d->projectName = _projectName;
    d->sessionStartedAt = QDateTime::currentDateTimeUtc();
}

void WritingSessionManager::setCountingEnabled(bool _enabled)
{
    d->isCountingEnabled = _enabled;
}

void WritingSessionManager::finishSession()
{
    d->saveCurrentSession();

    d->sessionUuid = {};
    d->projectUuid = {};
    d->projectName.clear();
    d->sessionStartedAt = {};
    d->words = 0;
    d->characters = 0;
    d->isLastCharacterSpace = true;
    d->sprintWords = 0;
}

void WritingSessionManager::showSprintPanel()
{
    d->writingSprintPanel->showPanel();
}

} // namespace ManagementLayer

#include "writing_session_manager.h"

#include <business_layer/plots/abstract_plot.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/session_statistics/session_statistics_navigator.h>
#include <ui/session_statistics/session_statistics_tool_bar.h>
#include <ui/session_statistics/session_statistics_view.h>
#include <ui/writing_session/writing_sprint_panel.h>
#include <utils/helpers/color_helper.h>

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
    Implementation(WritingSessionManager* _q, QWidget* _parentWidget);

    /**
     * @brief Сохранить информацию о текущей сессии
     */
    void saveCurrentSession(const QDateTime& _sessionEndedAt = {});

    /**
     * @brief Облатоботать статистику
     */
    void processStatistics();


    WritingSessionManager* q = nullptr;

    Ui::SessionStatisticsToolBar* toolBar = nullptr;
    Ui::SessionStatisticsNavigator* navigator = nullptr;
    Ui::SessionStatisticsView* view = nullptr;

    QUuid sessionUuid;
    QUuid projectUuid;
    QString projectName;
    QDateTime sessionStartedAt;
    bool isCountingEnabled = true;
    int words = 0;
    int characters = 0;
    bool isLastCharacterSpace = true;

    /**
     * @brief Аккумулированные данные за последние 30 дней <min,max>
     */
    struct {
        QPair<std::chrono::seconds, std::chrono::seconds> duration;
        QPair<int, int> words;
    } last30DaysOverview;

    /**
     * @brief Графики статистики
     */
    BusinessLayer::Plot plot;

    bool isSprintStarted = false;
    int sprintWords = 0;

    Ui::WritingSprintPanel* writingSprintPanel = nullptr;
};

WritingSessionManager::Implementation::Implementation(WritingSessionManager* _q,
                                                      QWidget* _parentWidget)
    : q(_q)
    , toolBar(new Ui::SessionStatisticsToolBar(_parentWidget))
    , navigator(new Ui::SessionStatisticsNavigator(_parentWidget))
    , view(new Ui::SessionStatisticsView(_parentWidget))
    , writingSprintPanel(new Ui::WritingSprintPanel(_parentWidget))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
    writingSprintPanel->hide();
}

void WritingSessionManager::Implementation::saveCurrentSession(const QDateTime& _sessionEndedAt)
{
    if (sessionUuid.isNull()) {
        return;
    }

    auto sessionEndsAt
        = _sessionEndedAt.isValid() ? _sessionEndedAt : QDateTime::currentDateTimeUtc();

    //
    // FIXME: Потенциально тут в файл может записаться хрень, если перед выключением компьютера были
    //        запущены несколько копий приложения и они начнут писать одновременно
    //
    QFile sessionsFile(sessionsFilePath());
    if (sessionsFile.open(QIODevice::WriteOnly | QIODevice::Append)) {
        sessionsFile.write(
            QString("%1;%2;%3;%4;%5;%6;%7;%8;%9;%10\n")
                .arg(DataStorageLayer::StorageFacade::settingsStorage()->accountEmail(),
                     sessionUuid.toString(), projectUuid.toString(), projectName,
                     settingsValue(DataStorageLayer::kDeviceUuidKey).toString(),
                     QSysInfo::prettyProductName(), sessionStartedAt.toString(Qt::ISODateWithMs),
                     sessionEndsAt.toString(Qt::ISODateWithMs))
                .arg(words)
                .arg(characters)
                .toUtf8());
        sessionsFile.close();
    }

    //
    // Уведомляем клиентов о том, что была создаана новая сессия
    //
    emit q->sessionStatisticsPublishRequested({ {
        sessionUuid,
        projectUuid,
        projectName,
        settingsValue(DataStorageLayer::kDeviceUuidKey).toString(),
        QSysInfo::prettyProductName(),
        sessionStartedAt,
        sessionEndsAt,
        words,
        characters,
    } });
}

void WritingSessionManager::Implementation::processStatistics()
{
    //
    // Читаем статистику из файла
    //
    QFile sessionsFile(sessionsFilePath());
    if (!sessionsFile.open(QIODevice::ReadOnly)) {
        return;
    }
    //
    QVector<Domain::SessionStatistics> sessionStatistics;
    while (!sessionsFile.atEnd()) {
        const auto sessionData = sessionsFile.readLine().split(';');
        if (sessionData.size() != 10) {
            continue;
        }

        if (!sessionData.constFirst().isEmpty()
            && sessionData.constFirst()
                != DataStorageLayer::StorageFacade::settingsStorage()->accountEmail()) {
            continue;
        }

        const Domain::SessionStatistics session{
            sessionData[1],
            sessionData[2],
            sessionData[3],
            sessionData[4],
            sessionData[5],
            QDateTime::fromString(sessionData[6], Qt::ISODateWithMs).toLocalTime(),
            QDateTime::fromString(sessionData[7], Qt::ISODateWithMs).toLocalTime(),
            sessionData[8].toInt(),
            sessionData[9].toInt(),
        };

        //
        // ... отсеиваем всё, что меньше 10 минут и меньше 100 слов
        //
        if (session.startDateTime.secsTo(session.endDateTime) < 10 * 60 && session.words < 100) {
            continue;
        }

        sessionStatistics.append(session);
    }
    //
    // ... сортируем по времени
    //
    std::sort(sessionStatistics.begin(), sessionStatistics.end(),
              [](const Domain::SessionStatistics& _lhs, const Domain::SessionStatistics& _rhs) {
                  return _lhs.startDateTime < _rhs.startDateTime;
              });

    //
    // Бежим по статистике и формируем график
    //
    // ... х - общий для всех
    QVector<uint> x;
    // ... y
    QVector<int> summaryY;
    QHash<QString, QVector<int>> deviceY;
    QHash<QString, QString> deviceNames;
    QPair<std::chrono::seconds, std::chrono::seconds> durationOverview
        = { std::chrono::hours{ 24 }, {} };
    QPair<int, int> wordsOverview = { std::numeric_limits<int>::max(), 0 };
    //
    // ... сначала собираем полный х, список девайсов и сводку
    //
    uint lastX = 0;
    const auto overviewStart = QDateTime::currentDateTime().addDays(-30);
    for (const auto& session : std::as_const(sessionStatistics)) {
        const auto newX = session.startDateTime.date().startOfDay().toTime_t();
        if (newX != lastX) {
            x.append(newX);
            lastX = newX;
        }

        if (!deviceY.contains(session.deviceUuid)) {
            deviceY.insert(session.deviceUuid, {});
        }

        deviceNames[session.deviceUuid] = session.deviceName;

        if (overviewStart < session.startDateTime) {
            durationOverview.first = std::min(
                durationOverview.first,
                std::chrono::seconds{ session.startDateTime.secsTo(session.endDateTime) });
            durationOverview.second = std::max(
                durationOverview.second,
                std::chrono::seconds{ session.startDateTime.secsTo(session.endDateTime) });

            //
            // ... не учитываем пустые сессии
            //
            if (session.words > 0) {
                wordsOverview.first = std::min(wordsOverview.first, session.words);
                wordsOverview.second = std::max(wordsOverview.second, session.words);
            }
        }
    }
    //
    // ... затем собираем детальную стату по девайсам
    //
    int sessionsStatisticsIndex = 0;
    QMap<qreal, QStringList> info;
    for (const auto nextX : x) {
        summaryY.append(0);
        for (auto& y : deviceY) {
            y.append(0);
        }

        for (; sessionsStatisticsIndex < sessionStatistics.size(); ++sessionsStatisticsIndex) {
            const auto& session = sessionStatistics[sessionsStatisticsIndex];
            const auto newX = session.startDateTime.date().startOfDay().toTime_t();
            if (newX != nextX) {
                break;
            }

            deviceY[session.deviceUuid].last() += session.words;
            summaryY.last() += session.words;
        }

        const auto infoTitle = QDateTime::fromTime_t(nextX).toString("dd.MM.yyyy");
        QString infoText;
        for (auto iter = deviceNames.begin(); iter != deviceNames.end(); ++iter) {
            const auto words = deviceY[iter.key()].constLast();
            if (words > 0) {
                infoText.append(QString("%1: %2\n").arg(iter.value()).arg(words));
            }
        }
        infoText.append(QString("%1: %2").arg(tr("Total words")).arg(summaryY.constLast()));
        info.insert(nextX, { infoTitle, infoText });
    }
    //
    // ... формируем графики
    //
    plot = {};
    plot.info = info;
    if (view->showDevices()) {
        int plotIndex = 0;
        QVector<qreal> lastY;
        for (auto iter = deviceNames.begin(); iter != deviceNames.end(); ++iter) {
            BusinessLayer::PlotData data;
            data.name = iter.value();
            data.color = ColorHelper::forNumber(plotIndex++);
            data.brushColor
                = ColorHelper::transparent(data.color, Ui::DesignSystem::elevationEndOpacity());
            data.x = { x.begin(), x.end() };
            QVector<qreal> dataY = { deviceY[iter.key()].begin(), deviceY[iter.key()].end() };
            std::transform(dataY.begin(), dataY.end(), lastY.begin(), std::back_inserter(data.y),
                           std::plus<qreal>());
            plot.data.prepend(data);

            lastY = data.y;
        }
    } else {
        BusinessLayer::PlotData data;
        data.color = Ui::DesignSystem::color().accent();
        data.brushColor
            = ColorHelper::transparent(data.color, Ui::DesignSystem::elevationEndOpacity());
        data.x = { x.begin(), x.end() };
        data.y = { summaryY.begin(), summaryY.end() };
        plot.data.append(data);
    }

    //
    // Сохраняем данные для последующего отображения
    //
    last30DaysOverview.duration = durationOverview;
    last30DaysOverview.words = wordsOverview;
}


// ****


WritingSessionManager::WritingSessionManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget))
{
    connect(d->toolBar, &Ui::SessionStatisticsToolBar::backPressed, this,
            &WritingSessionManager::closeSessionStatisticsRequested);
    connect(d->navigator, &Ui::SessionStatisticsNavigator::aboutToBeAppeared, this, [this] {
        d->navigator->setCurrentSessionDetails(d->words, d->sessionStartedAt.toLocalTime());

        d->processStatistics();
        d->navigator->set30DaysOverviewDetails(
            d->last30DaysOverview.duration.first, d->last30DaysOverview.duration.second,
            d->last30DaysOverview.words.first, d->last30DaysOverview.words.second);
        d->view->setPlot(d->plot);
    });
    connect(d->view, &Ui::SessionStatisticsView::viewPreferencesChanged, this, [this] {
        d->processStatistics();
        d->view->setPlot(d->plot);
    });

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

QWidget* WritingSessionManager::toolBar() const
{
    return d->toolBar;
}

QWidget* WritingSessionManager::navigator() const
{
    return d->navigator;
}

QWidget* WritingSessionManager::view() const
{
    return d->view;
}

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
    // Для случая, когда сессия была прервана по долгому ожиданию, возобновляем её
    //
    if (!d->sessionStartedAt.isValid()) {
        d->sessionStartedAt = QDateTime::currentDateTimeUtc();
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

void WritingSessionManager::splitSession(const QDateTime& _lastActionAt)
{
    d->saveCurrentSession(_lastActionAt);

    d->sessionStartedAt = {};
    d->words = 0;
    d->characters = 0;
    d->isLastCharacterSpace = true;
    d->sprintWords = 0;
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

void WritingSessionManager::setSessionStatistics(
    const QVector<Domain::SessionStatistics>& _sessionStatistics)
{
    qDebug("hehe");
}

} // namespace ManagementLayer

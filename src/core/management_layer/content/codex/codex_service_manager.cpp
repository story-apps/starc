#include "codex_service_manager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QProcess>
#include <QProcessEnvironment>
#include <QQueue>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>
#include <QDebug>


namespace ManagementLayer {

namespace {

const QString kStoryAssistantSkillSettingsKey = "codex/story-assistant/skill";
const QString kDefaultStoryAssistantSkill = "edit-story";
const QString kEricEdsonStorySkill = "eric-edson-story-skill";

QString selectedStoryAssistantSkill()
{
    const auto selected
        = QSettings().value(kStoryAssistantSkillSettingsKey,
                            kDefaultStoryAssistantSkill).toString();
    return selected == kEricEdsonStorySkill ? kEricEdsonStorySkill
                                            : kDefaultStoryAssistantSkill;
}

QString joinedSections(const QVector<QString>& _sections, const QString& _label)
{
    QString result;
    for (int index = 0; index < _sections.size(); ++index) {
        if (!result.isEmpty()) {
            result.append("\n\n");
        }
        result.append(QString("--- %1 %2 ---\n%3").arg(_label).arg(index + 1).arg(_sections.at(index)));
    }
    return result;
}

bool isStoryboardPrompt(const QString& _prompt)
{
    const auto prompt = _prompt.toLower();
    return prompt.contains("storyboard") || prompt.contains("story board")
        || prompt.contains("beat board") || prompt.contains("beat-board")
        || prompt.contains("beat breakdown") || prompt.contains("nine-grid")
        || prompt.contains("nine grid") || prompt.contains("sequence board")
        || prompt.contains("shot list");
}

QString withoutMarkdownFence(QString _text)
{
    _text = _text.trimmed();
    if (!_text.startsWith("```")) {
        return _text;
    }

    const auto firstLineEnd = _text.indexOf('\n');
    const auto lastFence = _text.lastIndexOf("```");
    if (firstLineEnd >= 0 && lastFence > firstLineEnd) {
        return _text.mid(firstLineEnd + 1, lastFence - firstLineEnd - 1).trimmed();
    }
    return _text;
}

} // namespace

class CodexServiceManager::Implementation
{
public:
    enum class Operation {
        Rephrase,
        Expand,
        Shorten,
        Insert,
        Summarize,
        Translate,
        TranslateDocument,
        Synopsis,
        Novel,
        Script,
        GenerateText,
        StoryAssist,
        StoryEdit,
        Storyboard,
    };

    enum class State {
        Stopped,
        Starting,
        Initializing,
        Ready,
        StartingThread,
        RunningTurn,
    };

    struct Task {
        Operation operation = Operation::GenerateText;
        QString prompt;
        int expectedSections = 0;
        QString storySkill = kDefaultStoryAssistantSkill;
        bool usesStoryActionProtocol = false;
    };

    explicit Implementation(CodexServiceManager* _q)
        : q(_q)
    {
        process.setProcessChannelMode(QProcess::SeparateChannels);
        QObject::connect(&process, &QProcess::started, q, [this] { initialize(); });
        QObject::connect(&process, &QProcess::readyReadStandardOutput, q,
                         [this] { readStandardOutput(); });
        QObject::connect(&process, &QProcess::readyReadStandardError, q, [this] {
            const auto message = QString::fromUtf8(process.readAllStandardError()).trimmed();
            if (!message.isEmpty()) {
                qWarning().noquote() << "Codex App Server:" << message;
            }
        });
        QObject::connect(&process, &QProcess::errorOccurred, q,
                         [this](QProcess::ProcessError _error) {
                             Q_UNUSED(_error)
                             stopWithError(QObject::tr("Codex could not be started: %1")
                                               .arg(process.errorString()));
                         });
        QObject::connect(
            &process, qOverload<int, QProcess::ExitStatus>(&QProcess::finished), q,
            [this](int _exitCode, QProcess::ExitStatus _status) {
                if (state == State::Stopped) {
                    return;
                }
                stopWithError(QObject::tr("Codex App Server stopped unexpectedly (%1, %2).")
                                  .arg(_exitCode)
                                  .arg(static_cast<int>(_status)));
            });
    }

    ~Implementation()
    {
        state = State::Stopped;
        if (process.state() != QProcess::NotRunning) {
            process.terminate();
            if (!process.waitForFinished(1000)) {
                process.kill();
                process.waitForFinished(1000);
            }
        }
    }

    QString codexExecutable() const
    {
        const auto configured = qEnvironmentVariable("STARC_CODEX_BIN");
        if (!configured.isEmpty() && QFileInfo(configured).isExecutable()) {
            return configured;
        }

        const auto onPath = QStandardPaths::findExecutable("codex");
        if (!onPath.isEmpty()) {
            return onPath;
        }

        const QStringList candidates = {
            QDir::home().filePath(".local/bin/codex"),
            QLatin1String("/opt/homebrew/bin/codex"),
            QLatin1String("/usr/local/bin/codex"),
        };
        for (const auto& candidate : candidates) {
            if (QFileInfo(candidate).isExecutable()) {
                return candidate;
            }
        }
        return {};
    }

    QString workspacePath() const
    {
        const auto configured = qEnvironmentVariable("STARC_CODEX_WORKSPACE");
        const QStringList candidates = {
            configured,
            QDir(QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation))
                .filePath("GitHub/AI-Storyboard"),
            QDir::currentPath(),
        };
        for (const auto& candidate : candidates) {
            if (!candidate.isEmpty()
                && QDir(candidate).exists(QLatin1String(".agents/skills"))) {
                return QDir(candidate).absolutePath();
            }
        }
        return QDir::currentPath();
    }

    QString storyboardSkillPath() const
    {
        return QDir(workspacePath()).filePath(".agents/skills/create-storyboard/SKILL.md");
    }

    QString storySkillPath(const QString& _skillName) const
    {
        const auto skillName = _skillName == kEricEdsonStorySkill
            ? kEricEdsonStorySkill
            : kDefaultStoryAssistantSkill;
        return QDir(workspacePath()).filePath(
            QString(".agents/skills/%1/SKILL.md").arg(skillName));
    }

    void setActivity(const QString& _activity)
    {
        if (activity == _activity) {
            return;
        }
        activity = _activity;
        emit q->activityChanged(activity);
    }

    void enqueue(Task _task)
    {
        const bool wasIdle = tasks.isEmpty() && !hasCurrentTask;
        tasks.enqueue(std::move(_task));
        if (wasIdle) {
            emit q->busyChanged(true);
            setActivity(QObject::tr("Preparing request…"));
        }
        if (state == State::Stopped) {
            start();
        } else if (state == State::Ready) {
            startNextTask();
        }
    }

    void start()
    {
        const auto executable = codexExecutable();
        if (executable.isEmpty()) {
            stopWithError(QObject::tr(
                "Codex CLI was not found. Install Codex or set STARC_CODEX_BIN to its path."));
            return;
        }

        state = State::Starting;
        setActivity(QObject::tr("Starting Codex…"));
        process.setProgram(executable);
        process.setArguments({ QLatin1String("app-server") });
        process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        process.start();
    }

    void initialize()
    {
        state = State::Initializing;
        setActivity(QObject::tr("Connecting to Codex…"));
        initializeRequestId = nextRequestId++;
        send({ { "method", "initialize" },
               { "id", initializeRequestId },
               { "params",
                 QJsonObject{
                     { "clientInfo",
                       QJsonObject{
                           { "name", "starc_local" },
                           { "title", "Story Architect" },
                           { "version", QCoreApplication::applicationVersion() },
                       } },
                     { "capabilities", QJsonValue::Null },
                 } } });
    }

    void startNextTask()
    {
        if (state != State::Ready || tasks.isEmpty()) {
            return;
        }

        currentTask = tasks.dequeue();
        hasCurrentTask = true;
        cancelRequested = false;
        response.clear();
        currentThreadId.clear();
        currentTurnId.clear();
        state = State::StartingThread;
        setActivity(currentTask.operation == Operation::Storyboard
                        ? QObject::tr("Preparing storyboard context…")
                        : QObject::tr("Preparing story context…"));

        threadStartRequestId = nextRequestId++;
        const bool writesStoryboard = currentTask.operation == Operation::Storyboard;
        send({ { "method", "thread/start" },
               { "id", threadStartRequestId },
               { "params",
                 QJsonObject{
                     { "cwd", workspacePath() },
                     { "approvalPolicy", "never" },
                     { "sandbox", writesStoryboard ? "workspace-write" : "read-only" },
                     { "ephemeral", true },
                     { "developerInstructions",
                       "You are the local Codex assistant embedded in Story Architect. Follow the "
                       "requested writing operation exactly. Preserve screenplay facts and return "
                       "only the requested content. Do not wrap ordinary text or Fountain output "
                       "in Markdown fences. For screenplay output, use real Fountain scene "
                       "headings such as 'INT. KITCHEN - NIGHT' or 'EXT. CITY STREET - DAY'. "
                       "Never use Markdown or numbered labels such as '# Scene 1' as screenplay "
                       "content. When a storyboard skill is attached, follow it and "
                       "finish with a concise report containing the artifact path." },
                 } } });
    }

    void startTurn()
    {
        if (cancelRequested) {
            completeCancellation();
            return;
        }
        state = State::RunningTurn;
        setActivity(QObject::tr("Thinking…"));
        turnStartRequestId = nextRequestId++;

        QJsonArray input;
        if (currentTask.operation == Operation::Storyboard) {
            const auto skillPath = storyboardSkillPath();
            if (QFileInfo::exists(skillPath)) {
                input.append(QJsonObject{
                    { "type", "skill" },
                    { "name", "create-storyboard" },
                    { "path", skillPath },
                });
            }
        } else if (currentTask.operation == Operation::StoryAssist
                   || currentTask.operation == Operation::StoryEdit) {
            const auto skillName = currentTask.storySkill == kEricEdsonStorySkill
                ? kEricEdsonStorySkill
                : kDefaultStoryAssistantSkill;
            const auto skillPath = storySkillPath(skillName);
            if (QFileInfo::exists(skillPath)) {
                input.append(QJsonObject{
                    { "type", "skill" },
                    { "name", skillName },
                    { "path", skillPath },
                });
            }
        }
        input.append(QJsonObject{
            { "type", "text" },
            { "text", currentTask.prompt },
            { "text_elements", QJsonArray{} },
        });

        QJsonObject params{
            { "threadId", currentThreadId },
            { "input", input },
        };
        if (currentTask.operation == Operation::TranslateDocument) {
            params.insert(
                "outputSchema",
                QJsonObject{
                    { "type", "object" },
                    { "properties",
                      QJsonObject{
                          { "sections",
                            QJsonObject{
                                { "type", "array" },
                                { "items", QJsonObject{ { "type", "string" } } },
                                { "minItems", currentTask.expectedSections },
                                { "maxItems", currentTask.expectedSections },
                            } },
                      } },
                    { "required", QJsonArray{ "sections" } },
                    { "additionalProperties", false },
                });
        } else if (currentTask.usesStoryActionProtocol) {
            params.insert(
                "outputSchema",
                QJsonObject{
                    { "type", "object" },
                    { "properties",
                      QJsonObject{
                          { "version", QJsonObject{ { "type", "integer" },
                                                     { "enum", QJsonArray{ 3 } } } },
                          { "action",
                            QJsonObject{
                                { "type", "string" },
                                { "enum",
                                  QJsonArray{
                                      "answer",
                                      "suggest_ideas",
                                      "insert_screenplay",
                                      "replace_selection",
                                      "delete_selection",
                                      "clear_screenplay",
                                      "update_logline",
                                      "replace_synopsis",
                                      "revise_treatment",
                                      "create_character",
                                      "update_character",
                                      "remove_character",
                                      "merge_character",
                                      "update_character_relationship",
                                      "update_story_memory",
                                      "request_clarification",
                                  } },
                            } },
                          { "target",
                            QJsonObject{
                                { "type", "string" },
                                { "enum",
                                  QJsonArray{ "none", "selection", "cursor", "beginning",
                                              "end", "logline", "synopsis", "treatment",
                                              "characters", "character_relationships",
                                              "story_memory" } },
                            } },
                          { "content", QJsonObject{ { "type", "string" } } },
                          { "summary", QJsonObject{ { "type", "string" } } },
                          { "requiresApproval", QJsonObject{ { "type", "boolean" } } },
                          { "entityId", QJsonObject{ { "type", "string" } } },
                          { "entityName", QJsonObject{ { "type", "string" } } },
                          { "fieldChanges",
                            QJsonObject{
                                { "type", "array" },
                                { "maxItems", 24 },
                                { "items",
                                  QJsonObject{
                                      { "type", "object" },
                                      { "properties",
                                        QJsonObject{
                                            { "field",
                                              QJsonObject{
                                                  { "type", "string" },
                                                  { "enum",
                                                    QJsonArray{
                                                        "name", "story_role", "age", "nickname",
                                                        "one_sentence_description",
                                                        "long_description", "family", "personality",
                                                        "motivation", "moral", "greatest_fear",
                                                        "secrets", "short_term_goal", "long_term_goal",
                                                        "initial_beliefs", "changed_beliefs",
                                                        "plot_involvement", "conflict", "speech",
                                                        "related_character_id",
                                                        "merge_source_character_id", "feeling",
                                                        "details",
                                                    } },
                                              } },
                                            { "value", QJsonObject{ { "type", "string" } } },
                                        } },
                                      { "required", QJsonArray{ "field", "value" } },
                                      { "additionalProperties", false },
                                  } },
                            } },
                          { "impactSummary", QJsonObject{ { "type", "string" } } },
                          { "continuityChecks",
                            QJsonObject{
                                { "type", "array" },
                                { "maxItems", 12 },
                                { "items",
                                  QJsonObject{
                                      { "type", "object" },
                                      { "properties",
                                        QJsonObject{
                                            { "severity",
                                              QJsonObject{
                                                  { "type", "string" },
                                                  { "enum",
                                                    QJsonArray{ "critical", "caution",
                                                                "suggestion" } },
                                              } },
                                            { "category", QJsonObject{ { "type", "string" } } },
                                            { "issue", QJsonObject{ { "type", "string" } } },
                                            { "evidence", QJsonObject{ { "type", "string" } } },
                                        } },
                                      { "required",
                                        QJsonArray{ "severity", "category", "issue",
                                                    "evidence" } },
                                      { "additionalProperties", false },
                                  } },
                            } },
                      } },
                    { "required",
                      QJsonArray{ "version", "action", "target", "content", "summary",
                                  "requiresApproval", "entityId", "entityName", "fieldChanges",
                                  "impactSummary", "continuityChecks" } },
                    { "additionalProperties", false },
                });
        }

        send({ { "method", "turn/start" }, { "id", turnStartRequestId }, { "params", params } });
    }

    void send(const QJsonObject& _message)
    {
        if (process.state() == QProcess::NotRunning) {
            stopWithError(QObject::tr("Codex App Server is not running."));
            return;
        }
        process.write(QJsonDocument(_message).toJson(QJsonDocument::Compact));
        process.write("\n");
    }

    void readStandardOutput()
    {
        stdoutBuffer.append(process.readAllStandardOutput());
        while (true) {
            const auto lineEnd = stdoutBuffer.indexOf('\n');
            if (lineEnd < 0) {
                break;
            }

            const auto line = stdoutBuffer.left(lineEnd).trimmed();
            stdoutBuffer.remove(0, lineEnd + 1);
            if (line.isEmpty()) {
                continue;
            }

            QJsonParseError parseError;
            const auto document = QJsonDocument::fromJson(line, &parseError);
            if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
                qWarning().noquote() << "Invalid Codex App Server message:" << line;
                continue;
            }
            handleMessage(document.object());
        }
    }

    void handleMessage(const QJsonObject& _message)
    {
        if (_message.contains("id")) {
            const auto id = _message.value("id").toInt();
            if (_message.contains("error")) {
                const auto error = _message.value("error").toObject();
                failCurrentTask(error.value("message").toString(
                    QObject::tr("Codex rejected the request.")));
                return;
            }

            if (id == initializeRequestId) {
                send({ { "method", "initialized" }, { "params", QJsonObject{} } });
                state = State::Ready;
                startNextTask();
                return;
            }
            if (id == threadStartRequestId && state == State::StartingThread) {
                currentThreadId = _message.value("result")
                                      .toObject()
                                      .value("thread")
                                      .toObject()
                                      .value("id")
                                      .toString();
                if (currentThreadId.isEmpty()) {
                    failCurrentTask(QObject::tr("Codex did not return a thread identifier."));
                } else {
                    startTurn();
                }
                return;
            }
            if (id == turnStartRequestId && state == State::RunningTurn) {
                currentTurnId = _message.value("result")
                                    .toObject()
                                    .value("turn")
                                    .toObject()
                                    .value("id")
                                    .toString();
                if (cancelRequested && !currentTurnId.isEmpty()) {
                    send({ { "method", "turn/interrupt" },
                           { "id", nextRequestId++ },
                           { "params",
                             QJsonObject{
                                 { "threadId", currentThreadId },
                                 { "turnId", currentTurnId },
                             } } });
                }
                return;
            }
        }

        const auto method = _message.value("method").toString();
        const auto params = _message.value("params").toObject();
        if (method == QLatin1String("turn/started")
            && params.value("threadId").toString() == currentThreadId) {
            currentTurnId = params.value("turn").toObject().value("id").toString();
            setActivity(QObject::tr("Thinking…"));
            return;
        }
        if (method == QLatin1String("item/started")
            && params.value("threadId").toString() == currentThreadId) {
            const auto itemType = params.value("item").toObject().value("type").toString();
            if (itemType == QLatin1String("reasoning")) {
                setActivity(QObject::tr("Connecting story threads…"));
            } else if (itemType == QLatin1String("commandExecution")) {
                setActivity(QObject::tr("Working with story files…"));
            } else if (itemType == QLatin1String("fileChange")) {
                setActivity(QObject::tr("Writing story artifacts…"));
            } else if (itemType == QLatin1String("webSearch")) {
                setActivity(QObject::tr("Researching…"));
            }
            return;
        }
        if (method == QLatin1String("item/reasoning/summaryTextDelta")
            && params.value("threadId").toString() == currentThreadId) {
            setActivity(QObject::tr("Checking character, plot, and voice continuity…"));
            return;
        }
        if (method == QLatin1String("item/agentMessage/delta")
            && params.value("threadId").toString() == currentThreadId) {
            setActivity(QObject::tr("Writing response…"));
            response.append(params.value("delta").toString());
            return;
        }
        if (method != QLatin1String("turn/completed")
            || params.value("threadId").toString() != currentThreadId) {
            return;
        }

        const auto turn = params.value("turn").toObject();
        const auto turnStatus = turn.value("status").toString();
        if (turnStatus == QLatin1String("interrupted") || cancelRequested) {
            completeCancellation();
            return;
        }
        if (turnStatus != QLatin1String("completed")) {
            const auto error = turn.value("error").toObject();
            failCurrentTask(error.value("message").toString(
                QObject::tr("Codex did not complete the request.")));
            return;
        }

        setActivity(QObject::tr("Finishing…"));
        if (response.isEmpty()) {
            const auto items = turn.value("items").toArray();
            for (const auto& value : items) {
                const auto item = value.toObject();
                if (item.value("type").toString() == QLatin1String("agentMessage")) {
                    response = item.value("text").toString();
                }
            }
        }
        completeCurrentTask();
    }

    void completeCurrentTask()
    {
        const auto result = response.trimmed();
        switch (currentTask.operation) {
        case Operation::Rephrase:
            emit q->textRephrased(result);
            break;
        case Operation::Expand:
            emit q->textExpanded(result);
            break;
        case Operation::Shorten:
            emit q->textShortened(result);
            break;
        case Operation::Insert:
            emit q->textInserted(result);
            break;
        case Operation::Summarize:
            emit q->textSummarizeed(result);
            break;
        case Operation::Translate:
            emit q->textTranslated(result);
            break;
        case Operation::TranslateDocument: {
            QJsonParseError error;
            const auto document
                = QJsonDocument::fromJson(withoutMarkdownFence(result).toUtf8(), &error);
            const auto sections = document.object().value("sections").toArray();
            if (error.error != QJsonParseError::NoError || sections.size() != currentTask.expectedSections) {
                failCurrentTask(QObject::tr("Codex returned an invalid document translation."));
                return;
            }
            QVector<QString> translated;
            for (const auto& section : sections) {
                translated.append(section.toString());
            }
            emit q->documentTranslated(translated);
            break;
        }
        case Operation::Synopsis:
            emit q->synopsisGenerated(result);
            break;
        case Operation::Novel:
            emit q->novelGenerated(result);
            break;
        case Operation::Script:
            emit q->scriptGenerated(result);
            break;
        case Operation::GenerateText:
        case Operation::StoryAssist:
        case Operation::StoryEdit:
            emit q->textGenerated(result);
            break;
        case Operation::Storyboard:
            emit q->storyboardGenerated(result);
            break;
        }

        hasCurrentTask = false;
        state = State::Ready;
        if (tasks.isEmpty()) {
            setActivity({});
            emit q->busyChanged(false);
        }
        QTimer::singleShot(0, q, [this] { startNextTask(); });
    }

    void completeCancellation()
    {
        cancelRequested = false;
        hasCurrentTask = false;
        response.clear();
        currentTurnId.clear();
        emit q->taskCancelled();
        state = process.state() == QProcess::Running ? State::Ready : State::Stopped;
        if (tasks.isEmpty()) {
            setActivity({});
            emit q->busyChanged(false);
        }
        if (state == State::Ready) {
            QTimer::singleShot(0, q, [this] { startNextTask(); });
        }
    }

    void cancelCurrentTask()
    {
        if (!hasCurrentTask) {
            if (!tasks.isEmpty()) {
                tasks.dequeue();
                emit q->taskCancelled();
                if (tasks.isEmpty()) {
                    setActivity({});
                    emit q->busyChanged(false);
                }
            }
            return;
        }

        cancelRequested = true;
        setActivity(QObject::tr("Stopping…"));
        if (state == State::RunningTurn && !currentThreadId.isEmpty()
            && !currentTurnId.isEmpty()) {
            send({ { "method", "turn/interrupt" },
                   { "id", nextRequestId++ },
                   { "params",
                     QJsonObject{
                         { "threadId", currentThreadId },
                         { "turnId", currentTurnId },
                     } } });
        } else if (state == State::StartingThread) {
            completeCancellation();
        }
    }

    void failCurrentTask(const QString& _message)
    {
        hasCurrentTask = false;
        emit q->errorOccurred(_message);
        if (process.state() == QProcess::Running) {
            state = State::Ready;
            QTimer::singleShot(0, q, [this] { startNextTask(); });
        } else {
            state = State::Stopped;
        }
        if (tasks.isEmpty()) {
            setActivity({});
            emit q->busyChanged(false);
        }
    }

    void stopWithError(const QString& _message)
    {
        if (state == State::Stopped && tasks.isEmpty() && !hasCurrentTask) {
            return;
        }
        state = State::Stopped;
        hasCurrentTask = false;
        tasks.clear();
        emit q->errorOccurred(_message);
        setActivity({});
        emit q->busyChanged(false);
    }

    CodexServiceManager* q = nullptr;
    QProcess process;
    QByteArray stdoutBuffer;
    QQueue<Task> tasks;
    Task currentTask;
    bool hasCurrentTask = false;
    State state = State::Stopped;
    int nextRequestId = 1;
    int initializeRequestId = 0;
    int threadStartRequestId = 0;
    int turnStartRequestId = 0;
    QString currentThreadId;
    QString currentTurnId;
    QString response;
    QString activity;
    bool cancelRequested = false;
};

CodexServiceManager::CodexServiceManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

CodexServiceManager::~CodexServiceManager() = default;

void CodexServiceManager::cancelCurrentTask()
{
    d->cancelCurrentTask();
}

void CodexServiceManager::aiRephraseText(const QString& _source, const QString& _style)
{
    d->enqueue({ Implementation::Operation::Rephrase,
                 QString("Rephrase the source text%1. Preserve meaning and return only the revised "
                         "text.\n\nSOURCE:\n%2")
                     .arg(_style.isEmpty() ? QString() : QString(" in this style: %1").arg(_style),
                          _source) });
}

void CodexServiceManager::aiExpandText(const QString& _source)
{
    d->enqueue({ Implementation::Operation::Expand,
                 QString("Expand the source naturally with useful detail. Preserve its voice and "
                         "facts. Return only the expanded text.\n\nSOURCE:\n%1")
                     .arg(_source) });
}

void CodexServiceManager::aiShortenText(const QString& _source)
{
    d->enqueue({ Implementation::Operation::Shorten,
                 QString("Shorten the source while preserving its meaning and voice. Return only "
                         "the shortened text.\n\nSOURCE:\n%1")
                     .arg(_source) });
}

void CodexServiceManager::aiInsertText(const QString& _after, const QString& _before)
{
    d->enqueue({ Implementation::Operation::Insert,
                 QString("Write the missing text that connects the two passages smoothly. Return "
                         "only the inserted text.\n\nBEFORE:\n%1\n\nAFTER:\n%2")
                     .arg(_after, _before) });
}

void CodexServiceManager::aiSummarizeText(const QString& _source)
{
    d->enqueue({ Implementation::Operation::Summarize,
                 QString("Summarize the source accurately and concisely. Return only the summary."
                         "\n\nSOURCE:\n%1")
                     .arg(_source) });
}

void CodexServiceManager::aiTranslateText(const QString& _source, const QString& _languageCode)
{
    d->enqueue({ Implementation::Operation::Translate,
                 QString("Translate the source into language code %1. Preserve formatting, names, "
                         "and screenplay terminology. Return only the translation.\n\nSOURCE:\n%2")
                     .arg(_languageCode, _source) });
}

void CodexServiceManager::aiTranslateDocument(const QVector<QString>& _texts,
                                              const QString& _languageCode,
                                              Domain::DocumentObjectType _type, int _wordsRequired)
{
    Q_UNUSED(_type)
    Q_UNUSED(_wordsRequired)
    d->enqueue({
        Implementation::Operation::TranslateDocument,
        QString("Translate every numbered section into language code %1. Preserve section count, "
                "order, line breaks, names, and screenplay formatting. Return a JSON object with "
                "exactly one key, sections, whose value is an array of exactly %2 translated "
                "strings.\n\n%3")
            .arg(_languageCode)
            .arg(_texts.size())
            .arg(joinedSections(_texts, "SECTION")),
        static_cast<int>(_texts.size()),
    });
}

void CodexServiceManager::aiGenerateSynopsis(const QVector<QString>& _scenes,
                                             int _maxWordsPerScene, int _wordsRequired)
{
    Q_UNUSED(_wordsRequired)
    const auto lengthInstruction
        = _maxWordsPerScene > 0
        ? QString("Use no more than %1 words per scene.").arg(_maxWordsPerScene)
        : QString("Use as much detail as needed.");
    d->enqueue({ Implementation::Operation::Synopsis,
                 QString("Create a coherent scene-by-scene screenplay synopsis. %1 Preserve plot "
                         "order, character names, and causality. Return only the synopsis.\n\n%2")
                     .arg(lengthInstruction, joinedSections(_scenes, "SCENE")) });
}

void CodexServiceManager::aiGenerateNovel(const QVector<QString>& _scenes, int _wordsRequired)
{
    Q_UNUSED(_wordsRequired)
    d->enqueue({ Implementation::Operation::Novel,
                 QString("Adapt the complete screenplay below into polished Markdown prose. "
                         "Preserve all story facts and scene order. Return only the novel text."
                         "\n\n%1")
                     .arg(joinedSections(_scenes, "SCENE")) });
}

void CodexServiceManager::aiGenerateScript(const QVector<QString>& _chapters, int _wordsRequired)
{
    Q_UNUSED(_wordsRequired)
    d->enqueue({ Implementation::Operation::Script,
                 QString("Adapt the complete source below into a production-ready screenplay in "
                         "Fountain format. Preserve story facts. Begin every scene with a full "
                         "scene heading such as 'INT. LOCATION - TIME' or 'EXT. LOCATION - TIME'. "
                         "Never use '# Scene 1', 'Scene 1', or another numbered section label in "
                         "place of a scene heading. Return only Fountain text.\n\n%1")
                     .arg(joinedSections(_chapters, "CHAPTER")) });
}

void CodexServiceManager::aiGenerateText(const QString& _promptPrefix, const QString& _prompt,
                                         const QString& _promptSuffix)
{
    if (isStoryboardPrompt(_prompt)) {
        d->enqueue({
            Implementation::Operation::Storyboard,
            QString("Use the attached create-storyboard skill. Treat the screenplay supplied below "
                    "as approved source material. Produce the storyboard stage requested by the "
                    "user, without changing the source screenplay. If no episode ID is specified, "
                    "use the next unused episode ID in the workspace.\n\nUSER REQUEST:\n%1\n\n%2")
                .arg(_prompt, _promptPrefix),
        });
        return;
    }

    const bool usesStoryActionProtocol = _promptPrefix.contains("STARC_ACTION_PROTOCOL_V3");
    const bool replacesSelection
        = !usesStoryActionProtocol && _promptPrefix.contains("SELECTED EDIT TARGET");
    const auto operation = replacesSelection ? Implementation::Operation::StoryEdit
                                             : Implementation::Operation::StoryAssist;
    const auto storySkill = selectedStoryAssistantSkill();
    const auto skillInvocation = QString("$%1").arg(storySkill);
    const auto operationInstruction
        = usesStoryActionProtocol
        ? QString(
              "%1 Use the complete story package and screenplay as canon, preserve the author's "
              "voice and continuity, and classify the user's intent using "
              "STARC_ACTION_PROTOCOL_V3. Return exactly one schema-valid action object. Use "
              "answer for story questions, suggest_ideas for collaborative possibilities, "
              "insert_screenplay for new Fountain-formatted screenplay writing, "
              "replace_selection only when an editor selection exists, delete_selection only "
              "when an editor selection exists, clear_screenplay only when the user explicitly "
              "asks to remove the whole screenplay, update_logline for a direct logline change, "
              "replace_synopsis for a direct synopsis change, revise_treatment for a direct "
              "revision of the existing Treatment-tab outline, and request_clarification when a "
              "safe target or essential detail is missing. Use create_character to add one new "
              "native Character-tab record, update_character to change fields on one existing "
              "character, remove_character to move one existing native character to STARC's "
              "Recycle Bin, and update_character_relationship to create or update one native "
              "relationship. Use merge_character only to merge one duplicate character into one "
              "surviving character. For an existing character, copy its Stable ID exactly into entityId "
              "and its live Name into entityName. For creation, entityId must be empty and "
              "entityName is the new unique name. Character fieldChanges may use only the schema "
              "fields and must contain only fields the writer asked to change; an empty string "
              "intentionally clears a field. story_role values are primary, secondary, tertiary, "
              "or undefined. Relationship fieldChanges must include related_character_id copied "
              "from the other character's Stable ID and may include feeling and details. Never "
              "invent an ID, permanently delete a character, delete a relationship, or merge "
              "characters. remove_character must use the live Stable ID and Name, empty content, "
              "and an empty fieldChanges array; the app will inventory dependencies and preserve "
              "all screenplay text. For merge_character, entityId and entityName identify the "
              "survivor; fieldChanges must include merge_source_character_id containing the "
              "duplicate's live Stable ID. For every supported story field where the duplicate "
              "has a meaningful value different from the survivor, include that field with the "
              "final value the survivor should keep. If both values are non-empty and the writer "
              "has not supplied a precedence or a safe way to combine them, return "
              "request_clarification instead of guessing. Keep content empty. The app will plan "
              "relationship and photo transfers, reassign native script cues, and preserve the "
              "duplicate in Recycle Bin. "
              "For revise_treatment, content must "
              "contain exactly one line for every existing editable treatment paragraph, in the "
              "same order, with no numbering, labels, blank wrapper lines, or commentary; never "
              "add or remove scenes through that action. Targets must be none for conversational actions, "
              "none for clear_screenplay, selection for selection actions, and cursor, beginning, "
              "or end for insertion. Targets must be logline for update_logline, synopsis for "
              "replace_synopsis, treatment for revise_treatment, and story_memory for "
              "update_story_memory. Targets must be characters for create_character, "
              "update_character, and remove_character, and character_relationships for "
              "update_character_relationship. merge_character also targets characters. "
              "For all non-character actions, return empty entityId, empty entityName, and an "
              "empty fieldChanges array. Use update_story_memory only when explicitly asked to build "
              "or refresh Story Memory. Its content must be an evidence-based continuity record "
              "with these headings: CHARACTERS & RELATIONSHIPS, CHARACTER KNOWLEDGE, TIMELINE, "
              "PLOT THREADS, SETUPS & PAYOFFS, WORLD RULES, VOICE & STYLE, CONTINUITY RISKS. "
              "Distinguish confirmed canon from inference and cite scene headings or STARC tabs "
              "as evidence. Never invent missing facts. "
              "For every editor-changing action, perform a Continuity Gate self-audit against "
              "the live screenplay, linked STARC tabs, and Story Memory. Put a concise description "
              "of story consequences in impactSummary. Put each finding in continuityChecks with "
              "severity critical only for a direct conflict with confirmed canon, caution for a "
              "likely inconsistency or weak motivation, and suggestion for optional improvement. "
              "Use character knowledge, chronology, location, world rules, setups/payoffs, and "
              "voice as categories where relevant. Evidence must cite a scene heading or linked "
              "STARC tab; explicitly say when evidence is only inference. Return an empty checks "
              "array for conversational actions and when no issue is found. "
              "Put conversational text in content. Put only production-ready Fountain in content "
              "for insert_screenplay and replace_selection—never an introduction or Markdown "
              "fence. Use complete Fountain scene headings such as 'INT. KITCHEN - NIGHT', never "
              "'# Scene 1'. Set requiresApproval true for every editor-changing action and false "
              "for every conversational action. The app, not you, will preview and apply editor "
              "changes. Set requiresApproval true for logline, synopsis, and treatment changes too.")
              .arg(skillInvocation)
        : replacesSelection
        ? QString("%1 Revise only SELECTED EDIT TARGET. Treat the complete screenplay as "
                  "canon and preserve the author's established voice, character knowledge, "
                  "relationships, timeline, world rules, setups, and payoffs.").arg(skillInvocation)
        : QString("%1 Use the complete screenplay as canon. Follow the user's request while "
                  "preserving the author's established voice and the story's character, plot, "
                  "timeline, theme, and world continuity.").arg(skillInvocation);
    Implementation::Task task;
    task.operation = operation;
    task.prompt = QString("%1\n\n%2\n\nUSER REQUEST:\n%3\n\n%4")
                      .arg(operationInstruction, _promptPrefix, _prompt, _promptSuffix);
    task.storySkill = storySkill;
    task.usesStoryActionProtocol = usesStoryActionProtocol;
    d->enqueue(std::move(task));
}

} // namespace ManagementLayer

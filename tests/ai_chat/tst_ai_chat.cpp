#include <ui/modules/ai_assistant/ai_chat_session.h>

#include <QJsonObject>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class AiChatTest : public QObject
{
    Q_OBJECT
private slots:
    void persistsAndIsolatesProjectsAndAccounts();
    void replacesDocumentButKeepsConversation();
    void routesReplyToOriginalConversation();
    void deletingPendingChatDoesNotResurrectIt();
    void failuresAndAvailability();
    void coexistsWithSessionStatistics();
    void reportsStorageFailure();
    void timeoutConsumesLateReplies();
    void legacyRequestsCannotOverlapChatRequests();
    void changingAccountKeepsOnlyCurrentDocument();
    void renamesPersistAndSurviveFirstMessage();
    void toolRepliesKeepTheirOriginAndIgnoreStaleTargets();
    void toolTimeoutAndMultipleResults();
    void clearingChatKeepsNameAndDiscardsPendingReply();
};

void AiChatTest::persistsAndIsolatesProjectsAndAccounts()
{
    QTemporaryDir dir;
    const auto path = dir.filePath("sessions.db");
    const auto project = QUuid::createUuid();
    const auto otherProject = QUuid::createUuid();
    QString first;
    QString second;
    {
        Ui::AiChatSession chat(path);
        chat.setProject(project, "one@example.com");
        chat.setBackendAvailable(true);
        QVERIFY(chat.send("Привет, обсудим документ"));
        first = chat.currentChat();
        chat.finishReply("Давайте");
        QVERIFY(chat.createChat());
        second = chat.currentChat();
        QVERIFY(chat.send("Другой разговор"));
        chat.finishReply("Хорошо");
        QCOMPARE(chat.chats().size(), 2);
        chat.setProject(otherProject, "one@example.com");
        QVERIFY(chat.chats().isEmpty());
        QVERIFY(!chat.selectChat(first));
        chat.setProject(project, "two@example.com");
        QVERIFY(chat.chats().isEmpty());
        QVERIFY(!chat.selectChat(first));
        QVERIFY(chat.deleteChat());
    }
    Ui::AiChatSession restored(path);
    restored.setProject(project, "one@example.com");
    QCOMPARE(restored.currentChat(), second);
    QCOMPARE(restored.chats().size(), 2);
    QVERIFY(restored.selectChat(first));
    QCOMPARE(restored.messages().last().toObject().value("text").toString(), QString("Давайте"));
    QVERIFY(restored.deleteChat());
    QCOMPARE(restored.chats().size(), 1);
    QCOMPARE(restored.currentChat(), second);
}

void AiChatTest::replacesDocumentButKeepsConversation()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    const auto original = QUuid::createUuid();
    const auto draft = QUuid::createUuid();
    QString content = "ORIGINAL_ONLY_CONTENT";
    chat.setDocument(original, "Original", [&content] { return content; });
    QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
    QVERIFY(chat.send("What is the theme?"));
    QVERIFY(requests.last().first().toString().contains(content));
    chat.finishReply("A discussion of courage.");
    content = "UNSAVED_LATEST_EDIT";
    QVERIFY(chat.send("What changed?"));
    auto prompt = requests.last().first().toString();
    QVERIFY(prompt.contains(content));
    QVERIFY(!prompt.contains("ORIGINAL_ONLY_CONTENT"));
    chat.finishReply("An edit.");
    chat.setDocument(draft, "Draft 2", [] { return "DRAFT_ONLY_CONTENT"; });
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("context"));
    QCOMPARE(chat.messages().last().toObject().value("document").toString(), draft.toString());
    const auto count = chat.messages().size();
    chat.setDocument(draft, "Draft 2", [] { return "DRAFT_ONLY_CONTENT"; });
    QCOMPARE(chat.messages().size(), count);
    QVERIFY(chat.send("/summarize"));
    prompt = requests.last().first().toString();
    QVERIFY(prompt.contains("DRAFT_ONLY_CONTENT"));
    QVERIFY(prompt.contains("What is the theme?"));
    QVERIFY(prompt.contains("A discussion of courage."));
    QVERIFY(!prompt.contains("UNSAVED_LATEST_EDIT"));
    QVERIFY(!prompt.contains("ORIGINAL_ONLY_CONTENT"));
    chat.finishReply("Summary");
    chat.setDocument({}, {}, {});
    QVERIFY(chat.send("Continue our conversation"));
    prompt = requests.last().first().toString();
    QVERIFY(!prompt.contains("DRAFT_ONLY_CONTENT"));
    QVERIFY(prompt.contains("Summary"));
}

void AiChatTest::routesReplyToOriginalConversation()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    const auto project = QUuid::createUuid();
    chat.setProject(project, {});
    chat.setBackendAvailable(true);
    QVERIFY(chat.send("First request"));
    const auto first = chat.currentChat();
    QVERIFY(chat.createChat());
    QVERIFY(!chat.send("Must wait"));
    chat.setProject(QUuid::createUuid(), {});
    chat.finishReply("Reply for the first chat");
    QVERIFY(chat.messages().isEmpty());
    chat.setProject(project, {});
    QVERIFY(chat.selectChat(first));
    QCOMPARE(chat.messages().last().toObject().value("text").toString(), QString("Reply for the first chat"));
}

void AiChatTest::deletingPendingChatDoesNotResurrectIt()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    QVERIFY(chat.send("Delete me"));
    const auto id = chat.currentChat();
    QVERIFY(chat.deleteChat());
    chat.finishReply("Late reply");
    QVERIFY(chat.chats().isEmpty());
    QVERIFY(!chat.selectChat(id));
    QVERIFY(!chat.hasPendingReply());
    QVERIFY(chat.messages().isEmpty());
}

void AiChatTest::failuresAndAvailability()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setBackendAvailable(true);
    QVERIFY(!chat.send("No project"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(false);
    QVERIFY(!chat.send("Offline"));
    chat.setBackendAvailable(true);
    QVERIFY(!chat.send("   \n"));
    QVERIFY(chat.send("Question"));
    QVERIFY(!chat.send("Duplicate"));
    chat.failReply("Connection lost");
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("error"));
    QVERIFY(!chat.hasPendingReply());
    QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
    QVERIFY(chat.send("Try again"));
    QVERIFY(!requests.last().first().toString().contains("Connection lost"));
    chat.finishReply("Done");
    chat.finishReply("Unsolicited response");
    QCOMPARE(chat.messages().last().toObject().value("text").toString(), QString("Done"));
}

void AiChatTest::coexistsWithSessionStatistics()
{
    QTemporaryDir dir;
    const auto path = dir.filePath("sessions.db");
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", "statistics_test");
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec("CREATE TABLE sessions (uuid TEXT PRIMARY KEY, words INTEGER)"));
        QVERIFY(query.exec("INSERT INTO sessions VALUES ('existing-session', 42)"));
    }
    QSqlDatabase::removeDatabase("statistics_test");
    {
        Ui::AiChatSession chat(path);
        chat.setProject(QUuid::createUuid(), {});
        QVERIFY(chat.createChat());
    }
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", "statistics_test");
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec("SELECT words FROM sessions WHERE uuid = 'existing-session'"));
        QVERIFY(query.next());
        QCOMPARE(query.value(0).toInt(), 42);
    }
    QSqlDatabase::removeDatabase("statistics_test");
}

void AiChatTest::reportsStorageFailure()
{
    Ui::AiChatSession chat("/nonexistent-directory/ai-test/sessions.db");
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    QSignalSpy errors(&chat, &Ui::AiChatSession::errorOccurred);
    QVERIFY(!chat.createChat());
    QCOMPARE(errors.size(), 1);
    QVERIFY(!chat.canSend());
}

void AiChatTest::timeoutConsumesLateReplies()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    QVERIFY(chat.send("Slow request"));
    auto timer = chat.findChild<QTimer*>("aiChatReplyTimeout");
    QVERIFY(timer);
    QVERIFY(QMetaObject::invokeMethod(timer, "timeout", Qt::DirectConnection));
    QVERIFY(!chat.hasPendingReply());
    QVERIFY(!chat.canSend());
    QVERIFY(!chat.beginLegacyRequest());
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("error"));
    QVERIFY(chat.finishReply("Late response that must not enter the editor"));
    QVERIFY(chat.canSend());
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("error"));
    QVERIFY(chat.send("New request"));
    chat.setBackendAvailable(false);
    chat.failReply("Disconnected");
    QVERIFY(chat.finishReply("Unsolicited response after disconnection"));
    QVERIFY(!chat.canSend());
    chat.setBackendAvailable(true);
    QVERIFY(chat.canSend());
}

void AiChatTest::legacyRequestsCannotOverlapChatRequests()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    QVERIFY(chat.beginLegacyRequest());
    QVERIFY(!chat.beginLegacyRequest());
    QVERIFY(!chat.send("Chat must wait for the editor"));
    QVERIFY(!chat.finishReply("Legacy result belongs to the editor"));
    QVERIFY(chat.send("Chat request"));
    QVERIFY(!chat.beginLegacyRequest());
    QVERIFY(chat.finishReply("Chat result"));
    QVERIFY(chat.beginLegacyRequest());
    chat.setBackendAvailable(false);
    chat.setBackendAvailable(true);
    QVERIFY(chat.canSend());
    QVERIFY(chat.finishReply("Unsolicited result must be ignored"));
}

void AiChatTest::changingAccountKeepsOnlyCurrentDocument()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), "first@example.com");
    chat.setBackendAvailable(true);
    chat.setDocument(QUuid::createUuid(), "Current draft", [] { return "CURRENT_CONTENT"; });
    QVERIFY(chat.send("PRIVATE_ACCOUNT_MESSAGE"));
    chat.finishReply("PRIVATE_ACCOUNT_REPLY");
    chat.setAccount("second@example.com");
    QVERIFY(chat.chats().isEmpty());
    QCOMPARE(chat.documentName(), QString("Current draft"));
    QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
    QVERIFY(chat.send("New account question"));
    const auto prompt = requests.last().first().toString();
    QVERIFY(prompt.contains("CURRENT_CONTENT"));
    QVERIFY(!prompt.contains("PRIVATE_ACCOUNT_MESSAGE"));
    QVERIFY(!prompt.contains("PRIVATE_ACCOUNT_REPLY"));
}

void AiChatTest::renamesPersistAndSurviveFirstMessage()
{
    QTemporaryDir dir;
    const auto path = dir.filePath("sessions.db");
    const auto project = QUuid::createUuid();
    QString id;
    {
        // Emulate the schema shipped before rename support.
        auto db = QSqlDatabase::addDatabase("QSQLITE", "old-chat-schema");
        db.setDatabaseName(path);
        QVERIFY(db.open());
        QSqlQuery query(db);
        QVERIFY(query.exec("CREATE TABLE ai_chats (id TEXT PRIMARY KEY, project_uuid TEXT NOT NULL, "
                           "account_email TEXT NOT NULL, title TEXT NOT NULL, created_at TEXT NOT NULL, selected_at TEXT NOT NULL)"));
    }
    QSqlDatabase::removeDatabase("old-chat-schema");
    {
        Ui::AiChatSession chat(path);
        chat.setProject(project, {});
        chat.setBackendAvailable(true);
        QVERIFY(chat.createChat());
        id = chat.currentChat();
        QVERIFY(!chat.renameChat(" "));
        QVERIFY(!chat.renameChat(QString(161, 'x')));
        QVERIFY(chat.renameChat("  My   story  "));
        QVERIFY(chat.send("First message must not replace my name"));
        chat.finishReply("Reply");
        QCOMPARE(chat.chats().first().toObject().value("title").toString(), QString("My story"));
    }
    Ui::AiChatSession restored(path);
    restored.setProject(project, {});
    QCOMPARE(restored.currentChat(), id);
    QCOMPARE(restored.chats().first().toObject().value("title").toString(), QString("My story"));
    restored.setBackendAvailable(true);
    QVERIFY(restored.createChat());
    QVERIFY(restored.startTool("/expand An idea", "expand"));
    QCOMPARE(restored.chats().first().toObject().value("title").toString(), QString("/expand An idea"));
    restored.finishToolReply("expand", "An expanded idea");
}

void AiChatTest::toolRepliesKeepTheirOriginAndIgnoreStaleTargets()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    const auto project = QUuid::createUuid();
    const auto doc = QUuid::createUuid();
    QString content = "Original content";
    chat.setProject(project, {});
    chat.setBackendAvailable(true);
    chat.setDocument(doc, "Book / Draft", [&content] { return content; });
    QVERIFY(chat.startTool("/generate A scene", "generate"));
    const auto origin = chat.currentChat();
    QVERIFY(chat.toolTargetsCurrentDocument());
    QVERIFY(!chat.startTool("/expand", "expand"));
    QVERIFY(chat.beginLegacyRequest());
    QVERIFY(!chat.beginLegacyRequest());
    QVERIFY(!chat.finishToolReply("expand", "Wrong tool"));
    QVERIFY(chat.createChat());
    QVERIFY(chat.toolTargetsCurrentDocument());
    chat.setDocument(doc, "Book / Other draft", [&content] { return content; });
    QVERIFY(!chat.toolTargetsCurrentDocument());
    QVERIFY(chat.messages().last().toObject().value("text").toString().contains("Other draft"));
    QVERIFY(chat.finishToolReply("generate", "Result for the first chat"));
    QVERIFY(chat.selectChat(origin));
    const auto reply = chat.messages().at(chat.messages().size() - 2).toObject();
    QCOMPARE(reply.value("role").toString(), QString("assistant"));
    QVERIFY(reply.value("text").toString().startsWith("Result for the first chat"));
    QVERIFY(reply.value("text").toString().contains("not added"));
    QVERIFY(chat.startTool("/translate English", "translate-document"));
    QVERIFY(chat.toolTargetsCurrentDocument());
    content += " Edited while translating";
    QVERIFY(!chat.toolTargetsCurrentDocument());
    chat.finishToolReply("translate-document", "Translation");
    QVERIFY(chat.startTool("/generate Again", "generate"));
    QVERIFY(chat.deleteChat());
    QVERIFY(!chat.toolTargetsCurrentDocument());
    chat.finishToolReply("generate", "Late result");
    QVERIFY(!chat.selectChat(origin));
    QVERIFY(chat.canSend());
}

void AiChatTest::toolTimeoutAndMultipleResults()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setDocument(QUuid::createUuid(), "Character", [] { return "Content"; });
    chat.setBackendAvailable(true);
    QVERIFY(chat.startTool("/character --personal --image Hero", "generate", 2));
    QVERIFY(chat.finishToolReply("generate", "Character description"));
    QVERIFY(chat.hasPendingReply());
    QVERIFY(!chat.canSend());
    QVERIFY(chat.finishToolReply("generate", "Character image generated"));
    QVERIFY(chat.canSend());
    QVERIFY(chat.startTool("/generate Slow", "generate"));
    QVERIFY(QMetaObject::invokeMethod(chat.findChild<QTimer*>("aiChatReplyTimeout"), "timeout", Qt::DirectConnection));
    QVERIFY(!chat.toolTargetsCurrentDocument());
    QVERIFY(!chat.canSend());
    QVERIFY(chat.finishToolReply("generate", "Late reply"));
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("error"));
    QVERIFY(chat.canSend());
}

void AiChatTest::clearingChatKeepsNameAndDiscardsPendingReply()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    chat.setDocument(QUuid::createUuid(), "Document", [] { return "Text"; });
    QVERIFY(chat.createChat());
    const auto id = chat.currentChat();
    QVERIFY(chat.renameChat("My notes"));
    QVERIFY(chat.send("Old question"));
    QVERIFY(chat.clearChat());
    QCOMPARE(chat.currentChat(), id);
    QCOMPARE(chat.chats().first().toObject().value("title").toString(), QString("My notes"));
    QCOMPARE(chat.messages().size(), 1);
    QCOMPARE(chat.messages().first().toObject().value("role").toString(), QString("context"));
    QVERIFY(!chat.canSend());
    chat.finishReply("Late reply must not restore cleared messages");
    QCOMPARE(chat.messages().size(), 1);
    QVERIFY(chat.canSend());
    QVERIFY(chat.startTool("/generate More", "generate"));
    QVERIFY(chat.clearChat());
    QVERIFY(!chat.toolTargetsCurrentDocument());
    chat.finishToolReply("generate", "Late result");
    QCOMPARE(chat.messages().size(), 1);
}

QTEST_GUILESS_MAIN(AiChatTest)
#include "tst_ai_chat.moc"

#include <ui/modules/ai_assistant/ai_chat_session.h>

#include <QJsonObject>
#include <QJsonDocument>
#include <QRegularExpression>
#include <QSignalSpy>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QtTest>

class AiChatTest : public QObject
{
    Q_OBJECT
private slots:
    void concatenatedRetrievalCommandsStayOutOfChat();
    void malformedRetrievalRepairsOnce();
    void documentIdsRecoverOnlyKnownUuids();
    void projectCorpusFindsLateEpisodesAndScenes();
    void scopeAndSourcesSurviveRestart();
    void projectPreparationWaitsForCloudAndCancels();
    void retrievalKeepsOriginAndHasABudget();
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


void AiChatTest::projectCorpusFindsLateEpisodesAndScenes()
{
    Ui::AiProjectContext corpus;
    QVector<Ui::AiContextDocument> documents;
    const auto scene = QUuid::createUuid().toString();
    for (int i = 1; i <= 90; ++i) {
        const auto content = i == 90
            ? QString("<document><scene uuid=\"%1\"><heading><value>INT. ARCHIVE</value></heading>"
                      "<action><value>%2</value></action><action><value>Ксения спрятала завещание в сейфе.</value>"
                      "</action></scene><facts><occupation value=\"ASTRONOMER\"/><secret>HIDDEN_METADATA</secret></facts></document>")
                  .arg(scene, QString("Long scene text. ").repeated(3000))
            : QString("Ordinary episode %1. ").arg(i).repeated(1000);
        documents.append({ QUuid::createUuid().toString(), QString("Series / Episode %1 / Latest draft").arg(i), content });
    }
    corpus.resetDocuments(documents);
    for (const auto& document : documents) { corpus.updateDocument(document); }
    QCOMPARE(corpus.documentCount(), 90);
    QVERIFY(corpus.chunkCount() > 600); // Several megabytes, not just a catalog of empty episodes.
    QCOMPARE(corpus.catalog().value("documents").toArray().size(), 40);
    QCOMPARE(corpus.catalog(80).value("documents").toArray().size(), 10);
    auto results = corpus.search("завещание").value("results").toArray();
    QCOMPARE(results.size(), 1);
    const auto result = results.first().toObject();
    QCOMPARE(result.value("document").toString(), documents.last().id);
    QCOMPARE(result.value("scene").toString(), scene);
    QVERIFY(result.value("title").toString().contains("INT. ARCHIVE"));
    QVERIFY(result.value("text").toString().contains("сейфе"));
    QCOMPARE(corpus.source(result.value("source").toString()), result);
    QVERIFY(!corpus.search("ASTRONOMER").value("results").toArray().isEmpty());
    QVERIFY(!corpus.search("HIDDEN_METADATA").value("results").toArray().isEmpty());
    QCOMPARE(corpus.read(documents.last().id).value("results").toArray().size(), 6);
    QString reconstructed;
    for (int offset = 0; offset >= 0;) {
        const auto page = corpus.read(documents.last().id, offset);
        for (const auto& chunk : page.value("results").toArray()) { reconstructed += chunk.toObject().value("text").toString(); }
        offset = page.value("next_offset").toInt();
    }
    QVERIFY(reconstructed.contains("завещание"));
    documents.last().content = "Updated latest draft, no old secrets.";
    corpus.updateDocument(documents.last());
    QVERIFY(corpus.search("завещание").value("results").toArray().isEmpty());
    QVERIFY(corpus.source(result.value("source").toString()).isEmpty());
    documents.removeLast();
    corpus.resetDocuments(documents);
    QCOMPARE(corpus.documentCount(), 89);
    QVERIFY(corpus.search("Updated").value("results").toArray().isEmpty());
}

void AiChatTest::scopeAndSourcesSurviveRestart()
{
    QTemporaryDir dir;
    const auto project = QUuid::createUuid();
    const auto document = QUuid::createUuid();
    const auto scene = QUuid::createUuid();
    const Ui::AiContextDocument latest{ document.toString(), "Series / Episode 90 / Latest draft",
        QString("<text><scene uuid=\"%1\"><value>Final scene</value><value>LATEST_SECRET</value></scene></text>").arg(scene.toString()) };
    Ui::AiProjectContext corpus;
    corpus.resetDocuments({ latest }); corpus.updateDocument(latest);
    const auto source = corpus.search("LATEST_SECRET").value("results").toArray().first().toObject().value("source").toString();
    const QUrl url("starc-source:" + source);
    QString origin;
    {
        Ui::AiChatSession chat(dir.filePath("sessions.db"));
        chat.setProject(project, "author");
        chat.setBackendAvailable(true);
        chat.setDocument(QUuid::createUuid(), "Archived draft", [] { return "ARCHIVE_SECRET"; });
        chat.setProjectDocumentsProvider([latest](bool) { return QVector<Ui::AiContextDocument>{ latest }; });
        chat.setContextScope(Ui::AiChatSession::ContextScope::Project);
        QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
        QVERIFY(chat.send("Find LATEST_SECRET"));
        origin = chat.currentChat();
        QTRY_COMPARE(requests.size(), 1);
        const auto prompt = requests.last().first().toString();
        QVERIFY(prompt.contains("LATEST_SECRET"));
        QVERIFY(!prompt.contains("ARCHIVE_SECRET"));
        QVERIFY(prompt.size() < 65000);
        const auto count = chat.messages().size();
        chat.setDocument(QUuid::createUuid(), "Another document", [] { return "UNRELATED"; });
        QCOMPARE(chat.messages().size(), count); // Project context did not change.
        QVERIFY(chat.finishReply("The secret is here [[source:" + source + "]]."));
        QVERIFY(chat.messages().last().toObject().value("text").toString().contains(url.toString()));
        QSignalSpy navigation(&chat, &Ui::AiChatSession::sourceRequested);
        QVERIFY(chat.openSource(url));
        QCOMPARE(navigation.last().at(0).toUuid(), document);
        QCOMPARE(navigation.last().at(1).toUuid(), scene);
        QVERIFY(!chat.openSource(QUrl("https://example.com")));
        QVERIFY(!chat.openSource(QUrl("starc-source:000000000000000000000000")));
        QVERIFY(chat.createChat());
        QVERIFY(!chat.openSource(url));
        chat.setContextScope(Ui::AiChatSession::ContextScope::Document);
        QVERIFY(chat.selectChat(origin));
        QCOMPARE(chat.contextScope(), Ui::AiChatSession::ContextScope::Project);
    }
    Ui::AiChatSession restored(dir.filePath("sessions.db"));
    restored.setProject(project, "author");
    QCOMPARE(restored.contextScope(), Ui::AiChatSession::ContextScope::Project);
    QVERIFY(restored.openSource(url));
    restored.setAccount("other");
    QVERIFY(!restored.openSource(url));
    restored.setAccount("author");
    QVERIFY(restored.openSource(url));
    QVERIFY(restored.clearChat());
    QVERIFY(!restored.openSource(url));
}

void AiChatTest::projectPreparationWaitsForCloudAndCancels()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    const auto project = QUuid::createUuid();
    chat.setProject(project, {});
    chat.setBackendAvailable(true);
    QVector<Ui::AiContextDocument> documents;
    for (int i = 0; i < 7; ++i) {
        documents.append({ QUuid::createUuid().toString(), "Episode", "Cloud text", false });
    }
    int snapshots = 0;
    chat.setProjectDocumentsProvider([&documents, &snapshots](bool includeContent) {
        if (includeContent) { ++snapshots; }
        return documents;
    });
    chat.setContextScope(Ui::AiChatSession::ContextScope::Project);
    QSignalSpy downloads(&chat, &Ui::AiChatSession::projectDocumentRequested);
    QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
    QVERIFY(chat.send("Question"));
    QTRY_COMPARE(downloads.size(), 4);
    QCOMPARE(requests.size(), 0);
    QTest::qWait(450);
    QCOMPARE(downloads.size(), 4); // Don't request the same missing documents repeatedly.
    QCOMPARE(snapshots, 0);
    for (auto& document : documents) { document.available = true; }
    QTRY_COMPARE(requests.size(), 1);
    QCOMPARE(snapshots, 1);
    chat.failReply("Test complete");
    for (auto& document : documents) { document.available = false; }
    QVERIFY(chat.send("Cancelled"));
    QTRY_COMPARE(downloads.size(), 8);
    QVERIFY(chat.clearChat());
    for (auto& document : documents) { document.available = true; }
    QTest::qWait(500);
    QCOMPARE(requests.size(), 1);
    QVERIFY(chat.canSend());
    for (auto& document : documents) { document.available = false; }
    QVERIFY(chat.send("Cloud timeout"));
    QTRY_COMPARE(downloads.size(), 12);
    QVERIFY(QMetaObject::invokeMethod(chat.findChild<QTimer*>("aiChatReplyTimeout"), "timeout", Qt::DirectConnection));
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("error"));
    QVERIFY(chat.canSend());
    QVERIFY(chat.send("Switch project before indexing"));
    chat.setProject(QUuid::createUuid(), {});
    QTest::qWait(50);
    QCOMPARE(requests.size(), 1);
    QVERIFY(chat.canSend());
}

void AiChatTest::retrievalKeepsOriginAndHasABudget()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    QVector<Ui::AiContextDocument> documents;
    for (int i = 1; i <= 90; ++i) {
        documents.append({ QUuid::createUuid().toString(), QString("Episode %1").arg(i),
                           i == 90 ? "RARE_DISCOVERY" : "Ordinary dialogue" });
    }
    chat.setProjectDocumentsProvider([documents](bool) { return documents; });
    chat.setContextScope(Ui::AiChatSession::ContextScope::Project);
    QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
    QVERIFY(chat.send("Research the ending"));
    const auto origin = chat.currentChat();
    QTRY_COMPARE(requests.size(), 1);
    QVERIFY(chat.createChat());
    chat.finishReply(R"({"tool":"search","query":"RARE_DISCOVERY","notes":"Search episode endings"})");
    QCOMPARE(requests.size(), 2);
    QVERIFY(requests.last().first().toString().contains("RARE_DISCOVERY"));
    QVERIFY(!chat.messages().last().toObject().value("text").toString().contains("RARE_DISCOVERY"));
    Ui::AiProjectContext corpus;
    corpus.resetDocuments(documents);
    for (const auto& document : documents) { corpus.updateDocument(document); }
    const auto source = corpus.search("RARE_DISCOVERY").value("results").toArray().first().toObject().value("source").toString();
    chat.finishReply("An unsupported answer without citations.");
    QCOMPARE(requests.size(), 3); // One citation repair request.
    chat.finishReply("Episode 90 [[source:" + source + "]]");
    QVERIFY(chat.selectChat(origin));
    QVERIFY(chat.messages().last().toObject().value("text").toString().contains("starc-source:" + source));
    QVERIFY(chat.send("Keep searching"));
    QTRY_COMPARE(requests.size(), 4);
    for (int i = 0; i < 8; ++i) { chat.finishReply("```json\n{\"tool\":\"catalog\",\"offset\":40}\n```"); }
    QCOMPARE(requests.size(), 12);
    chat.finishReply("Still no citations");
    QCOMPARE(requests.size(), 13);
    chat.finishReply("```json\n{\"tool\":\"catalog\",\"offset\":80}\n```");
    QVERIFY(chat.canSend());
    QVERIFY(chat.messages().last().toObject().value("text").toString().contains("step limit"));
}


void AiChatTest::concatenatedRetrievalCommandsStayOutOfChat()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setBackendAvailable(true);
    const auto document = QUuid::createUuid();
    chat.setDocument(document, "Episode", [] { return QString("Scene text with important events. ").repeated(600); });
    QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
    QVERIFY(chat.send("Собери синопсис серии."));
    const auto messageCount = chat.messages().size();
    auto malformedId = document.toString();
    malformedId.chop(1); // As in the reported response: the UUID loses its closing brace.
    const auto command = [&](int offset, const QString& notes) {
        return QString::fromUtf8(QJsonDocument(QJsonObject{
            { "tool", "read" }, { "document", malformedId }, { "offset", offset }, { "notes", notes }
        }).toJson(QJsonDocument::Compact));
    };
    const auto response = command(0, "A quoted \"word\" and {braces} in notes") + " "
        + command(5, "Continue reading") + " transport separator\n"
        + command(5, "Repeated command with different notes");
    QVERIFY(chat.finishReply(response));
    QCOMPARE(requests.size(), 2); // One transport request for the bounded batch.
    QCOMPARE(chat.messages().size(), messageCount);
    const auto prompt = requests.last().first().toString();
    QVERIFY(prompt.contains("\"offset\":0,\"tool\":\"read\""));
    QVERIFY(prompt.contains("\"offset\":5,\"tool\":\"read\""));
    QVERIFY(!prompt.contains("Unknown document"));
    QVERIFY(!prompt.contains("transport separator"));
    QVERIFY(!prompt.contains("Repeated command"));
    QVERIFY(prompt.size() < 65000);
    QVERIFY(chat.preparationStatus().contains("2 of 8"));
    chat.finishReply("Синопсис серии: герои находят важные сведения.");
    QCOMPARE(chat.messages().size(), messageCount + 1);
    QVERIFY(!chat.messages().last().toObject().value("text").toString().contains("tool"));
    QVERIFY(chat.canSend());

    // Concatenated/fenced commands must also remain hidden after the step budget.
    QVERIFY(chat.send("Read more"));
    for (int i = 0; i < 4; ++i) {
        chat.finishReply("```json\n[" + command(0, "") + ',' + command(5, "") + "]\n```");
    }
    QCOMPARE(requests.size(), 7);
    chat.finishReply(response);
    QCOMPARE(requests.size(), 7);
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("error"));
    QVERIFY(chat.messages().last().toObject().value("text").toString().contains("step limit"));
    QVERIFY(chat.canSend());
}

void AiChatTest::malformedRetrievalRepairsOnce()
{
    QTemporaryDir dir;
    Ui::AiChatSession chat(dir.filePath("sessions.db"));
    chat.setProject(QUuid::createUuid(), {});
    chat.setDocument(QUuid::createUuid(), "Document", [] { return "A story"; });
    chat.setBackendAvailable(true);
    QSignalSpy requests(&chat, &Ui::AiChatSession::replyRequested);
    QVERIFY(chat.send("Summarize"));
    const auto count = chat.messages().size();
    const QString malformed = "```json\n{\"tool\":\"read\",\"document\":\"unfinished";
    chat.finishReply(malformed);
    QCOMPARE(requests.size(), 2);
    QCOMPARE(chat.messages().size(), count);
    QVERIFY(requests.last().first().toString().contains("could not be parsed"));
    chat.finishReply(malformed);
    QCOMPARE(requests.size(), 2);
    QCOMPARE(chat.messages().last().toObject().value("role").toString(), QString("error"));
    QVERIFY(!chat.messages().last().toObject().value("text").toString().contains("tool"));
    QVERIFY(chat.canSend());
    QVERIFY(chat.send("Try again"));
    chat.finishReply(malformed);
    QCOMPARE(requests.size(), 4); // Repair budget resets for the new question.
    chat.finishReply("A normal answer with JSON data: {\"theme\":\"hope\"}");
    QCOMPARE(requests.size(), 4);
    QVERIFY(chat.messages().last().toObject().value("text").toString().contains("hope"));
}

void AiChatTest::documentIdsRecoverOnlyKnownUuids()
{
    Ui::AiProjectContext corpus;
    const auto uuid = QUuid::createUuid();
    const Ui::AiContextDocument document{ uuid.toString(), "Episode", "Searchable evidence" };
    corpus.resetDocuments({ document }); corpus.updateDocument(document);
    const auto expected = corpus.read(uuid.toString());
    auto missingBrace = uuid.toString(); missingBrace.chop(1);
    QCOMPARE(corpus.read(missingBrace), expected);
    QCOMPARE(corpus.read(uuid.toString(QUuid::WithoutBraces).toUpper()), expected);
    QCOMPARE(corpus.search("evidence", 0, missingBrace), corpus.search("evidence", 0, uuid.toString()));
    QVERIFY(corpus.read(QUuid::createUuid().toString()).contains("error"));
    QVERIFY(corpus.search("evidence", 0, "invalid-id").contains("error"));
}

QTEST_GUILESS_MAIN(AiChatTest)
#include "tst_ai_chat.moc"

#include <ui/modules/ai_assistant/ai_assistant_view.h>
#include <ui/modules/ai_assistant/ai_chat_session.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/text_field/text_field.h>
#include <management_layer/content/writing_session/writing_session_storage.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/text_template.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>

#include <QJsonObject>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QtTest>
#include <memory>

class AiChatViewTest : public QObject
{
    Q_OBJECT
    QTemporaryDir data;

private slots:
    void initTestCase()
    {
        qputenv("XDG_DATA_HOME", data.path().toUtf8());
        QCoreApplication::setApplicationName("ai-chat-tests");
    }

    void chatsUseKeyboardAndRenderEscapedReplies()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setDocument(QUuid::createUuid(), "Current draft", [] { return "Draft text"; });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setAvailableWords(100);
        view.resize(440, 800);
        view.show();
        QTest::qWait(30);
        auto input = view.findChild<TextField*>("aiChatInput");
        auto send = view.findChild<Button*>("aiChatSend");
        auto history = view.findChild<QTextBrowser*>("aiChatHistory");
        QVERIFY(input && send && history);
        QVERIFY(input->isVisible());
        QSignalSpy requests(session, &Ui::AiChatSession::replyRequested);
        input->setText("First line");
        input->setFocus();
        auto cursor = input->textCursor();
        cursor.movePosition(QTextCursor::End);
        input->setTextCursor(cursor);
        QTest::keyClick(input, Qt::Key_Return, Qt::ShiftModifier);
        QVERIFY(input->text().contains('\n'));
        QCOMPARE(requests.size(), 0);
        QTest::keyClicks(input, "Second line");
        QTest::keyClick(input, Qt::Key_Return);
        QCOMPARE(requests.size(), 1);
        QVERIFY(input->text().isEmpty());
        QVERIFY(!send->isEnabled());
        session->finishReply("<b>Literal text</b>\nSecond paragraph");
        QVERIFY(history->toPlainText().contains("<b>Literal text</b>"));
        QVERIFY(history->toPlainText().contains("Second paragraph"));
        auto insert = view.findChild<Button*>("aiChatInsert");
        QVERIFY(insert->isVisible());
        QSignalSpy insertions(&view, &Ui::AiAssistantView::insertTextRequested);
        insert->click();
        QCOMPARE(insertions.size(), 1);
        QCOMPARE(insertions.first().first().toString(), QString("<b>Literal text</b>\nSecond paragraph"));
        view.setReadOnly(true);
        QVERIFY(!insert->isVisible());
        input->setText("Discuss a read-only document");
        QVERIFY(send->isEnabled());
        view.setInsertionAvailable(false);
        view.setReadOnly(false);
        QVERIFY(!insert->isVisible());
        const auto screenshot = qEnvironmentVariable("AI_CHAT_SCREENSHOT");
        if (!screenshot.isEmpty()) {
            QVERIFY(view.grab().save(screenshot));
        }
    }

    void createsAndDeletesChatsFromTheView()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        Ui::AiAssistantView view;
        view.resize(440, 800);
        view.show();
        QTest::qWait(30);
        auto create = view.findChild<IconButton*>("aiNewChat");
        auto remove = view.findChild<IconButton*>("aiDeleteChat");
        QVERIFY(create && remove);
        create->click();
        QCOMPARE(session->chats().size(), 1);
        create->click();
        QCOMPARE(session->chats().size(), 2);
        remove->click();
        QCOMPARE(session->chats().size(), 1);
        remove->click();
        QVERIFY(session->chats().isEmpty());
        QVERIFY(!remove->isEnabled());
    }

    void statisticsInitializeWhenChatTablesAlreadyExist()
    {
        auto chat = Ui::AiChatSession::instance();
        chat->setProject(QUuid::createUuid(), {});
        QVERIFY(chat->createChat());
        DataStorageLayer::WritingSessionStorage storage;
        const auto db = QSqlDatabase::database("session_statistics_database");
        QVERIFY(db.tables().contains("sessions"));
        QVERIFY(db.tables().contains("system_variables"));
        QVERIFY(db.tables().contains("ai_chats"));
        QCOMPARE(chat->chats().size(), 1);
    }

    void readsUnsavedModelWithoutChangingDocument()
    {
        std::unique_ptr<Domain::DocumentObject> document(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::SimpleText, {}, {}));
        BusinessLayer::SimpleTextModel model;
        model.setDocument(document.get());
        const auto stored = document->content();
        auto paragraph = model.createTextItem();
        paragraph->setParagraphType(BusinessLayer::TextParagraphType::Text);
        paragraph->setText("Unsaved document content for the assistant");
        model.appendItem(paragraph);
        const auto snapshot = model.contentSnapshot();
        QVERIFY(snapshot.contains("Unsaved document content for the assistant"));
        QCOMPARE(document->content(), stored);
    }
};

QTEST_MAIN(AiChatViewTest)
#include "tst_ai_chat_view.moc"

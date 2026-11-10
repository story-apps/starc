#include <ui/modules/ai_assistant/ai_assistant_view.h>
#include <ui/modules/ai_assistant/ai_chat_session.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/combo_box/combo_box.h>
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
#include <QTextBlock>
#include <QInputDialog>
#include <QClipboard>
#include <QFontDatabase>
#include <ui/widgets/chat/chat_message.h>
#include <ui/widgets/chat/chat_messages_view.h>
#include <QScrollArea>
#include <QScrollBar>
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
        for (const auto& font : { ":/fonts/roboto-regular", ":/fonts/roboto-medium", ":/fonts/roboto-bold",
                                  ":/fonts/materialdesignicons" }) {
            QFontDatabase::addApplicationFont(font);
        }
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
        auto history = view.findChild<ChatMessagesView*>("aiChatHistory");
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
        QCOMPARE(history->messages().last().text(), QString("<b>Literal text</b>\nSecond paragraph"));
        QVERIFY(!history->messages().last().isSistemMessage());
        QVERIFY(history->messages().first().isSistemMessage());
        QVERIFY(history->messages().last().dateTime().isValid());
        QVERIFY(history->messages().at(1).author() != history->messages().last().author());
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

    void sharedHistoryScrollsLongReplies()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setDocument(QUuid::createUuid(), "Draft", [] { return "Document"; });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setAvailableWords(100);
        view.resize(360, 650);
        view.show();
        QVERIFY(session->send("Explain the document"));
        session->finishReply(QString("A long response that wraps across several lines.\n").repeated(120));
        auto container = view.findChild<QScrollArea*>("aiChatHistoryContainer");
        auto history = view.findChild<ChatMessagesView*>("aiChatHistory");
        QVERIFY(container && history);
        auto scroll = container->verticalScrollBar();
        QTRY_VERIFY(scroll->maximum() > 0);
        QTRY_COMPARE(scroll->value(), scroll->maximum());
        QCOMPARE(history->messages().size(), 3);
        scroll->setValue(0);
        session->setDocument(QUuid::createUuid(), "Another draft", [] { return "New document"; });
        QTest::qWait(30);
        QCOMPARE(scroll->value(), 0);
        QVERIFY(history->messages().last().isSistemMessage());
        QVERIFY(history->messages().last().text().contains("Another draft"));
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
        QCOMPARE(session->chats().size(), 2);
        auto dialog = view.findChild<Dialog*>("aiDeleteChatConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 0, {}, Dialog::RejectButton });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(session->chats().size(), 2);
        remove->click();
        dialog = view.findChild<Dialog*>("aiDeleteChatConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 1, {}, Dialog::AcceptCriticalButton });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(session->chats().size(), 1);
        remove->click();
        dialog = view.findChild<Dialog*>("aiDeleteChatConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 1, {}, Dialog::AcceptCriticalButton });
        QVERIFY(session->chats().isEmpty());
        QVERIFY(!remove->isEnabled());
    }

    void commandsPreserveDraftAndHelpWorksOffline()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setBackendAvailable(false);
        Ui::AiAssistantView view;
        auto input = view.findChild<TextField*>("aiChatInput");
        auto commands = view.findChild<ComboBox*>("aiChatCommands");
        auto send = view.findChild<Button*>("aiChatSend");
        input->setText("Keep my text\nincluding line breaks");
        commands->currentIndexChanged(commands->model()->index(0, 0));
        QCOMPARE(input->text(), QString("/rephrase Keep my text\nincluding line breaks"));
        commands->currentIndexChanged(commands->model()->index(1, 0));
        QCOMPARE(input->text(), QString("/expand Keep my text\nincluding line breaks"));
        QVERIFY(!send->isEnabled());
        input->setText("/help");
        QVERIFY(send->isEnabled());
    }

    void switchingChatsRestoresReadingPosition()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.resize(360, 650);
        view.show();
        auto scroll = view.findChild<QScrollArea*>("aiChatHistoryContainer")->verticalScrollBar();
        QVERIFY(session->send("First chat"));
        session->finishReply(QString("Long reply\n").repeated(100));
        const auto first = session->currentChat();
        QTRY_VERIFY(scroll->maximum() > 100);
        QTRY_COMPARE(scroll->value(), scroll->maximum());
        scroll->setValue(70);
        QVERIFY(session->createChat());
        QVERIFY(session->send("Second chat"));
        session->finishReply(QString("Another long reply\n").repeated(100));
        QTRY_COMPARE(scroll->value(), scroll->maximum());
        QVERIFY(session->selectChat(first));
        QTRY_COMPARE(scroll->value(), 70);
    }

    void markdownAndTextSelectionUseSharedChatWidget()
    {
        ChatMessagesView history;
        history.resize(420, 800);
        history.setCurrentUser(User("You", "user"));
        ChatMessage reply(QDateTime::currentDateTime(),
                          "## Heading\n\nA **bold** word and `code`.\n\n- First\n- Second",
                          User("Assistant", "assistant"), false);
        reply.setMarkdown(true);
        history.setMessages({ ChatMessage(QDateTime::currentDateTime(), "**literal user input**",
                                          User("You", "user"), false), reply });
        history.show();
        auto browser = history.findChild<QTextBrowser*>("chatMessageText");
        QVERIFY(browser);
        QVERIFY(browser->toPlainText().contains("**literal user input**"));
        QVERIFY(!browser->toPlainText().contains("**bold**"));
        auto bold = browser->document()->find("bold");
        QVERIFY(!bold.isNull());
        QVERIFY(bold.charFormat().fontWeight() >= QFont::Bold);
        browser->setTextCursor(bold);
        QCOMPARE(history.selectedText(), QString("bold"));
        history.setMessages(history.messages());
        QCOMPARE(history.selectedText(), QString("bold"));
        browser->setFocus();
        QTest::keyClick(browser, Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->text(), QString("bold"));
        bool listFound = false;
        for (auto block = browser->document()->begin(); block.isValid(); block = block.next()) {
            listFound |= block.textList() != nullptr;
        }
        QVERIFY(listFound);
    }

    void commandsCallExistingToolsAndValidateDocumentAccess()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setDocument(QUuid::createUuid(), "Book / Chapter", [] {
            return "<document><value>Current draft text</value></document>";
        });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setAvailableWords(100);
        auto input = view.findChild<TextField*>("aiChatInput");
        auto send = view.findChild<Button*>("aiChatSend");
        QSignalSpy freeChat(session, &Ui::AiChatSession::replyRequested);
        QSignalSpy rephrase(&view, &Ui::AiAssistantView::rephraseRequested);
        QSignalSpy generate(&view, &Ui::AiAssistantView::generateTextRequested);
        QSignalSpy translate(&view, &Ui::AiAssistantView::translateRequested);
        input->setText("/rephrase friendly | A sentence");
        send->click();
        QCOMPARE(rephrase.size(), 1);
        QCOMPARE(rephrase.first().at(0).toString(), QString("A sentence"));
        QCOMPARE(rephrase.first().at(1).toString(), QString("friendly"));
        QVERIFY(input->text().isEmpty());
        QVERIFY(session->finishToolReply("rephrase", "A friendly sentence"));
        input->setText("/rephrase formal");
        send->click();
        QCOMPARE(rephrase.last().at(0).toString(), QString("Current draft text"));
        session->finishToolReply("rephrase", "Formal text");
        input->setText("/generate --end Write a scene");
        send->click();
        QCOMPARE(generate.size(), 1);
        QCOMPARE(generate.first().first().toString(), QString("Write a scene"));
        QCOMPARE(view.textInsertPosition(), Ui::AiAssistantView::TextInsertPosition::AtEnd);
        session->finishToolReply("generate", "Scene");
        view.setReadOnly(true);
        input->setText("/generate Another scene");
        send->click();
        QCOMPARE(generate.size(), 1);
        QVERIFY(!input->text().isEmpty());
        view.setReadOnly(false);
        input->setText("/translate French | Hello");
        send->click();
        QCOMPARE(translate.size(), 1);
        QCOMPARE(translate.first().at(0).toString(), QString("Hello"));
        QCOMPARE(translate.first().at(1).toString(), QString("French"));
        session->finishToolReply("translate", "Bonjour");
        QCOMPARE(freeChat.size(), 0);
        QVERIFY(!view.findChild<QWidget*>("aiChatTools"));
        input->setText("/unknown");
        send->click();
        QVERIFY(!session->hasPendingReply());
        QCOMPARE(input->text(), QString("/unknown"));
    }

    void wholeDocumentTranslationRequiresConfirmation()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setDocument(QUuid::createUuid(), "Chapter", [] { return "Original text"; });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setAvailableWords(100);
        view.setDocumentTranslationAvailable(true);
        auto input = view.findChild<TextField*>("aiChatInput");
        auto send = view.findChild<Button*>("aiChatSend");
        QSignalSpy translate(&view, &Ui::AiAssistantView::translateDocumentRequested);
        input->setText("/translate French");
        send->click();
        QCOMPARE(translate.size(), 0);
        auto dialog = view.findChild<Dialog*>("aiTranslateConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 0, {}, Dialog::RejectButton });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(input->text(), QString("/translate French"));
        send->click();
        dialog = view.findChild<Dialog*>("aiTranslateConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 1, {}, Dialog::AcceptCriticalButton });
        QCOMPARE(translate.size(), 1);
        QCOMPARE(translate.first().first().toString(), QString("French"));
        QVERIFY(input->text().isEmpty());
        session->finishToolReply("translate-document", "Texte original");
    }

    void renameIsAvailableFromChatHeader()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        QVERIFY(session->createChat());
        Ui::AiAssistantView view;
        auto rename = view.findChild<IconButton*>("aiRenameChat");
        QVERIFY(rename && rename->isEnabled());
        QTimer::singleShot(0, &view, [&view] {
            auto dialog = view.findChild<QInputDialog*>();
            if (dialog) {
                dialog->setTextValue("My renamed conversation");
                dialog->accept();
            }
        });
        rename->click();
        QCOMPARE(session->chats().first().toObject().value("title").toString(), QString("My renamed conversation"));
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

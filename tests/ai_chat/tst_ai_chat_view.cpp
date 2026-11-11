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
#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/text_template.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>

#include <QJsonObject>
#include <QAction>
#include <QPushButton>
#include <QLabel>
#include <QSignalSpy>
#include <QSqlDatabase>
#include <QTemporaryDir>
#include <QTextBrowser>
#include <QTextBlock>
#include <QAbstractTextDocumentLayout>
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

    void scopeSelectorAndSourceLinks()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        const auto document = QUuid::createUuid();
        session->setDocument(document, "Episode 90", [] { return "Final discovery"; });
        session->setProjectDocumentsProvider([document](bool) {
            return QVector<Ui::AiContextDocument>{ { document.toString(), "Episode 90", "Final discovery" } };
        });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setAvailableWords(1000);
        view.resize(440, 800);
        view.show();
        auto scope = view.findChild<ComboBox*>("aiContextScope");
        QVERIFY(scope);
        QCOMPARE(scope->model()->rowCount(), 2);
        scope->currentIndexChanged(scope->model()->index(1, 0));
        QCOMPARE(session->contextScope(), Ui::AiChatSession::ContextScope::Project);
        QSignalSpy requests(session, &Ui::AiChatSession::replyRequested);
        QVERIFY(session->send("Find the discovery"));
        QTRY_COMPARE(requests.size(), 1);
        Ui::AiProjectContext corpus;
        const Ui::AiContextDocument doc{ document.toString(), "Episode 90", "Final discovery" };
        corpus.resetDocuments({ doc }); corpus.updateDocument(doc);
        const auto source = corpus.search("discovery").value("results").toArray().first().toObject().value("source").toString();
        session->finishReply("Found it [[source:" + source + "]].");
        auto browser = view.findChild<QTextBrowser*>("chatMessageText");
        QVERIFY(browser);
        QTest::qWait(30);
        const auto link = browser->document()->find("Episode 90", browser->document()->find("Found it"));
        QVERIFY(!link.isNull());
        auto cursor = link;
        cursor.setPosition(link.selectionStart() + 2);
        const auto point = browser->cursorRect(cursor).center();
        QTest::mouseMove(browser->viewport(), point);
        QCOMPARE(browser->viewport()->cursor().shape(), Qt::PointingHandCursor);
        QSignalSpy navigation(session, &Ui::AiChatSession::sourceRequested);
        QTest::mouseClick(browser->viewport(), Qt::LeftButton, Qt::NoModifier, point);
        QCOMPARE(navigation.size(), 1);
        QCOMPARE(navigation.first().first().toUuid(), document);
        const auto screenshot = qEnvironmentVariable("AI_PROJECT_SCREENSHOT");
        if (!screenshot.isEmpty()) { QVERIFY(view.grab().save(screenshot)); }
        scope->currentIndexChanged(scope->model()->index(0, 0));
        QCOMPARE(session->contextScope(), Ui::AiChatSession::ContextScope::Document);
    }

    void sourceNavigationFindsGroupsInsideFolders()
    {
        std::unique_ptr<Domain::DocumentObject> document(Domain::ObjectsBuilder::createDocument(
            {}, QUuid::createUuid(), Domain::DocumentObjectType::SimpleText, {}, {}));
        BusinessLayer::SimpleTextModel model;
        model.setDocument(document.get());
        auto folder = model.createFolderItem(BusinessLayer::TextFolderType::Part);
        model.appendItem(folder);
        auto chapter = model.createGroupItem(BusinessLayer::TextGroupType::Chapter1);
        model.appendItem(chapter, folder);
        auto paragraph = model.createTextItem();
        paragraph->setParagraphType(BusinessLayer::TextParagraphType::Text);
        paragraph->setText("Chapter evidence");
        model.appendItem(paragraph, chapter);
        const auto index = model.indexForUuid(chapter->uuid());
        QVERIFY(index.isValid());
        QCOMPARE(model.itemForIndex(index), chapter);
        Ui::AiProjectContext corpus;
        const Ui::AiContextDocument doc{ document->uuid().toString(), "Novel", QString::fromUtf8(model.contentSnapshot()) };
        corpus.resetDocuments({ doc }); corpus.updateDocument(doc);
        const auto source = corpus.search("evidence").value("results").toArray().first().toObject();
        QCOMPARE(QUuid(source.value("scene").toString()), chapter->uuid());
    }

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
        auto send = view.findChild<IconButton*>("aiChatSend");
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
        auto insert = view.findChild<QAction*>("aiChatInsert");
        QVERIFY(insert->isVisible());
        QSignalSpy insertions(&view, &Ui::AiAssistantView::insertTextRequested);
        insert->trigger();
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
        auto create = view.findChild<QAction*>("aiNewChat");
        auto remove = view.findChild<QAction*>("aiDeleteChat");
        QVERIFY(create && remove);
        create->trigger();
        QCOMPARE(session->chats().size(), 1);
        create->trigger();
        QCOMPARE(session->chats().size(), 2);
        remove->trigger();
        QCOMPARE(session->chats().size(), 2);
        auto dialog = view.findChild<Dialog*>("aiDeleteChatConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 0, {}, Dialog::RejectButton });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(session->chats().size(), 2);
        remove->trigger();
        dialog = view.findChild<Dialog*>("aiDeleteChatConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 1, {}, Dialog::AcceptCriticalButton });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(session->chats().size(), 1);
        remove->trigger();
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
        auto commands = view.findChild<IconButton*>("aiChatCommandsToggle");
        auto send = view.findChild<IconButton*>("aiChatSend");
        input->setText("Keep my text\nincluding line breaks");
        commands->click();
        view.findChild<QPushButton*>("aiCommand_rephrase")->click();
        QVERIFY(input->text().isEmpty());
        view.findChild<QPushButton*>("aiToolCancel")->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(input->text(), QString("Keep my text\nincluding line breaks"));
        view.findChild<QPushButton*>("aiCommand_expand")->click();
        QCOMPARE(view.findChild<QLabel*>("aiToolArguments")->text(), QString("Keep my text\nincluding line breaks"));
        view.findChild<QPushButton*>("aiToolCancel")->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(input->text(), QString("Keep my text\nincluding line breaks"));
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
        history.setConversationStyle(true);
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

    void contextNoticesSelectWithoutEmptyParagraphsAndUseTextHitTesting()
    {
        ChatMessagesView history;
        history.resize(420, 400);
        const User context("Context", "context");
        const auto now = QDateTime::currentDateTime();
        history.setMessages({ ChatMessage(now, "First context notice", context, true),
                              ChatMessage(now, "Second context notice", context, true) });
        history.show();
        QTest::qWait(20);
        auto browser = history.findChild<QTextBrowser*>("chatMessageText");
        QVERIFY(browser);
        auto first = browser->document()->find("First context notice");
        auto second = browser->document()->find("Second context notice");
        QVERIFY(!first.isNull() && !second.isNull());
        auto selection = first;
        selection.setPosition(first.selectionStart());
        selection.setPosition(second.selectionEnd(), QTextCursor::KeepAnchor);
        browser->setTextCursor(selection);
        QCOMPARE(history.selectedText(), QString("First context notice\nSecond context notice"));
        browser->setFocus();
        QTest::keyClick(browser, Qt::Key_C, Qt::ControlModifier);
        QCOMPARE(QApplication::clipboard()->text(), history.selectedText());
        QCOMPARE(browser->cursorWidth(), 0);

        auto start = first;
        start.setPosition(first.selectionStart());
        const auto textPoint = browser->cursorRect(start).center() + QPoint(3, 0);
        QTest::mouseMove(browser->viewport(), textPoint);
        QCOMPARE(browser->viewport()->cursor().shape(), Qt::IBeamCursor);
        auto layout = browser->document()->documentLayout();
        const auto firstRect = layout->blockBoundingRect(first.block());
        const auto secondRect = layout->blockBoundingRect(second.block());
        QVERIFY(secondRect.top() > firstRect.bottom());
        const QPoint gap(textPoint.x(), qRound((firstRect.bottom() + secondRect.top()) / 2));
        QTest::mouseMove(browser->viewport(), gap);
        QCOMPARE(browser->viewport()->cursor().shape(), Qt::ArrowCursor);
        QTest::mouseMove(browser->viewport(), textPoint);
        QCOMPARE(browser->viewport()->cursor().shape(), Qt::IBeamCursor);
        QTest::mouseMove(browser->viewport(), QPoint(2, 2));
        QCOMPARE(browser->viewport()->cursor().shape(), Qt::ArrowCursor);
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
        auto send = view.findChild<IconButton*>("aiChatSend");
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

    void synopsisMenuCollectsParametersBeforeStarting()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setDocument(QUuid::createUuid(), "Screenplay / Draft", [] { return "Scene text"; });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setSynopsisGenerationAvaiable(true);
        view.setGenerationSynopsisOptions("Uses 200 words");
        view.setAvailableWords(1000);
        view.resize(320, 700);
        view.show();
        QSignalSpy generate(&view, &Ui::AiAssistantView::generateSynopsisRequested);
        auto toggle = view.findChild<IconButton*>("aiChatCommandsToggle");
        QTest::mouseClick(toggle, Qt::LeftButton);
        QVERIFY(view.findChild<QFrame*>("aiChatCommands")->isVisible());
        view.findChild<QPushButton*>("aiCommand_synopsis")->click();
        QVERIFY(!toggle->isChecked());
        QVERIFY(view.findChild<QFrame*>("aiToolFlowCard")->isVisible());
        QCOMPARE(generate.size(), 0);
        view.findChild<QPushButton*>("aiSynopsis_45")->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QVERIFY(view.findChild<QLabel*>("aiToolPrompt")->text().contains("45"));
        QCOMPARE(view.findChild<QLabel*>("aiToolCost")->text(), QString("Uses 200 words"));
        auto start = view.findChild<QPushButton*>("aiToolStart");
        QVERIFY(start && start->isEnabled());
        QCOMPARE(generate.size(), 0);
        session->setBackendAvailable(false);
        QVERIFY(!start->isEnabled());
        session->setBackendAvailable(true);
        QVERIFY(start->isEnabled());
        QTest::qWait(30);
        const auto screenshot = qEnvironmentVariable("AI_CHAT_REFERENCE_SCREENSHOT");
        if (!screenshot.isEmpty()) { QVERIFY(view.grab().save(screenshot)); }
        start->click();
        QCOMPARE(generate.size(), 1);
        QCOMPARE(generate.first().first().toInt(), 45);
        QVERIFY(!view.findChild<QFrame*>("aiToolFlowCard")->isVisible());
        QVERIFY(session->finishToolReply("synopsis", "Synopsis"));
    }

    void translationFlowUsesOneExplicitStart()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setDocument(QUuid::createUuid(), "Chapter", [] { return "Text"; });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setDocumentTranslationAvailable(true);
        view.setAvailableWords(100);
        QSignalSpy translate(&view, &Ui::AiAssistantView::translateDocumentRequested);
        view.findChild<QPushButton*>("aiCommand_translate")->click();
        view.findChild<TextField*>("aiChatInput")->setText("French");
        view.findChild<IconButton*>("aiChatSend")->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(translate.size(), 0);
        QVERIFY(view.findChild<QLabel*>("aiToolDestination")->text().contains("replace"));
        view.findChild<QPushButton*>("aiToolStart")->click();
        QCOMPARE(translate.size(), 1);
        QCOMPARE(translate.first().first().toString(), QString("French"));
        QVERIFY(!view.findChild<Dialog*>("aiTranslateConfirmation"));
        session->finishToolReply("translate-document", "Texte");
    }

    void clearingFromMenuRequiresConfirmationAndKeepsChat()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setBackendAvailable(true);
        QVERIFY(session->send("Old message"));
        session->finishReply("Old answer");
        const auto id = session->currentChat();
        QVERIFY(session->renameChat("Keep this name"));
        Ui::AiAssistantView view;
        auto clear = view.findChild<QAction*>("aiClearChat");
        QVERIFY(clear && clear->isEnabled());
        clear->trigger();
        auto dialog = view.findChild<Dialog*>("aiClearChatConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 0, {}, Dialog::RejectButton });
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(session->messages().size(), 3);
        clear->trigger();
        dialog = view.findChild<Dialog*>("aiClearChatConfirmation");
        QVERIFY(dialog);
        dialog->finished({ 1, {}, Dialog::AcceptCriticalButton });
        QCOMPARE(session->currentChat(), id);
        QCOMPARE(session->messages().size(), 1);
        QCOMPARE(session->chats().first().toObject().value("title").toString(), QString("Keep this name"));
    }

    void generationFlowCancelsWhenContextChanges()
    {
        auto session = Ui::AiChatSession::instance();
        session->setProject(QUuid::createUuid(), {});
        session->setDocument(QUuid::createUuid(), "Document", [] { return "Text"; });
        session->setBackendAvailable(true);
        Ui::AiAssistantView view;
        view.setAvailableWords(100);
        QSignalSpy generate(&view, &Ui::AiAssistantView::generateTextRequested);
        auto input = view.findChild<TextField*>("aiChatInput");
        auto send = view.findChild<IconButton*>("aiChatSend");
        view.findChild<QPushButton*>("aiCommand_generate")->click();
        input->setText("Describe a seaside town");
        send->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        QCOMPARE(generate.size(), 0);
        QVERIFY(view.findChild<QPushButton*>("aiToolStart"));
        session->setDocument(QUuid::createUuid(), "Other document", [] { return "Other text"; });
        QVERIFY(view.findChild<QFrame*>("aiToolFlowCard")->isHidden());
        QCOMPARE(generate.size(), 0);
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        view.findChild<QPushButton*>("aiCommand_generate")->click();
        input->setText("Describe a forest");
        send->click();
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        view.findChild<QPushButton*>("aiToolStart")->click();
        QCOMPARE(generate.size(), 1);
        QCOMPARE(generate.first().first().toString(), QString("Describe a forest"));
        session->finishToolReply("generate", "A forest");
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
        auto send = view.findChild<IconButton*>("aiChatSend");
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
        auto rename = view.findChild<QAction*>("aiRenameChat");
        QVERIFY(rename && rename->isEnabled());
        QTimer::singleShot(0, &view, [&view] {
            auto dialog = view.findChild<QInputDialog*>();
            if (dialog) {
                dialog->setTextValue("My renamed conversation");
                dialog->accept();
            }
        });
        rename->trigger();
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

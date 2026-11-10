#include "ai_assistant_view.h"
#include "ai_chat_session.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/chat/chat_message.h>
#include <ui/widgets/chat/chat_messages_view.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QApplication>
#include <QBoxLayout>
#include <QClipboard>
#include <QInputDialog>
#include <QJsonObject>
#include <QKeyEvent>
#include <QLineEdit>
#include <QRegularExpression>
#include <QScrollArea>
#include <QSet>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QTimer>
#include <algorithm>

namespace Ui {
class AiAssistantView::Implementation
{
public:
    bool isReadOnly = false;
    bool insertionAvailable = true;
    bool translationAvailable = false;
    bool synopsisAvailable = false;
    bool novelAvailable = false;
    bool scriptAvailable = false;
    int availableWords = 0;
    GenerationViewType generationViewType = GenerationViewType::Text;
    TextInsertPosition insertPosition = TextInsertPosition::AtCursorPosition;
    QString translationHint, synopsisHint, novelHint, scriptHint, promptHint;
    Body2Label* credits = nullptr;
    Body2LinkLabel* buyCredits = nullptr;
    struct Chat {
        ComboBox* chats = nullptr;
        QStandardItemModel* model = nullptr;
        IconButton* newChat = nullptr;
        IconButton* deleteChat = nullptr;
        Body2Label* context = nullptr;
        Widget* widget = nullptr;
        ChatMessagesView* history = nullptr;
        QScrollArea* historyContainer = nullptr;
        Body2Label* emptyHint = nullptr;
        ComboBox* commands = nullptr;
        TextField* input = nullptr;
        Button* send = nullptr;
        Button* insert = nullptr;
        IconButton* renameChat = nullptr;
        Body2Label* status = nullptr;
        QString lastReply;
        QString displayedChat;
        QHash<QString, QString> drafts;
        QHash<QString, int> scrollPositions;
        int scrollRevision = 0;
        bool scrollUpdatePending = false;
        bool pendingScrollToBottom = true;
        int pendingScrollPosition = 0;
    } chat;
};

AiAssistantView::AiAssistantView(QWidget* _parent)
    : Widget(_parent), d(new Implementation)
{
    initChat();
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(d->chat.widget, 1);
    auto footer = new QHBoxLayout;
    footer->setContentsMargins(12, 0, 12, 12);
    d->credits = new Body2Label(this);
    d->buyCredits = new Body2LinkLabel(this);
    footer->addWidget(d->credits);
    footer->addWidget(d->buyCredits);
    footer->addStretch();
    layout->addLayout(footer);
    connect(d->buyCredits, &Body2LinkLabel::clicked, this, &AiAssistantView::buyCreditsPressed);
}
AiAssistantView::~AiAssistantView() = default;
bool AiAssistantView::isReadOnly() const
{
    return d->isReadOnly;
}

void AiAssistantView::setReadOnly(bool value)
{
    d->isReadOnly = value;
    refreshChat();
}

void AiAssistantView::setInsertionAvailable(bool value)
{
    d->insertionAvailable = value;
    refreshChat();
}

void AiAssistantView::setDocumentTranslationAvailable(bool value)
{
    d->translationAvailable = value;
}

void AiAssistantView::setSynopsisGenerationAvaiable(bool value)
{
    d->synopsisAvailable = value;
}

void AiAssistantView::setNovelGenerationAvaiable(bool value)
{
    d->novelAvailable = value;
}

void AiAssistantView::setScriptGenerationAvaiable(bool value)
{
    d->scriptAvailable = value;
}

void AiAssistantView::setTranslationDocumentOption(const QString& value)
{
    d->translationHint = value;
    updateChatComposer();
}

void AiAssistantView::setGenerationSynopsisOptions(const QString& value)
{
    d->synopsisHint = value;
    updateChatComposer();
}

void AiAssistantView::setGenerationNovelOptions(const QString& value)
{
    d->novelHint = value;
    updateChatComposer();
}

void AiAssistantView::setGenerationScriptOptions(const QString& value)
{
    d->scriptHint = value;
    updateChatComposer();
}

void AiAssistantView::setGenerationViewType(GenerationViewType value)
{
    d->generationViewType = value;
}

void AiAssistantView::setGenerationPromptHint(const QString& value)
{
    d->promptHint = value;
    updateChatComposer();
}

void AiAssistantView::setGenerationPrompt(const QString& value)
{
    d->chat.input->setText("/generate " + value);
}

AiAssistantView::TextInsertPosition AiAssistantView::textInsertPosition() const
{
    return d->insertPosition;
}

void AiAssistantView::setRephraseResult(const QString& value)
{
    AiChatSession::instance()->finishToolReply("rephrase", value);
}

void AiAssistantView::setExpandResult(const QString& value)
{
    AiChatSession::instance()->finishToolReply("expand", value);
}

void AiAssistantView::setShortenResult(const QString& value)
{
    AiChatSession::instance()->finishToolReply("shorten", value);
}

void AiAssistantView::setInsertResult(const QString& value)
{
    AiChatSession::instance()->finishToolReply("insert", value);
}

void AiAssistantView::setSummarizeResult(const QString& value)
{
    AiChatSession::instance()->finishToolReply("summarize", value);
}

void AiAssistantView::setTransateResult(const QString& value)
{
    emit insertTextRequested(value);
}

void AiAssistantView::setGenerateSynopsisResult(const QString& value)
{
    emit insertTextRequested(value);
}

void AiAssistantView::setAvailableWords(int value)
{
    d->availableWords = value;
    d->credits->setText(value > 0 ? tr("%n word(s) available", nullptr, value) : tr("No words available"));
    refreshChat();
}
void AiAssistantView::updateTranslations()
{
    updateChatTranslations();
    d->buyCredits->setText(tr("Purchase credits"));
    setAvailableWords(d->availableWords);
}
void AiAssistantView::designSystemChangeEvent(DesignSystemChangeEvent* event)
{
    Widget::designSystemChangeEvent(event);
    setBackgroundColor(DesignSystem::color().primary());
    updateChatDesign();
    d->credits->setBackgroundColor(backgroundColor());
    d->credits->setTextColor(DesignSystem::color().onPrimary());
    d->buyCredits->setBackgroundColor(backgroundColor());
    d->buyCredits->setTextColor(DesignSystem::color().accent());
}

bool AiAssistantView::sendChatMessage(const QString& _message)
{
    auto session = AiChatSession::instance();
    const auto message = _message.trimmed();
    if (!message.startsWith('/')) {
        return session->send(message);
    }
    const auto match = QRegularExpression("^/([a-z]+)(?:\\s+([\\s\\S]*))?$").match(message);
    const auto command = match.captured(1);
    auto arguments = match.captured(2).trimmed();
    auto error = [this](const QString& text) { d->chat.input->setError(text); return false; };
    const QSet<QString> chatTools{ "rephrase", "expand", "shorten", "insert", "summarize" };
    const QSet<QString> documentTools{ "translate", "generate", "synopsis", "novel", "script", "character", "mindmap" };
    if (!chatTools.contains(command) && !documentTools.contains(command)) {
        return error(tr("Unknown command. Use /help to see available commands."));
    }
    if (!session->canSend()) {
        return false;
    }
    if (documentTools.contains(command) && (d->isReadOnly || session->documentId().isEmpty())) {
        return error(tr("Open an editable document to use this command."));
    }
    if (chatTools.contains(command)) {
        QString source = arguments;
        QString option;
        if (command == "rephrase") {
            const auto separator = arguments.indexOf('|');
            option = separator < 0 ? arguments : arguments.left(separator).trimmed();
            source = separator < 0 ? session->documentText() : arguments.mid(separator + 1).trimmed();
        } else if (command == "insert") {
            const auto separator = arguments.indexOf('|');
            if (separator < 0) {
                return error(tr("Use /insert text before | text after."));
            }
            source = arguments.left(separator).trimmed();
            option = arguments.mid(separator + 1).trimmed();
        } else if (source.isEmpty()) {
            source = session->documentText();
        }
        if (source.isEmpty()) {
            return error(tr("Add text after the command or open a document."));
        }
        if (TextHelper::wordsCount(source) > 1000 || TextHelper::wordsCount(option) > 1000) {
            return error(tr("This tool accepts up to 1000 words. Provide a shorter excerpt after the command."));
        }
        if (!session->startTool(message, command)) { return false; }
        if (command == "rephrase") { emit rephraseRequested(source, option); }
        else if (command == "expand") { emit expandRequested(source); }
        else if (command == "shorten") { emit shortenRequested(source); }
        else if (command == "summarize") { emit summarizeRequested(source); }
        else { emit insertRequested(source, option); }
        return true;
    }
    if (command == "translate") {
        const auto separator = arguments.indexOf('|');
        const auto language = (separator < 0 ? arguments : arguments.left(separator)).trimmed();
        if (language.isEmpty()) { return error(tr("Specify a language: /translate English.")); }
        if (separator >= 0) {
            const auto source = arguments.mid(separator + 1).trimmed();
            if (source.isEmpty() || TextHelper::wordsCount(source) > 1000) {
                return error(tr("Provide between 1 and 1000 words after the | separator."));
            }
            if (!session->startTool(message, "translate")) { return false; }
            emit translateRequested(source, language);
            return true;
        }
        if (!d->translationAvailable) { return error(tr("Whole-document translation is unavailable in this editor. Use /translate language | text.")); }
        const auto document = session->documentId();
        const auto chat = session->currentChat();
        auto dialog = new Dialog(this);
        dialog->setObjectName("aiTranslateConfirmation");
        connect(dialog, &Dialog::finished, this,
                [this, session, dialog, document, chat, message, language](const Dialog::ButtonInfo& button) {
            if (button.id == 1 && session->documentId() == document && session->currentChat() == chat
                && !d->isReadOnly && session->startTool(message, "translate-document")) {
                if (d->chat.input->text().trimmed() == message) { d->chat.input->clear(); }
                emit translateDocumentRequested(language);
            }
            dialog->hideDialog();
            dialog->deleteLater();
        });
        dialog->showDialog(tr("Translate document"),
                           tr("The translation will replace the contents of %1.").arg(session->documentName()),
                           { { 0, tr("Cancel"), Dialog::RejectButton }, { 1, tr("Translate"), Dialog::AcceptCriticalButton } });
        return false;
    }
    if (command == "synopsis" || command == "novel" || command == "script") {
        if ((command == "synopsis" && !d->synopsisAvailable)
            || (command == "novel" && !d->novelAvailable)
            || (command == "script" && !d->scriptAvailable)) {
            return error(tr("This conversion is unavailable for the current document."));
        }
        const int length = arguments.isEmpty() ? -1 : arguments.toInt();
        if (command == "synopsis" && length != -1 && length != 20 && length != 45) {
            return error(tr("Use /synopsis with 20, 45 or -1 words per scene."));
        }
        if (!session->startTool(message, command)) { return false; }
        if (command == "synopsis") { emit generateSynopsisRequested(length); }
        else if (command == "novel") { emit generateNovelRequested(); }
        else { emit generateScriptRequested(); }
        return true;
    }
    auto type = d->generationViewType;
    if (command == "character" && type != GenerationViewType::CharacterInformation) {
        return error(tr("Open a character document to use /character."));
    }
    if (command == "mindmap" && type != GenerationViewType::MindMap) {
        return error(tr("Open a mind map to use /mindmap."));
    }
    d->insertPosition = TextInsertPosition::AtCursorPosition;
    if (arguments.startsWith("--begin ")) { d->insertPosition = TextInsertPosition::AtBeginning; arguments.remove(0, 8); }
    else if (arguments.startsWith("--end ")) { d->insertPosition = TextInsertPosition::AtEnd; arguments.remove(0, 6); }
    else if (arguments.startsWith("--cursor ")) { arguments.remove(0, 9); }
    bool personal = true, physique = true, life = true, attitude = true, biography = true, image = false;
    if (type == GenerationViewType::CharacterInformation && arguments.startsWith("--")) {
        personal = physique = life = attitude = biography = false;
        while (arguments.startsWith("--")) {
            const auto end = arguments.indexOf(' ');
            const auto flag = end < 0 ? arguments : arguments.left(end);
            if (flag == "--personal") { personal = true; }
            else if (flag == "--physique") { physique = true; }
            else if (flag == "--life") { life = true; }
            else if (flag == "--attitude") { attitude = true; }
            else if (flag == "--biography") { biography = true; }
            else if (flag == "--image") { image = true; }
            else { return error(tr("Unknown character option. Use --personal, --physique, --life, --attitude, --biography or --image.")); }
            arguments = end < 0 ? QString() : arguments.mid(end + 1).trimmed();
        }
    }
    if (arguments.isEmpty() || TextHelper::wordsCount(arguments) > 1000) {
        return error(tr("Add a generation prompt of up to 1000 words."));
    }
    const int replyCount = type == GenerationViewType::CharacterInformation && image
        && (personal || physique || life || attitude || biography) ? 2 : 1;
    if (!session->startTool(message, "generate", replyCount)) { return false; }
    if (type == GenerationViewType::CharacterInformation) {
        emit generateCharacterRequested(arguments, personal, physique, life, attitude, biography, image);
    } else if (type == GenerationViewType::MindMap) {
        emit generateMindMapRequested(arguments);
    } else {
        emit generateTextRequested(arguments);
    }
    return true;
}

void AiAssistantView::initChat()
{
    d->chat.widget = new Widget(this);
    d->chat.chats = new ComboBox(d->chat.widget);
    d->chat.chats->setObjectName("aiChatSelector");
    d->chat.model = new QStandardItemModel(d->chat.widget);
    d->chat.chats->setModel(d->chat.model);
    d->chat.newChat = new IconButton(d->chat.widget);
    d->chat.newChat->setObjectName("aiNewChat");
    d->chat.newChat->setIcon(u8"\U000F0415");
    d->chat.deleteChat = new IconButton(d->chat.widget);
    d->chat.deleteChat->setObjectName("aiDeleteChat");
    d->chat.deleteChat->setIcon(u8"\U000F01B4");
    d->chat.context = new Body2Label(d->chat.widget);
    d->chat.history = new ChatMessagesView;
    d->chat.historyContainer = new QScrollArea(d->chat.widget);
    d->chat.historyContainer->setObjectName("aiChatHistoryContainer");
    d->chat.historyContainer->setFrameShape(QFrame::NoFrame);
    d->chat.historyContainer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->chat.historyContainer->setVerticalScrollBar(new ScrollBar);
    d->chat.historyContainer->setWidget(d->chat.history);
    d->chat.historyContainer->setWidgetResizable(true);
    d->chat.emptyHint = new Body2Label(d->chat.widget);
    d->chat.history->setObjectName("aiChatHistory");
    d->chat.commands = new ComboBox(d->chat.widget);
    d->chat.commands->setObjectName("aiChatCommands");
    const QStringList commands{ "/rephrase", "/expand", "/shorten", "/insert", "/summarize",
                                "/translate", "/synopsis", "/novel", "/script", "/generate",
                                "/character", "/mindmap", "/help" };
    auto commandsModel = new QStandardItemModel(d->chat.widget);
    for (const auto& command : commands) {
        auto item = new QStandardItem(command);
        item->setData(command, Qt::UserRole);
        commandsModel->appendRow(item);
    }
    d->chat.commands->setModel(commandsModel);
    d->chat.commands->setCurrentIndex({});
    d->chat.input = new TextField(d->chat.widget);
    d->chat.input->setObjectName("aiChatInput");
    d->chat.input->setEnterMakesNewLine(true);
    d->chat.input->installEventFilter(this);
    d->chat.input->setMinimumHeight(80);
    d->chat.input->setMaximumHeight(160);
    d->chat.send = new Button(d->chat.widget);
    d->chat.send->setObjectName("aiChatSend");
    d->chat.insert = new Button(d->chat.widget);
    d->chat.insert->setObjectName("aiChatInsert");

    d->chat.status = new Body2Label(d->chat.widget);

    auto layout = new QVBoxLayout(d->chat.widget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    auto header = new QHBoxLayout;
    header->addWidget(d->chat.chats, 1);
    header->addWidget(d->chat.newChat);
    d->chat.renameChat = new IconButton(d->chat.widget);
    d->chat.renameChat->setObjectName("aiRenameChat");
    d->chat.renameChat->setIcon(u8"\U000F03EB");
    header->addWidget(d->chat.renameChat);
    header->addWidget(d->chat.deleteChat);
    layout->addLayout(header);
    layout->addWidget(d->chat.context);
    layout->addWidget(d->chat.emptyHint);
    layout->addWidget(d->chat.historyContainer, 1);
    layout->addWidget(d->chat.status);
    layout->addWidget(d->chat.commands);
    layout->addWidget(d->chat.input);
    auto actions = new QHBoxLayout;

    actions->addStretch();
    actions->addWidget(d->chat.send);
    layout->addLayout(actions);
    layout->addWidget(d->chat.insert);

    auto session = AiChatSession::instance();
    connect(session, &AiChatSession::changed, this, &AiAssistantView::refreshChat);
    connect(session, &AiChatSession::errorOccurred, this,
            [this](const QString& _error) { d->chat.input->setError(_error); });
    connect(d->chat.newChat, &IconButton::clicked, session, [session] { session->createChat(); });
    connect(d->chat.renameChat, &IconButton::clicked, this, [this, session] {
        const auto id = session->currentChat();
        bool accepted = false;
        const auto title = QInputDialog::getText(this, tr("Rename chat"), tr("Chat name"),
            QLineEdit::Normal, d->chat.chats->currentText(), &accepted);
        if (accepted && session->currentChat() == id && !session->renameChat(title)) {
            d->chat.input->setError(tr("Enter a chat name between 1 and 160 characters."));
        }
    });
    connect(d->chat.deleteChat, &IconButton::clicked, this, [this, session] {
        const auto chatId = session->currentChat();
        auto dialog = new Dialog(this);
        dialog->setObjectName("aiDeleteChatConfirmation");
        connect(dialog, &Dialog::finished, this,
                [this, session, dialog, chatId](const Dialog::ButtonInfo& _button) {
                    if (_button.id == 1 && session->currentChat() == chatId) {
                        session->deleteChat();
                        d->chat.drafts.remove(chatId);
                        d->chat.scrollPositions.remove(chatId);
                    }
                    dialog->hideDialog();
                    dialog->deleteLater();
                });
        dialog->showDialog(tr("Delete chat"), tr("This conversation will be permanently deleted from this device."),
                           { { 0, tr("Cancel"), Dialog::RejectButton },
                             { 1, tr("Delete"), Dialog::AcceptCriticalButton } });
    });
    connect(d->chat.chats, &ComboBox::currentIndexChanged, this, [this, session](const QModelIndex& _index) {
        if (_index.isValid()) {
            session->selectChat(_index.data(Qt::UserRole).toString());
        }
    });
    connect(d->chat.commands, &ComboBox::currentIndexChanged, this, [this](const QModelIndex& _index) {
        if (!_index.isValid()) {
            return;
        }
        const auto command = _index.data(Qt::UserRole).toString();
        auto text = d->chat.input->text();
        text.remove(QRegularExpression(QStringLiteral("^/[a-z]+(?:\\s+|$)")));
        d->chat.input->setText(command + " " + text);
        d->chat.input->setFocus();
        const QSignalBlocker blocker(d->chat.commands);
        d->chat.commands->setCurrentIndex({});
    });
    auto send = [this, session] {
        if (d->chat.input->text().trimmed() == "/help") {
            StandardDialog::information(this, tr("Commands"), tr(
                "/rephrase style | text — rephrase text (or the current document)\n"
                "/expand, /shorten, /summarize [text] — reply in chat\n"
                "/insert text before | text after — write a transition in chat\n"
                "/translate language [| text] — translate into the document\n"
                "/generate [--begin|--cursor|--end] prompt — write into the document\n"
                "/synopsis [20|45|-1], /novel, /script — use the document conversion tools\n"
                "/character prompt, /mindmap prompt — generate in the current editor\n"
                "Document translation replaces its content and asks for confirmation."));
            return;
        }
        const auto text = d->chat.input->text();
        if (d->availableWords > 0 && sendChatMessage(text)) {
            d->chat.input->clear();
            d->chat.drafts.remove(session->currentChat());
            d->chat.input->clearError();
        } else {
            d->chat.input->setText(text);
        }
    };
    connect(d->chat.send, &Button::clicked, this, send);
    connect(d->chat.input, &TextField::enterPressed, this, send);
    connect(d->chat.input, &TextField::textChanged, this, [this, session] {
        updateChatComposer();
    });

    connect(d->chat.insert, &Button::clicked, this, [this] { emit insertTextRequested(d->chat.lastReply); });
    updateChatTranslations();
    refreshChat();
    connect(d->chat.history, &ChatMessagesView::messageContextMenuRequested, this,
            [this](int _index) {
                const auto messages = d->chat.history->messages();
                if (_index < 0 || _index >= messages.size()) {
                    return;
                }
                auto menu = new ContextMenu(this);
                auto copy = new QAction(tr("Copy"), menu);
                const auto selected = d->chat.history->selectedText();
                const auto text = selected.isEmpty() ? messages.at(_index).text() : selected;
                connect(copy, &QAction::triggered, this,
                        [text] { QApplication::clipboard()->setText(text); });
                QVector<QAction*> actions{ copy };
                if (d->insertionAvailable && !d->isReadOnly
                    && messages.at(_index).author().email() == "ai-chat-assistant") {
                    auto insert = new QAction(tr("Insert into current document"), menu);
                    connect(insert, &QAction::triggered, this,
                            [this, text] { emit insertTextRequested(text); });
                    actions.append(insert);
                }
                menu->setActions(actions);
                menu->setBackgroundColor(DesignSystem::color().background());
                menu->setTextColor(DesignSystem::color().onBackground());
                connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);
                menu->showContextMenu(QCursor::pos());
            });

}

bool AiAssistantView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->chat.input && _event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent*>(_event);
        if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
            && !(key->modifiers() & Qt::ShiftModifier)) {
            if (d->chat.send->isEnabled()) {
                d->chat.send->click();
            }
            return true;
        }
    }
    return Widget::eventFilter(_watched, _event);
}

void AiAssistantView::refreshChat()
{
    auto session = AiChatSession::instance();
    const bool switchedChat = d->chat.displayedChat != session->currentChat();
    if (switchedChat) {
        // Keep unsent drafts scoped to the conversation when switching between chats.
        if (!d->chat.displayedChat.isEmpty()) {
            d->chat.drafts.insert(d->chat.displayedChat, d->chat.input->text());
            const auto scroll = d->chat.historyContainer->verticalScrollBar();
            d->chat.scrollPositions.insert(d->chat.displayedChat,
                                          d->chat.scrollUpdatePending
                                              ? (d->chat.pendingScrollToBottom ? -1 : d->chat.pendingScrollPosition)
                                              : (scroll->value() >= scroll->maximum() - 4 ? -1 : scroll->value()));
        }
        d->chat.displayedChat = session->currentChat();
        d->chat.input->setText(d->chat.drafts.value(d->chat.displayedChat));
        d->chat.input->clearError();
    }
    const QSignalBlocker blocker(d->chat.chats);
    d->chat.model->clear();
    for (const auto& value : session->chats()) {
        const auto chat = value.toObject();
        auto item = new QStandardItem(chat.value("title").toString().simplified());
        item->setData(chat.value("id").toString(), Qt::UserRole);
        d->chat.model->appendRow(item);
        if (chat.value("id").toString() == session->currentChat()) {
            d->chat.chats->setCurrentIndex(item->index());
        }
    }
    if (session->currentChat().isEmpty()) {
        d->chat.chats->setCurrentIndex({});
    }
    d->chat.deleteChat->setEnabled(!session->currentChat().isEmpty());
    d->chat.renameChat->setEnabled(!session->currentChat().isEmpty());
    d->chat.context->setText(session->documentName().isEmpty() ? tr("Document context cleared")
                                                        : tr("Current document: %1").arg(session->documentName()));
    d->chat.context->setToolTip(d->chat.context->text());
    QVector<ChatMessage> messages;
    const User currentUser(tr("You"), QStringLiteral("ai-chat-user"));
    const User assistant(tr("Assistant"), QStringLiteral("ai-chat-assistant"));
    d->chat.lastReply.clear();
    for (const auto& value : session->messages()) {
        const auto message = value.toObject();
        const auto role = message.value("role").toString();
        const auto text = message.value("text").toString();
        const auto author = role == "user" ? currentUser : role == "assistant" ? assistant
            : User(role == "error" ? tr("Error") : tr("Context"), "ai-chat-" + role);
        messages.append(ChatMessage(
            QDateTime::fromString(message.value("created_at").toString(), Qt::ISODateWithMs)
                .toLocalTime(),
            text, author, role == "context" || role == "error"));
        messages.last().setMarkdown(role == "assistant");
        if (role == "assistant") {
            d->chat.lastReply = text;
        }
    }
    const auto scroll = d->chat.historyContainer->verticalScrollBar();
    const auto oldScroll = switchedChat
        ? d->chat.scrollPositions.value(session->currentChat(), -1)
        : d->chat.scrollUpdatePending ? d->chat.pendingScrollPosition : scroll->value();
    const auto atBottom = switchedChat ? oldScroll == -1
        : d->chat.scrollUpdatePending ? d->chat.pendingScrollToBottom
                                     : oldScroll >= scroll->maximum() - 4;
    d->chat.history->setCurrentUser(currentUser);
    d->chat.history->setMessages(messages);
    const bool hasConversation = std::any_of(messages.cbegin(), messages.cend(),
                                           [](const ChatMessage& message) { return !message.isSistemMessage(); });
    d->chat.emptyHint->setVisible(!hasConversation);
    // QScrollArea updates its range after the child has recalculated heightForWidth.
    d->chat.scrollUpdatePending = true;
    d->chat.pendingScrollToBottom = atBottom;
    d->chat.pendingScrollPosition = oldScroll;
    const auto revision = ++d->chat.scrollRevision;
    QTimer::singleShot(0, this, [this, oldScroll, atBottom, revision] {
        if (revision != d->chat.scrollRevision) {
            return;
        }
        const auto scroll = d->chat.historyContainer->verticalScrollBar();
        scroll->setValue(atBottom ? scroll->maximum() : oldScroll);
        d->chat.scrollUpdatePending = false;
    });
    updateChatComposer();

    d->chat.insert->setVisible((d->insertionAvailable && !d->isReadOnly) && !d->chat.lastReply.isEmpty());
}

void AiAssistantView::updateChatComposer()
{
    auto session = AiChatSession::instance();
    const auto text = d->chat.input->text().trimmed();
    const auto command = text.section(QRegularExpression("\\s+"), 0, 0);
    QString helper = tr("Enter to send · Shift+Enter for a new line");
    if (command == "/rephrase") {
        helper = tr("/rephrase style | text — reply in chat");
    } else if (command == "/insert") {
        helper = tr("/insert text before | text after — reply in chat");
    } else if (command == "/expand" || command == "/shorten" || command == "/summarize") {
        helper = tr("%1 [text] — reply in chat; omit text to use the current document").arg(command);
    } else if (command == "/translate") {
        helper = tr("/translate language [| text] — write into the document");
        if (!text.contains('|') && !d->translationHint.isEmpty()) { helper += "\n" + d->translationHint; }
    } else if (command == "/generate" || command == "/character" || command == "/mindmap") {
        helper = tr("%1 prompt — write into the document").arg(command);
        if (!d->promptHint.isEmpty()) { helper += "\n" + d->promptHint; }
    } else if (command == "/synopsis") {
        helper = tr("/synopsis [20|45|-1] — words per scene; -1 means no limit");
        if (!d->synopsisHint.isEmpty()) { helper += "\n" + d->synopsisHint; }
    } else if (command == "/novel" && !d->novelHint.isEmpty()) {
        helper = d->novelHint;
    } else if (command == "/script" && !d->scriptHint.isEmpty()) {
        helper = d->scriptHint;
    }
    d->chat.input->setHelper(helper);
    d->chat.send->setEnabled(text == "/help"
                            || (!text.isEmpty() && d->availableWords > 0 && session->canSend()));
    const auto status = session->hasPendingReply()
        ? (session->isWaitingForCurrentChat() ? tr("Waiting for the assistant…")
                                             : tr("The assistant is answering another chat…"))
        : !session->canSend() ? tr("The assistant is unavailable. You can still read your chats.")
        : d->availableWords <= 0 ? tr("Replenish credits to send a message.") : QString();
    d->chat.status->setText(status);
    d->chat.status->setVisible(!status.isEmpty());
}

void AiAssistantView::updateChatTranslations()
{
    d->chat.emptyHint->setText(tr("Discuss the current document, ask a question, or choose a /command."));
    d->chat.chats->setLabel(tr("Chat"));
    d->chat.chats->setPlaceholderText(tr("New chat"));
    d->chat.newChat->setToolTip(tr("New chat"));
    d->chat.deleteChat->setToolTip(tr("Delete chat"));
    d->chat.commands->setLabel(tr("Commands"));
    d->chat.commands->setPlaceholderText(tr("Choose a /command"));
    d->chat.input->setLabel(tr("Message"));
    d->chat.input->setPlaceholderText(tr("Ask about this document or type /help"));
    d->chat.input->setHelper(tr("Enter to send · Shift+Enter for a new line"));
    d->chat.send->setText(tr("Send"));
    d->chat.insert->setText(tr("Insert last reply"));
    d->chat.insert->setToolTip(tr("Insert last reply into the current document"));

    const QStringList descriptions{ tr("Rephrase"), tr("Expand"), tr("Shorten"), tr("Insert"),
        tr("Summarize"), tr("Translate"), tr("Generate synopsis"), tr("Generate novel"),
        tr("Generate script"), tr("Generate"), tr("Character"), tr("Mind map"), tr("Help") };
    auto model = qobject_cast<QStandardItemModel*>(d->chat.commands->model());
    for (int row = 0; row < model->rowCount(); ++row) {
        auto item = model->item(row);
        item->setText(descriptions.at(row) + "  " + item->data(Qt::UserRole).toString());
    }
    d->chat.renameChat->setToolTip(tr("Rename chat"));
    d->chat.renameChat->setAccessibleName(tr("Rename chat"));
    d->chat.newChat->setAccessibleName(tr("New chat"));
    d->chat.deleteChat->setAccessibleName(tr("Delete chat"));
}

void AiAssistantView::updateChatDesign()
{
    const auto background = DesignSystem::color().primary();
    const auto foreground = DesignSystem::color().onPrimary();
    d->chat.widget->setBackgroundColor(background);
    for (auto field : { static_cast<TextField*>(d->chat.chats), static_cast<TextField*>(d->chat.commands), d->chat.input }) {
        field->setBackgroundColor(background);
        field->setTextColor(foreground);
    }
    for (auto label : { d->chat.context, d->chat.status, d->chat.emptyHint }) {
        label->setBackgroundColor(background);
        label->setTextColor(foreground);
    }
    for (auto button : { d->chat.send, d->chat.insert }) {
        button->setBackgroundColor(ColorHelper::nearby(background));
        button->setTextColor(foreground);
    }
    for (auto button : { d->chat.newChat, d->chat.deleteChat, d->chat.renameChat }) {
        button->setBackgroundColor(background);
        button->setTextColor(foreground);
    }
    d->chat.history->setBackgroundColor(background);
    d->chat.history->setTextColor(foreground);
    auto palette = d->chat.historyContainer->palette();
    palette.setColor(QPalette::Base, background);
    palette.setColor(QPalette::Window, background);
    d->chat.historyContainer->setPalette(palette);
}


} // namespace Ui

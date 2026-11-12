#include "ai_assistant_view.h"
#include "ai_chat_session.h"

#include <ui/design_system/design_system.h>
#include <ui/layouts/flow_layout/flow_layout.h>
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
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
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
#include <QPushButton>
#include <QLabel>
#include <QPointer>
#include <QTextBrowser>
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
        QAction* newChat = nullptr;
        QAction* deleteChat = nullptr;
        QAction* clearChat = nullptr;
        Widget* contextScope = nullptr;
        RadioButton* documentScope = nullptr;
        RadioButton* projectScope = nullptr;
        RadioButton* noDocumentScope = nullptr;
        Widget* widget = nullptr;
        ChatMessagesView* history = nullptr;
        QScrollArea* historyContainer = nullptr;
        QFrame* commands = nullptr;
        IconButton* commandsToggle = nullptr;
        IconButton* chatMenu = nullptr;
        QFrame* composer = nullptr;
        QFrame* flowCard = nullptr;
        QPointer<QPushButton> flowStart;
        QPointer<QLabel> flowCost;
        QVector<QPair<QString, QPushButton*>> commandButtons;
        TextField* input = nullptr;
        IconButton* send = nullptr;
        QAction* insert = nullptr;
        QAction* renameChat = nullptr;
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
    struct ToolFlow {
        enum Step { None, Arguments, SynopsisLength, Confirm } step = None;
        QString command, message, originalDraft, chatId, documentId, documentName;
        int synopsisLength = -1;
    } flow;
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
    footer->addStretch();
    footer->addWidget(d->buyCredits);
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
    updateChatComposer();
}

void AiAssistantView::setSynopsisGenerationAvaiable(bool value)
{
    d->synopsisAvailable = value;
    updateChatComposer();
}

void AiAssistantView::setNovelGenerationAvaiable(bool value)
{
    d->novelAvailable = value;
    updateChatComposer();
}

void AiAssistantView::setScriptGenerationAvaiable(bool value)
{
    d->scriptAvailable = value;
    updateChatComposer();
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
    updateChatComposer();
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
    d->credits->setText(value > 0 ? tr("%Ln word(s)", nullptr, value) : tr("No words available"));
    refreshChat();
}
void AiAssistantView::updateTranslations()
{
    updateChatTranslations();
    d->buyCredits->setText(tr("Top up"));
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

bool AiAssistantView::sendChatMessage(const QString& _message, bool _confirmed)
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
        if (_confirmed) {
            if (!session->startTool(message, "translate-document")) { return false; }
            emit translateDocumentRequested(language);
            return true;
        }
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

bool AiAssistantView::toolWritesDocument(const QString& _command)
{
    return QSet<QString>{ "/translate", "/generate", "/synopsis", "/novel", "/script",
                          "/character", "/mindmap" }.contains(_command);
}

void AiAssistantView::positionCommandMenu()
{
    if (d->chat.commands->isHidden()) { return; }
    const auto viewport = d->chat.historyContainer->viewport();
    const int width = qMax(1, viewport->width() - 16);
    const int height = d->chat.commands->layout()->heightForWidth(width);
    d->chat.commands->setGeometry(8, qMax(0, viewport->height() - height - 8), width, height);
    d->chat.commands->raise();
}

void AiAssistantView::cancelToolFlow(bool _restoreDraft)
{
    if (d->flow.step == Implementation::ToolFlow::None) { return; }
    const auto originalDraft = d->flow.originalDraft;
    d->flow = {};
    if (_restoreDraft) { d->chat.input->setText(originalDraft); }
    renderToolFlow();
    refreshChat();
}

void AiAssistantView::beginToolFlow(const QString& _command)
{
    auto session = AiChatSession::instance();
    d->chat.commandsToggle->setChecked(false);
    if (_command == "/help") {
        const auto draft = d->chat.input->text();
        d->chat.input->setText("/help");
        d->chat.send->click();
        d->chat.input->setText(draft);
        return;
    }
    if (toolWritesDocument(_command) && (d->isReadOnly || session->documentId().isEmpty())) {
        d->chat.input->setError(tr("Open an editable document to use this command."));
        return;
    }
    if ((_command == "/synopsis" && !d->synopsisAvailable)
        || (_command == "/novel" && !d->novelAvailable)
        || (_command == "/script" && !d->scriptAvailable)) {
        d->chat.input->setError(tr("This conversion is unavailable for the current document."));
        return;
    }
    if (d->flow.step != Implementation::ToolFlow::None) { cancelToolFlow(true); }
    const auto draft = d->chat.input->text();
    if (session->currentChat().isEmpty() && !session->createChat()) { return; }
    d->flow.originalDraft = draft;
    d->flow.command = _command;
    d->flow.chatId = session->currentChat();
    d->flow.documentId = session->documentId();
    d->flow.documentName = session->documentName();
    d->flow.message = _command;
    auto source = draft.trimmed();
    if (source.startsWith('/')) { source.clear(); }
    d->chat.input->clear();
    d->chat.input->clearError();
    if (_command == "/synopsis") {
        d->flow.step = Implementation::ToolFlow::SynopsisLength;
    } else if (_command == "/novel" || _command == "/script"
               || _command == "/expand" || _command == "/shorten" || _command == "/summarize") {
        d->flow.step = Implementation::ToolFlow::Confirm;
        if (!toolWritesDocument(_command) && !source.isEmpty()) { d->flow.message += " " + source; }
    } else if ((_command == "/generate" || _command == "/character" || _command == "/mindmap")
               && !source.isEmpty()) {
        d->flow.step = Implementation::ToolFlow::Confirm;
        d->flow.message += " " + source;
    } else {
        d->flow.step = Implementation::ToolFlow::Arguments;
        if (_command == "/insert") { d->chat.input->setText(source); }
    }
    renderToolFlow();
    d->chat.input->setFocus();
}

void AiAssistantView::prepareToolArguments(const QString& _text)
{
    const auto text = _text.trimmed();
    if (text.isEmpty()) { return; }
    d->flow.message = d->flow.command + " " + text;
    auto source = d->flow.originalDraft.trimmed();
    if (source.startsWith('/')) { source.clear(); }
    if ((d->flow.command == "/rephrase" || d->flow.command == "/translate")
        && !source.isEmpty() && !text.contains('|')) {
        d->flow.message += " | " + source;
    }
    d->flow.step = Implementation::ToolFlow::Confirm;
    d->chat.input->clear();
    renderToolFlow();
}

void AiAssistantView::renderToolFlow()
{
    auto layout = d->chat.flowCard->layout();
    std::function<void(QLayout*)> clearLayout = [&clearLayout](QLayout* target) {
        while (auto item = target->takeAt(0)) {
            if (item->layout()) { clearLayout(item->layout()); }
            if (item->widget()) {
                item->widget()->hide();
                item->widget()->deleteLater();
            }
            delete item;
        }
    };
    clearLayout(layout);
    d->chat.flowStart = nullptr;
    d->chat.flowCost = nullptr;
    d->chat.input->setPlaceholderText(tr("Message"));
    if (d->flow.step == Implementation::ToolFlow::None) {
        d->chat.flowCard->hide();
        d->chat.history->setFooterWidget(d->chat.flowCard);
        updateChatComposer();
        return;
    }
    const auto background = DesignSystem::color().primary();
    const auto foreground = DesignSystem::color().onPrimary();
    const auto accent = DesignSystem::color().accent();
    const auto muted = ColorHelper::flattenAlpha(ColorHelper::transparent(foreground, 0.55), background);
    d->chat.flowCard->setStyleSheet(QString(
        "QFrame#aiToolFlowCard { background: %1; border-radius: 8px; } "
        "QLabel { color: %2; background: transparent; } "
        "QPushButton { color: %2; background: transparent; border: 1px solid %3; border-radius: 12px; padding: 6px 8px; } "
        "QPushButton:hover { background: %3; } "
        "QPushButton#aiToolStart { border-color: %4; color: %4; border-radius: 6px; padding: 10px; } "
        "QPushButton#aiToolBack, QPushButton#aiToolCancel { border: none; color: %3; padding: 4px; } "
        "QPushButton:disabled { color: %3; border-color: %3; }")
        .arg(background.name(), foreground.name(), muted.name(), accent.name()));
    auto addLabel = [this, layout](const QString& text, const char* name) {
        auto label = new QLabel(text, d->chat.flowCard);
        label->setObjectName(name);
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        label->setFont(DesignSystem::font().body2());
        layout->addWidget(label);
        return label;
    };
    auto addButton = [this, layout](const QString& text, const QString& name) {
        auto button = new QPushButton(text, d->chat.flowCard);
        button->setObjectName(name);
        button->setCursor(Qt::PointingHandCursor);
        button->setFont(DesignSystem::font().body2());
        layout->addWidget(button);
        return button;
    };
    QPushButton* back = nullptr;
    if (d->flow.step == Implementation::ToolFlow::SynopsisLength) {
        addLabel(tr("Choose the synopsis length — words per scene."), "aiToolPrompt");
        const QVector<QPair<int, QString>> choices{
            { 20, tr("Short (20 words per scene)") },
            { 45, tr("Medium (45 words per scene)") },
            { -1, tr("No limit") }
        };
        for (const auto& choice : choices) {
            auto button = addButton(choice.second, "aiSynopsis_" + QString::number(choice.first));
            connect(button, &QPushButton::clicked, this, [this, choice] {
                d->flow.synopsisLength = choice.first;
                d->flow.message = "/synopsis " + QString::number(choice.first);
                d->flow.step = Implementation::ToolFlow::Confirm;
                renderToolFlow();
            });
        }
    } else if (d->flow.step == Implementation::ToolFlow::Arguments) {
        QString question;
        if (d->flow.command == "/rephrase") {
            question = tr("How should I rephrase the text? Describe the style in your next message.");
            d->chat.input->setPlaceholderText(tr("For example: simpler and more vivid"));
        } else if (d->flow.command == "/translate") {
            question = tr("Which language should I translate into? Write the language in your next message.");
            d->chat.input->setPlaceholderText(tr("For example: English"));
        } else if (d->flow.command == "/insert") {
            question = tr("Send the text before and after the insertion, separated by |.");
        } else if (d->flow.command == "/expand" || d->flow.command == "/shorten" || d->flow.command == "/summarize") {
            question = tr("Send the text you would like to work on.");
        } else {
            question = tr("What would you like to create? Describe it in your next message.");
            d->chat.input->setPlaceholderText(tr("Describe what to create"));
        }
        addLabel(question, "aiToolPrompt");
    } else {
        QString title;
        for (const auto& command : d->chat.commandButtons) {
            if (command.first == d->flow.command) { title = command.second->text(); break; }
        }
        if (d->flow.command == "/synopsis") {
            title += " · " + (d->flow.synopsisLength == -1 ? tr("No limit")
                               : tr("%1 words per scene").arg(d->flow.synopsisLength));
        }
        addLabel(title, "aiToolPrompt");
        if (d->flow.command != "/synopsis") {
            const auto arguments = d->flow.message.mid(d->flow.command.size()).trimmed();
            if (!arguments.isEmpty()) { addLabel(arguments.left(300), "aiToolArguments"); }
        }
        const bool replacing = d->flow.command == "/translate" && !d->flow.message.contains('|');
        addLabel(replacing ? tr("The translation will replace the contents of %1.").arg(d->flow.documentName)
                 : toolWritesDocument(d->flow.command)
                    ? tr("The result will be added to %1.").arg(d->flow.documentName)
                    : tr("The result will appear here in the chat."), "aiToolDestination");
        QString cost;
        if (d->flow.command == "/synopsis") { cost = d->synopsisHint; }
        else if (d->flow.command == "/novel") { cost = d->novelHint; }
        else if (d->flow.command == "/script") { cost = d->scriptHint; }
        else if (replacing) { cost = d->translationHint; }
        d->chat.flowStart = addButton(tr("Start"), "aiToolStart");
        connect(d->chat.flowStart, &QPushButton::clicked, this, [this] {
            auto session = AiChatSession::instance();
            if (session->currentChat() != d->flow.chatId || session->documentId() != d->flow.documentId
                || session->documentName() != d->flow.documentName) {
                cancelToolFlow(true);
                return;
            }
            if (d->availableWords > 0 && sendChatMessage(d->flow.message, true)) {
                cancelToolFlow(false);
                d->chat.input->clear();
                d->chat.input->clearError();
            }
        });
        if (!cost.isEmpty()) {
            d->chat.flowCost = addLabel(cost, "aiToolCost");
            d->chat.flowCost->setAlignment(Qt::AlignCenter);
            d->chat.flowCost->setStyleSheet(QString("color: %1;").arg(accent.name()));
        }
        back = addButton(tr("Change"), "aiToolBack");
        layout->removeWidget(back);
        connect(back, &QPushButton::clicked, this, [this] {
            if (d->flow.command == "/synopsis") {
                d->flow.step = Implementation::ToolFlow::SynopsisLength;
            } else {
                d->flow.step = Implementation::ToolFlow::Arguments;
                d->chat.input->setText(d->flow.message.mid(d->flow.command.size()).trimmed());
            }
            renderToolFlow();
            d->chat.input->setFocus();
        });
        back->setVisible(d->flow.command != "/novel" && d->flow.command != "/script");
    }
    auto cancel = addButton(tr("Cancel"), "aiToolCancel");
    layout->removeWidget(cancel);
    auto secondaryActions = new QHBoxLayout;
    if (back) { secondaryActions->addWidget(back); }
    secondaryActions->addWidget(cancel);
    static_cast<QVBoxLayout*>(layout)->addLayout(secondaryActions);
    connect(cancel, &QPushButton::clicked, this, [this] { cancelToolFlow(true); });
    d->chat.flowCard->show();
    refreshChat();
    d->chat.history->setFooterWidget(d->chat.flowCard);
    d->chat.scrollUpdatePending = true;
    d->chat.pendingScrollToBottom = true;
    const auto revision = ++d->chat.scrollRevision;
    QTimer::singleShot(0, this, [this, revision] {
        if (revision != d->chat.scrollRevision) { return; }
        const auto scroll = d->chat.historyContainer->verticalScrollBar();
        scroll->setValue(scroll->maximum());
        d->chat.scrollUpdatePending = false;
    });
    updateChatComposer();
}

void AiAssistantView::initChat()
{
    d->chat.widget = new Widget(this);
    d->chat.chats = new ComboBox(d->chat.widget);
    d->chat.chats->setObjectName("aiChatSelector");
    d->chat.model = new QStandardItemModel(d->chat.widget);
    d->chat.chats->setModel(d->chat.model);
    d->chat.newChat = new QAction(d->chat.widget);
    d->chat.newChat->setObjectName("aiNewChat");

    d->chat.deleteChat = new QAction(d->chat.widget);
    d->chat.deleteChat->setObjectName("aiDeleteChat");
    d->chat.clearChat = new QAction(d->chat.widget);
    d->chat.clearChat->setObjectName("aiClearChat");

    d->chat.contextScope = new Widget(d->chat.widget);
    d->chat.contextScope->setObjectName("aiContextScope");
    d->chat.documentScope = new RadioButton(d->chat.contextScope);
    d->chat.documentScope->setObjectName("aiDocumentScope");
    d->chat.projectScope = new RadioButton(d->chat.contextScope);
    d->chat.projectScope->setObjectName("aiProjectScope");
    d->chat.noDocumentScope = new RadioButton(d->chat.contextScope);
    d->chat.noDocumentScope->setObjectName("aiNoDocumentScope");
    auto scopeGroup = new RadioButtonGroup(d->chat.contextScope);
    auto scopeLayout = new FlowLayout(d->chat.contextScope, 0, 8, 0);
    for (auto button : { d->chat.documentScope, d->chat.projectScope, d->chat.noDocumentScope }) {
        scopeGroup->add(button);
        scopeLayout->addWidget(button);
    }
    connect(d->chat.documentScope, &RadioButton::checkedChanged, this, [](bool checked) {
        if (checked) { AiChatSession::instance()->setContextScope(AiChatSession::ContextScope::Document); }
    });
    connect(d->chat.projectScope, &RadioButton::checkedChanged, this, [](bool checked) {
        if (checked) { AiChatSession::instance()->setContextScope(AiChatSession::ContextScope::Project); }
    });
    connect(d->chat.noDocumentScope, &RadioButton::checkedChanged, this, [](bool checked) {
        if (checked) { AiChatSession::instance()->setContextScope(AiChatSession::ContextScope::None); }
    });
    d->chat.history = new ChatMessagesView;
    d->chat.history->setConversationStyle(true);
    d->chat.history->findChild<QTextBrowser*>("chatMessageText")->viewport()->installEventFilter(this);
    d->chat.historyContainer = new QScrollArea(d->chat.widget);
    d->chat.historyContainer->setObjectName("aiChatHistoryContainer");
    d->chat.historyContainer->setFrameShape(QFrame::NoFrame);
    d->chat.historyContainer->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->chat.historyContainer->setVerticalScrollBar(new ScrollBar);
    d->chat.historyContainer->setWidget(d->chat.history);
    d->chat.historyContainer->setWidgetResizable(true);
    d->chat.history->setObjectName("aiChatHistory");
    d->chat.commands = new QFrame(d->chat.historyContainer->viewport());
    d->chat.commands->setObjectName("aiChatCommands");
    auto commandsLayout = new FlowLayout(d->chat.commands, 8, 6, 6);
    const QStringList commands{ "/rephrase", "/translate", "/expand", "/shorten", "/insert", "/summarize",
                                "/generate", "/synopsis", "/novel", "/script", "/character", "/mindmap", "/help" };
    for (const auto& command : commands) {
        auto button = new QPushButton(d->chat.commands);
        button->setObjectName("aiCommand_" + command.mid(1));
        button->setCursor(Qt::PointingHandCursor);
        button->setToolTip(command);
        commandsLayout->addWidget(button);
        d->chat.commandButtons.append({ command, button });
        connect(button, &QPushButton::clicked, this, [this, command] { beginToolFlow(command); });
    }
    d->chat.commands->hide();
    d->chat.historyContainer->viewport()->installEventFilter(this);
    d->chat.composer = new QFrame(d->chat.widget);
    d->chat.composer->setObjectName("aiChatComposer");
    d->chat.commandsToggle = new IconButton(d->chat.composer);
    d->chat.commandsToggle->setObjectName("aiChatCommandsToggle");
    d->chat.commandsToggle->setCheckable(true);
    d->chat.commandsToggle->setFocusPolicy(Qt::StrongFocus);
    d->chat.commandsToggle->installEventFilter(this);
    d->chat.commandsToggle->setIcon(u8"\U000F035C");
    connect(d->chat.commandsToggle, &IconButton::checkedChanged, this, [this](bool checked) {
        d->chat.commands->setVisible(checked);
        d->chat.commandsToggle->setIcon(checked ? u8"\U000F0156" : u8"\U000F035C");
        positionCommandMenu();
    });
    d->chat.flowCard = new QFrame;
    d->chat.flowCard->setObjectName("aiToolFlowCard");
    auto flowLayout = new QVBoxLayout(d->chat.flowCard);
    flowLayout->setContentsMargins(12, 12, 12, 12);
    flowLayout->setSpacing(8);
    d->chat.flowCard->hide();
    d->chat.history->setFooterWidget(d->chat.flowCard);
    d->chat.input = new TextField(d->chat.widget);
    d->chat.input->setObjectName("aiChatInput");
    d->chat.input->setDefaultMarginsEnabled(false);
    d->chat.input->setUnderlineDecorationVisible(false);
    d->chat.input->setEnterMakesNewLine(true);
    d->chat.input->installEventFilter(this);
    d->chat.input->setLabelVisible(false);
    d->chat.input->setMinimumHeight(48);
    d->chat.input->setMaximumHeight(160);
    d->chat.send = new IconButton(d->chat.composer);
    d->chat.send->setIcon(u8"\U000F048A");
    d->chat.send->setObjectName("aiChatSend");
    d->chat.insert = new QAction(d->chat.widget);
    d->chat.insert->setObjectName("aiChatInsert");

    d->chat.status = new Body2Label(d->chat.widget);

    auto layout = new QVBoxLayout(d->chat.widget);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    auto header = new QHBoxLayout;
    header->addWidget(d->chat.chats, 1);
    d->chat.renameChat = new QAction(d->chat.widget);
    d->chat.renameChat->setObjectName("aiRenameChat");
    d->chat.chatMenu = new IconButton(d->chat.widget);
    d->chat.chatMenu->setObjectName("aiChatMenu");
    d->chat.chatMenu->setFocusPolicy(Qt::StrongFocus);
    d->chat.chatMenu->installEventFilter(this);
    d->chat.chatMenu->setIcon(u8"\U000F01D9");
    header->addWidget(d->chat.chatMenu);
    layout->addLayout(header);
    layout->addWidget(d->chat.contextScope);
    layout->addWidget(d->chat.historyContainer, 1);
    layout->addWidget(d->chat.status);
    auto composer = new QHBoxLayout(d->chat.composer);
    composer->setContentsMargins(2, 0, 2, 0);
    composer->setSpacing(0);
    composer->addWidget(d->chat.commandsToggle, 0, Qt::AlignBottom);
    composer->addWidget(d->chat.input, 1);
    composer->addWidget(d->chat.send, 0, Qt::AlignBottom);
    layout->addWidget(d->chat.composer);
    connect(d->chat.chatMenu, &IconButton::clicked, this, [this] {
        auto menu = new ContextMenu(this);
        menu->setActions({ d->chat.newChat, d->chat.renameChat, d->chat.clearChat, d->chat.deleteChat, d->chat.insert });
        menu->setBackgroundColor(DesignSystem::color().primary());
        menu->setTextColor(DesignSystem::color().onPrimary());
        connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);
        menu->showContextMenu(d->chat.chatMenu->mapToGlobal(d->chat.chatMenu->rect().bottomLeft()));
    });

    auto session = AiChatSession::instance();
    connect(session, &AiChatSession::changed, this, &AiAssistantView::refreshChat);
    connect(d->chat.history, &ChatMessagesView::linkActivated, this, [session](const QUrl& url) {
        session->openSource(url);
    });
    connect(session, &AiChatSession::errorOccurred, this,
            [this](const QString& _error) { d->chat.input->setError(_error); });
    connect(d->chat.newChat, &QAction::triggered, session, [session] { session->createChat(); });
    connect(d->chat.renameChat, &QAction::triggered, this, [this, session] {
        const auto id = session->currentChat();
        bool accepted = false;
        const auto title = QInputDialog::getText(this, tr("Rename chat"), tr("Chat name"),
            QLineEdit::Normal, d->chat.chats->currentText(), &accepted);
        if (accepted && session->currentChat() == id && !session->renameChat(title)) {
            d->chat.input->setError(tr("Enter a chat name between 1 and 160 characters."));
        }
    });
    connect(d->chat.clearChat, &QAction::triggered, this, [this, session] {
        const auto chatId = session->currentChat();
        auto dialog = new Dialog(this);
        dialog->setObjectName("aiClearChatConfirmation");
        connect(dialog, &Dialog::finished, this,
                [this, session, dialog, chatId](const Dialog::ButtonInfo& button) {
            if (button.id == 1 && session->currentChat() == chatId) {
                cancelToolFlow(false);
                session->clearChat();
            }
            dialog->hideDialog();
            dialog->deleteLater();
        });
        dialog->showDialog(tr("Clear chat"), tr("Delete all messages in this chat? Its name will be kept."),
                           { { 0, tr("Cancel"), Dialog::RejectButton },
                             { 1, tr("Clear"), Dialog::AcceptCriticalButton } });
    });
    connect(d->chat.deleteChat, &QAction::triggered, this, [this, session] {
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
        if (d->flow.step == Implementation::ToolFlow::Arguments && !text.startsWith('/')) {
            prepareToolArguments(text);
            return;
        }
        const auto command = text.trimmed();
        if (command == "/synopsis" || command == "/generate" || command == "/translate"
            || command == "/character" || command == "/mindmap" || command == "/rephrase") {
            beginToolFlow(command);
            return;
        }
        if (d->flow.step != Implementation::ToolFlow::None) { cancelToolFlow(false); }
        if (d->availableWords > 0 && sendChatMessage(text)) {
            d->chat.input->clear();
            d->chat.drafts.remove(session->currentChat());
            d->chat.input->clearError();
        } else {
            d->chat.input->setText(text);
        }
    };
    connect(d->chat.send, &IconButton::clicked, this, send);
    connect(d->chat.input, &TextField::enterPressed, this, send);
    connect(d->chat.input, &TextField::textChanged, this, [this, session] {
        updateChatComposer();
    });

    connect(d->chat.insert, &QAction::triggered, this, [this] { emit insertTextRequested(d->chat.lastReply); });
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
    if ((_watched == d->chat.commandsToggle || _watched == d->chat.chatMenu)
        && _event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent*>(_event);
        if (key->key() == Qt::Key_Space || key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter) {
            if (_watched == d->chat.commandsToggle) {
                d->chat.commandsToggle->setChecked(!d->chat.commandsToggle->isChecked());
            } else {
                d->chat.chatMenu->click();
            }
            return true;
        }
    }
    if (_event->type() == QEvent::MouseButtonPress
        && (_watched == d->chat.input || _watched == d->chat.history->findChild<QTextBrowser*>("chatMessageText")->viewport())) {
        d->chat.commandsToggle->setChecked(false);
    }
    if (_watched == d->chat.historyContainer->viewport() && _event->type() == QEvent::Resize) {
        positionCommandMenu();
    }
    if (_watched == d->chat.input && _event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent*>(_event);
        if (key->key() == Qt::Key_Escape) {
            d->chat.commandsToggle->setChecked(false);
            cancelToolFlow(true);
            return true;
        }
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
    if (d->flow.step != Implementation::ToolFlow::None
        && (switchedChat || d->flow.documentId != session->documentId()
            || d->flow.documentName != session->documentName())) {
        const auto originalDraft = d->flow.originalDraft;
        d->flow = {};
        d->chat.input->setText(originalDraft);
        renderToolFlow();
    }
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
    d->chat.clearChat->setEnabled(!session->currentChat().isEmpty());
    d->chat.renameChat->setEnabled(!session->currentChat().isEmpty());
    const QSignalBlocker documentScopeBlocker(d->chat.documentScope);
    const QSignalBlocker projectScopeBlocker(d->chat.projectScope);
    const QSignalBlocker noDocumentScopeBlocker(d->chat.noDocumentScope);
    d->chat.documentScope->setChecked(session->contextScope() == AiChatSession::ContextScope::Document);
    d->chat.projectScope->setChecked(session->contextScope() == AiChatSession::ContextScope::Project);
    d->chat.noDocumentScope->setChecked(session->contextScope() == AiChatSession::ContextScope::None);
    d->chat.documentScope->setToolTip(session->documentName().isEmpty()
        ? tr("Document context cleared") : tr("Current document: %1").arg(session->documentName()));
    QVector<ChatMessage> messages;
    const User currentUser(tr("You"), QStringLiteral("ai-chat-user"));
    const User assistant(tr("Assistant"), QStringLiteral("ai-chat-assistant"));
    const User guide(tr("Assistant"), QStringLiteral("ai-chat-guide"));
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
    const bool hasConversation = std::any_of(messages.cbegin(), messages.cend(),
                                           [](const ChatMessage& message) { return !message.isSistemMessage(); });
    if (!hasConversation) {
        ChatMessage welcome({}, session->contextScope() == AiChatSession::ContextScope::None
            ? tr("Let's discuss your ideas. I can use our chat history and any text you send here; documents are not attached.")
            : session->contextScope() == AiChatSession::ContextScope::Project
            ? tr("Ask about any document or episode in the project. I will search the latest drafts and link to the sources.")
            : tr("I can help you develop ideas, work on the text, or discuss this document. Write a message or choose a task from the menu."), guide, false);
        messages.append(welcome);
    }
    if (d->flow.step != Implementation::ToolFlow::None) {
        QString request;
        for (const auto& command : d->chat.commandButtons) {
            if (command.first == d->flow.command) { request = command.second->text(); break; }
        }
        messages.append(ChatMessage({}, request, currentUser, false));
        if (d->flow.command == "/synopsis" && d->flow.step == Implementation::ToolFlow::Confirm) {
            messages.append(ChatMessage({}, tr("Choose the synopsis length — words per scene."), guide, false));
            messages.append(ChatMessage({}, d->flow.synopsisLength == -1 ? tr("No limit")
                : tr("%1 words per scene").arg(d->flow.synopsisLength), currentUser, false));
        }
    }
    d->chat.history->setCurrentUser(currentUser);
    d->chat.history->setMessages(messages);
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
    QString helper;
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
    if (d->chat.flowStart) {
        d->chat.flowStart->setEnabled(session->canSend() && d->availableWords > 0
            && (!d->isReadOnly || !toolWritesDocument(d->flow.command)));
    }
    if (d->chat.flowCost) {
        if (d->flow.command == "/synopsis") { d->chat.flowCost->setText(d->synopsisHint); }
        else if (d->flow.command == "/novel") { d->chat.flowCost->setText(d->novelHint); }
        else if (d->flow.command == "/script") { d->chat.flowCost->setText(d->scriptHint); }
        else if (d->flow.command == "/translate") { d->chat.flowCost->setText(d->translationHint); }
        d->chat.history->setFooterWidget(d->chat.flowCard);
    }
    for (const auto& entry : d->chat.commandButtons) {
        const auto commandName = entry.first;
        const bool available = commandName == "/help" ||
            ((commandName != "/synopsis" || d->synopsisAvailable)
             && (commandName != "/novel" || d->novelAvailable)
             && (commandName != "/script" || d->scriptAvailable)
             && (commandName != "/character" || d->generationViewType == GenerationViewType::CharacterInformation)
             && (commandName != "/mindmap" || d->generationViewType == GenerationViewType::MindMap)
             && (!toolWritesDocument(commandName) || (!d->isReadOnly && !session->documentId().isEmpty())));
        entry.second->setEnabled(available);
        const bool supported = (commandName != "/synopsis" || d->synopsisAvailable)
            && (commandName != "/novel" || d->novelAvailable)
            && (commandName != "/script" || d->scriptAvailable)
            && (commandName != "/character" || d->generationViewType == GenerationViewType::CharacterInformation)
            && (commandName != "/mindmap" || d->generationViewType == GenerationViewType::MindMap);
        entry.second->setVisible(supported);
    }
    positionCommandMenu();
    d->chat.send->setEnabled(text == "/help"
                            || (!text.isEmpty() && (d->flow.step == Implementation::ToolFlow::Arguments
                                || (d->availableWords > 0 && session->canSend()))));
    const auto status = !session->preparationStatus().isEmpty() ? session->preparationStatus()
        : session->hasPendingReply()
        ? (session->isWaitingForCurrentChat() ? tr("Waiting for the assistant…")
                                             : tr("The assistant is answering another chat…"))
        : !session->canSend() ? tr("The assistant is unavailable. You can still read your chats.")
        : d->availableWords <= 0 ? tr("Replenish credits to send a message.") : QString();
    d->chat.status->setText(status);
    d->chat.status->setVisible(!status.isEmpty());
}

void AiAssistantView::updateChatTranslations()
{
    d->chat.documentScope->setText(tr("Current document"));
    d->chat.documentScope->setAccessibleName(tr("Current document"));
    d->chat.projectScope->setText(tr("Entire project"));
    d->chat.projectScope->setAccessibleName(tr("Entire project"));
    d->chat.noDocumentScope->setText(tr("Chat only"));
    d->chat.noDocumentScope->setAccessibleName(tr("Chat only"));
    d->chat.noDocumentScope->setToolTip(tr("Use only the conversation, without attaching documents. Explicit commands still apply to the open document."));
    d->chat.projectScope->setToolTip(tr("Project mode searches all latest drafts and can read sources in several steps. Commands still apply to the open document."));
    d->chat.contextScope->setAccessibleName(tr("Conversation context"));
    d->chat.chats->setLabelVisible(false);
    d->chat.chats->setDefaultMarginsEnabled(false);
    d->chat.chats->setPlaceholderText(tr("New chat"));
    d->chat.newChat->setText(tr("New chat"));
    d->chat.deleteChat->setText(tr("Delete chat"));
    d->chat.clearChat->setText(tr("Clear chat"));
    d->chat.renameChat->setText(tr("Rename chat"));
    d->chat.chatMenu->setToolTip(tr("Manage chats"));
    d->chat.chatMenu->setAccessibleName(tr("Manage chats"));
    d->chat.commandsToggle->setToolTip(tr("Commands"));
    d->chat.commandsToggle->setAccessibleName(tr("Commands"));
    d->chat.input->setPlaceholderText(tr("Message"));
    d->chat.input->setToolTip(tr("Enter to send · Shift+Enter for a new line"));
    d->chat.send->setToolTip(tr("Send"));
    d->chat.send->setAccessibleName(tr("Send"));
    d->chat.insert->setText(tr("Insert last reply"));
    const QStringList descriptions{ tr("Rephrase"), tr("Translate"), tr("Expand"), tr("Shorten"),
        tr("Insert"), tr("Summarize"), tr("Generate"), tr("Synopsis"), tr("Novel"), tr("Script"),
        tr("Character"), tr("Mind map"), tr("Help") };
    for (int index = 0; index < d->chat.commandButtons.size(); ++index) {
        d->chat.commandButtons.at(index).second->setText(descriptions.at(index));
    }
    renderToolFlow();
}

void AiAssistantView::updateChatDesign()
{
    const auto background = DesignSystem::color().primary();
    const auto foreground = DesignSystem::color().onPrimary();
    const auto historyBackground = ColorHelper::isColorLight(background)
        ? background.lighter(106) : background.darker(116);
    const auto composerBackground = ColorHelper::flattenAlpha(
        ColorHelper::transparent(foreground, 0.05), historyBackground);
    d->chat.widget->setBackgroundColor(background);
    d->chat.chats->setBackgroundColor(background);
    d->chat.chats->setTextColor(foreground);
    d->chat.contextScope->setBackgroundColor(background);
    for (auto button : { d->chat.documentScope, d->chat.projectScope, d->chat.noDocumentScope }) {
        button->setBackgroundColor(background);
        button->setTextColor(foreground);
    }
    // The composer owns its rounded surface; the TextField fill blends into it.
    d->chat.input->setBackgroundColor(composerBackground);
    d->chat.input->setTextColor(foreground);
    d->chat.composer->setStyleSheet(QString("QFrame#aiChatComposer { background: %1; border-radius: 8px; }").arg(composerBackground.name()));
    d->chat.commands->setStyleSheet(QString(
        "QFrame#aiChatCommands { background: %1; border-radius: 8px; } "
        "QPushButton { color: %2; background: transparent; border: 1px solid %3; border-radius: 13px; padding: 5px 10px; } "
        "QPushButton:hover { background: %3; } QPushButton:disabled { color: %3; border-color: %3; }")
        .arg(background.name(), foreground.name(), ColorHelper::flattenAlpha(ColorHelper::transparent(foreground, 0.25), background).name()));
    d->chat.status->setBackgroundColor(background);
    d->chat.status->setTextColor(ColorHelper::transparent(foreground, 0.65));
    for (auto button : { d->chat.send, d->chat.commandsToggle, d->chat.chatMenu }) {
        button->setBackgroundColor(button == d->chat.chatMenu ? background : composerBackground);
        button->setTextColor(button == d->chat.send ? DesignSystem::color().accent() : foreground);
    }
    d->chat.history->setBackgroundColor(historyBackground);
    d->chat.history->setTextColor(foreground);
    auto palette = d->chat.historyContainer->palette();
    palette.setColor(QPalette::Base, historyBackground);
    palette.setColor(QPalette::Window, historyBackground);
    d->chat.historyContainer->setPalette(palette);
    d->chat.historyContainer->setStyleSheet("QScrollArea#aiChatHistoryContainer { border: 0; }");
    auto scrollBar = static_cast<ScrollBar*>(d->chat.historyContainer->verticalScrollBar());
    scrollBar->setBackgroundColor(historyBackground);
    scrollBar->setHandleColor(ColorHelper::transparent(foreground, DesignSystem::focusBackgroundOpacity()));
    renderToolFlow();
}

} // namespace Ui

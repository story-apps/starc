#include "ai_chat_view.h"

#include "ai_chat_session.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/combo_box/combo_box.h>
#include <ui/widgets/icon_button/icon_button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>

#include <QBoxLayout>
#include <QJsonObject>
#include <QKeyEvent>
#include <QSignalBlocker>
#include <QStandardItemModel>
#include <QTextBrowser>

namespace Ui {

class AiChatView::Implementation
{
public:
    ComboBox* chats = nullptr;
    QStandardItemModel* model = nullptr;
    IconButton* newChat = nullptr;
    IconButton* deleteChat = nullptr;
    Body2Label* context = nullptr;
    QTextBrowser* history = nullptr;
    ComboBox* commands = nullptr;
    TextField* input = nullptr;
    Button* send = nullptr;
    Button* insert = nullptr;
    Button* tools = nullptr;
    Body2Label* status = nullptr;
    int words = 0;
    bool insertionAvailable = true;
    QString lastReply;
    QString displayedChat;
    QHash<QString, QString> drafts;
};

AiChatView::AiChatView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    d->chats = new ComboBox(this);
    d->chats->setObjectName("aiChatSelector");
    d->model = new QStandardItemModel(this);
    d->chats->setModel(d->model);
    d->newChat = new IconButton(this);
    d->newChat->setObjectName("aiNewChat");
    d->newChat->setIcon(u8"\U000F0415");
    d->deleteChat = new IconButton(this);
    d->deleteChat->setObjectName("aiDeleteChat");
    d->deleteChat->setIcon(u8"\U000F01B4");
    d->context = new Body2Label(this);
    d->history = new QTextBrowser(this);
    d->history->setObjectName("aiChatHistory");
    d->history->setFrameShape(QFrame::NoFrame);
    d->history->setOpenLinks(false);
    d->history->setVerticalScrollBar(new ScrollBar);
    d->commands = new ComboBox(this);
    const QStringList commands{ "/rephrase", "/expand", "/shorten", "/insert", "/summarize",
                                "/translate", "/synopsis", "/novel", "/script", "/generate",
                                "/character", "/mindmap", "/help" };
    auto commandsModel = new QStandardItemModel(this);
    for (const auto& command : commands) {
        commandsModel->appendRow(new QStandardItem(command));
    }
    d->commands->setModel(commandsModel);
    d->commands->setCurrentIndex({});
    d->input = new TextField(this);
    d->input->setObjectName("aiChatInput");
    d->input->setEnterMakesNewLine(true);
    d->input->installEventFilter(this);
    d->input->setMinimumHeight(80);
    d->input->setMaximumHeight(160);
    d->send = new Button(this);
    d->send->setObjectName("aiChatSend");
    d->insert = new Button(this);
    d->insert->setObjectName("aiChatInsert");
    d->tools = new Button(this);
    d->tools->setObjectName("aiChatTools");
    d->status = new Body2Label(this);

    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);
    auto header = new QHBoxLayout;
    header->addWidget(d->chats, 1);
    header->addWidget(d->newChat);
    header->addWidget(d->deleteChat);
    layout->addLayout(header);
    layout->addWidget(d->context);
    layout->addWidget(d->history, 1);
    layout->addWidget(d->status);
    layout->addWidget(d->commands);
    layout->addWidget(d->input);
    auto actions = new QHBoxLayout;
    actions->addWidget(d->tools);
    actions->addStretch();
    actions->addWidget(d->send);
    layout->addLayout(actions);
    layout->addWidget(d->insert);

    auto session = AiChatSession::instance();
    connect(session, &AiChatSession::changed, this, &AiChatView::refresh);
    connect(session, &AiChatSession::errorOccurred, this,
            [this](const QString& _error) { d->input->setError(_error); });
    connect(d->newChat, &IconButton::clicked, session, [session] { session->createChat(); });
    connect(d->deleteChat, &IconButton::clicked, session, [session] { session->deleteChat(); });
    connect(d->chats, &ComboBox::currentIndexChanged, this, [this, session](const QModelIndex& _index) {
        if (_index.isValid()) {
            session->selectChat(_index.data(Qt::UserRole).toString());
        }
    });
    connect(d->commands, &ComboBox::currentIndexChanged, this, [this](const QModelIndex& _index) {
        if (!_index.isValid()) {
            return;
        }
        const auto command = _index.data().toString();
        d->input->setText(command + " ");
        d->input->setFocus();
        const QSignalBlocker blocker(d->commands);
        d->commands->setCurrentIndex({});
    });
    auto send = [this, session] {
        if (d->input->text().trimmed() == "/help") {
            d->input->setHelper(tr("Use /rephrase, /expand, /shorten, /insert, /summarize, /translate, "
                                  "/synopsis, /novel, /script, /generate, /character or /mindmap. "
                                  "Add instructions or text after the command. Without text, the current document is used."));
            return;
        }
        if (d->words > 0 && session->send(d->input->text())) {
            d->input->clear();
            d->drafts.remove(session->currentChat());
            d->input->clearError();
        }
    };
    connect(d->send, &Button::clicked, this, send);
    connect(d->input, &TextField::enterPressed, this, send);
    connect(d->input, &TextField::textChanged, this, [this, session] {
        d->send->setEnabled(d->words > 0 && session->canSend() && !d->input->text().trimmed().isEmpty());
    });
    connect(d->tools, &Button::clicked, this, &AiChatView::toolsRequested);
    connect(d->insert, &Button::clicked, this, [this] { emit insertRequested(d->lastReply); });
    updateTranslations();
    refresh();
}

AiChatView::~AiChatView() = default;

bool AiChatView::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->input && _event->type() == QEvent::KeyPress) {
        const auto key = static_cast<QKeyEvent*>(_event);
        if ((key->key() == Qt::Key_Return || key->key() == Qt::Key_Enter)
            && !(key->modifiers() & Qt::ShiftModifier)) {
            if (d->send->isEnabled()) {
                d->send->click();
            }
            return true;
        }
    }
    return Widget::eventFilter(_watched, _event);
}

void AiChatView::setAvailableWords(int _words)
{
    d->words = _words;
    refresh();
}

void AiChatView::setInsertionAvailable(bool _available)
{
    d->insertionAvailable = _available;
    refresh();
}

void AiChatView::refresh()
{
    auto session = AiChatSession::instance();
    if (d->displayedChat != session->currentChat()) {
        // Keep unsent drafts scoped to the conversation when switching between chats.
        if (!d->displayedChat.isEmpty()) {
            d->drafts.insert(d->displayedChat, d->input->text());
        }
        d->displayedChat = session->currentChat();
        d->input->setText(d->drafts.value(d->displayedChat));
    }
    const QSignalBlocker blocker(d->chats);
    d->model->clear();
    for (const auto& value : session->chats()) {
        const auto chat = value.toObject();
        auto item = new QStandardItem(chat.value("title").toString());
        item->setData(chat.value("id").toString(), Qt::UserRole);
        d->model->appendRow(item);
        if (chat.value("id").toString() == session->currentChat()) {
            d->chats->setCurrentIndex(item->index());
        }
    }
    if (session->currentChat().isEmpty()) {
        d->chats->setCurrentIndex({});
    }
    d->deleteChat->setEnabled(!session->currentChat().isEmpty());
    d->context->setText(session->documentName().isEmpty() ? tr("No document is open")
                                                        : tr("Context: %1").arg(session->documentName()));
    d->context->setToolTip(d->context->text());
    const auto messages = session->messages();
    QString html;
    d->lastReply.clear();
    for (const auto& value : messages) {
        const auto message = value.toObject();
        const auto role = message.value("role").toString();
        const auto text = message.value("text").toString();
        const auto label = role == "user" ? tr("You") : role == "assistant" ? tr("Assistant")
                                            : role == "error" ? tr("Error") : tr("Context");
        html += "<p><b>" + label.toHtmlEscaped() + "</b><br>"
            + text.toHtmlEscaped().replace("\n", "<br>") + "</p>";
        if (role == "assistant") {
            d->lastReply = text;
        }
    }
    if (messages.isEmpty()) {
        html = "<p>" + tr("Discuss the current document, ask a question, or choose a /command.").toHtmlEscaped() + "</p>";
    }
    const auto scroll = d->history->verticalScrollBar();
    const auto oldScroll = scroll->value();
    const auto atBottom = oldScroll >= scroll->maximum() - 4;
    d->history->setHtml(html);
    scroll->setValue(atBottom ? scroll->maximum() : oldScroll);
    d->send->setEnabled(d->words > 0 && session->canSend() && !d->input->text().trimmed().isEmpty());
    d->status->setText(session->hasPendingReply() ? tr("Waiting for the assistant…")
                          : !session->canSend() ? tr("The assistant is unavailable. You can still read your chats.") : QString());
    d->insert->setVisible(d->insertionAvailable && !d->lastReply.isEmpty());
}

void AiChatView::updateTranslations()
{
    d->chats->setLabel(tr("Chat"));
    d->chats->setPlaceholderText(tr("New chat"));
    d->newChat->setToolTip(tr("New chat"));
    d->deleteChat->setToolTip(tr("Delete chat"));
    d->commands->setLabel(tr("Commands"));
    d->commands->setPlaceholderText(tr("Choose a /command"));
    d->input->setLabel(tr("Message"));
    d->input->setPlaceholderText(tr("Ask about this document or type /help"));
    d->input->setHelper(tr("Enter to send · Shift+Enter for a new line"));
    d->send->setText(tr("Send"));
    d->insert->setText(tr("Insert last reply into the current document"));
    d->tools->setText(tr("Writing tools"));
}

void AiChatView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);
    const auto background = DesignSystem::color().primary();
    const auto foreground = DesignSystem::color().onPrimary();
    setBackgroundColor(background);
    for (auto field : { static_cast<TextField*>(d->chats), static_cast<TextField*>(d->commands), d->input }) {
        field->setBackgroundColor(background);
        field->setTextColor(foreground);
    }
    for (auto label : { d->context, d->status }) {
        label->setBackgroundColor(background);
        label->setTextColor(foreground);
    }
    for (auto button : { d->send, d->insert, d->tools }) {
        button->setBackgroundColor(ColorHelper::nearby(background));
        button->setTextColor(foreground);
    }
    for (auto button : { d->newChat, d->deleteChat }) {
        button->setBackgroundColor(background);
        button->setTextColor(foreground);
    }
    auto palette = d->history->palette();
    palette.setColor(QPalette::Base, background);
    palette.setColor(QPalette::Text, foreground);
    d->history->setPalette(palette);
}

} // namespace Ui

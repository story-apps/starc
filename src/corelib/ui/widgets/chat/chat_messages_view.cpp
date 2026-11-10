#include "chat_messages_view.h"
#include "chat_message.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QAbstractTextDocumentLayout>
#include <QResizeEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QtMath>

class ChatMessagesView::Implementation
{
public:
    // Chat text may contain Markdown image URLs. Rendering must never load local files
    // or make network requests on behalf of message authors.
    class MessageBrowser : public QTextBrowser
    {
    protected:
        QVariant loadResource(int, const QUrl&) override { return {}; }

        void mouseMoveEvent(QMouseEvent* _event) override
        {
            QTextBrowser::mouseMoveEvent(_event);
            const auto point = _event->pos() + QPoint(horizontalScrollBar()->value(),
                                                       verticalScrollBar()->value());
            const auto position = document()->documentLayout()->hitTest(point, Qt::ExactHit);
            const auto block = document()->findBlock(position);
            const bool overText = position >= 0 && block.isValid() && !block.text().isEmpty();
            viewport()->setCursor(overText ? Qt::IBeamCursor : Qt::ArrowCursor);
        }
    };
    class MessageDocument : public QTextDocument
    {
    protected:
        QVariant loadResource(int, const QUrl&) override { return {}; }
    };
    MessageBrowser* browser = nullptr;
    User currentUser;
    QVector<ChatMessage> messages;
    QVector<QPair<int, int>> ranges;
};

ChatMessagesView::ChatMessagesView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    d->browser = new Implementation::MessageBrowser;
    d->browser->setObjectName("chatMessageText");
    d->browser->setWordWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    d->browser->setFrameShape(QFrame::NoFrame);
    d->browser->setCursorWidth(0);
    d->browser->viewport()->setMouseTracking(true);
    d->browser->viewport()->setCursor(Qt::ArrowCursor);
    d->browser->setOpenLinks(false);
    d->browser->setOpenExternalLinks(false);
    d->browser->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
    d->browser->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->browser->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    d->browser->setContextMenuPolicy(Qt::CustomContextMenu);
    d->browser->setParent(this);
    auto policy = sizePolicy();
    policy.setHeightForWidth(true);
    setSizePolicy(policy);
    connect(d->browser, &QWidget::customContextMenuRequested, this, [this](const QPoint& _point) {
        const auto position = d->browser->cursorForPosition(_point).position();
        for (int index = 0; index < d->ranges.size(); ++index) {
            if (position >= d->ranges.at(index).first && position <= d->ranges.at(index).second) {
                emit messageContextMenuRequested(index);
                return;
            }
        }
    });
}

ChatMessagesView::~ChatMessagesView() = default;

void ChatMessagesView::setCurrentUser(const User& _user)
{
    if (d->currentUser == _user) {
        return;
    }
    d->currentUser = _user;
    rebuildDocument();
}

void ChatMessagesView::setMessages(const QVector<ChatMessage>& _messages)
{
    if (d->messages == _messages) {
        return;
    }
    d->messages = _messages;
    rebuildDocument();
}

QVector<ChatMessage> ChatMessagesView::messages() const
{
    return d->messages;
}

QString ChatMessagesView::selectedText() const
{
    return d->browser->textCursor().selectedText().replace(QChar::ParagraphSeparator, '\n');
}

void ChatMessagesView::rebuildDocument()
{
    auto document = d->browser->document();
    document->clear();
    document->setDefaultFont(Ui::DesignSystem::font().body2());
    document->setDocumentMargin(12);
    document->setDefaultStyleSheet("pre { white-space: pre-wrap; } code { font-family: monospace; } "
                                  "table { border-collapse: collapse; } td, th { padding: 4px; }");
    QTextCursor cursor(document);
    d->ranges.clear();
    QDate date;
    for (const auto& message : std::as_const(d->messages)) {
        // Root-level notices use paragraph margins for spacing, rather than frames
        // with empty separator paragraphs that would become part of the selection.
        if (!cursor.block().text().isEmpty()) {
            cursor.insertBlock(QTextBlockFormat(), QTextCharFormat());
        }
        if (message.dateTime().isValid() && date != message.dateTime().date()) {
            date = message.dateTime().date();
            QTextBlockFormat format;
            format.setAlignment(Qt::AlignCenter);
            format.setTopMargin(8);
            format.setBottomMargin(8);
            cursor.setBlockFormat(format);
            cursor.insertText(message.dateTime().toString("d MMMM"));
            cursor.insertBlock(QTextBlockFormat(), QTextCharFormat());
        }
        if (message.isSistemMessage()) {
            QTextBlockFormat format;
            format.setTopMargin(8);
            format.setBottomMargin(8);
            format.setLeftMargin(10);
            format.setRightMargin(10);
            cursor.setBlockFormat(format);
            QTextCharFormat textFormat;
            textFormat.setForeground(textColor());
            textFormat.setFontItalic(true);
            cursor.setCharFormat(textFormat);
            const int start = cursor.position();
            cursor.insertText(message.text());
            d->ranges.append({ start, cursor.position() });
            continue;
        }
        QTextFrameFormat frame;
        frame.setBorder(0);
        frame.setPadding(10);
        frame.setTopMargin(4);
        frame.setBottomMargin(8);
        const bool own = message.author() == d->currentUser;
        frame.setLeftMargin(own ? 36 : 0);
        frame.setRightMargin(own ? 0 : 16);
        if (!message.isSistemMessage()) {
            frame.setBackground(ColorHelper::nearby(backgroundColor()));
        }
        auto messageFrame = cursor.insertFrame(frame);
        cursor = messageFrame->firstCursorPosition();
        QTextBlockFormat normal;
        normal.setAlignment(Qt::AlignLeft);
        cursor.setBlockFormat(normal);
        QTextCharFormat textFormat;
        textFormat.setForeground(textColor());
        textFormat.setFontItalic(message.isSistemMessage());
        cursor.setCharFormat(textFormat);
        const int start = cursor.position();
        if (!message.isSistemMessage()) {
            auto authorFormat = textFormat;
            authorFormat.setFontWeight(QFont::DemiBold);
            cursor.insertText(message.author().name(), authorFormat);
            cursor.insertBlock(normal, textFormat);
        }
        if (message.isMarkdown()) {
            Implementation::MessageDocument fragment;
            fragment.setDefaultFont(document->defaultFont());
            fragment.setMarkdown(message.text(), QTextDocument::MarkdownFeatures(QTextDocument::MarkdownDialectGitHub)
                                                     | QTextDocument::MarkdownNoHTML);
            cursor.insertFragment(QTextDocumentFragment(&fragment));
        } else {
            cursor.insertText(message.text(), textFormat);
        }
        d->ranges.append({ start, cursor.position() });
        cursor = messageFrame->lastCursorPosition();
        cursor.movePosition(QTextCursor::NextCharacter);
        cursor.setBlockFormat(QTextBlockFormat());
        cursor.setCharFormat(QTextCharFormat());
    }
    auto palette = d->browser->palette();
    palette.setColor(QPalette::Base, backgroundColor());
    palette.setColor(QPalette::Text, textColor());
    d->browser->setPalette(palette);
    document->setTextWidth(qMax(1, width() - 4));
    updateGeometry();
}

int ChatMessagesView::heightForWidth(int _width) const
{
    d->browser->document()->setTextWidth(qMax(1, _width - 4));
    return qCeil(d->browser->document()->size().height()) + 4;
}

void ChatMessagesView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);
    d->browser->setGeometry(rect());
    d->browser->document()->setTextWidth(qMax(1, _event->size().width() - 4));
}

void ChatMessagesView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);
    rebuildDocument();
}

void ChatMessagesView::processBackgroundColorChange()
{
    rebuildDocument();
}

void ChatMessagesView::processTextColorChange()
{
    rebuildDocument();
}

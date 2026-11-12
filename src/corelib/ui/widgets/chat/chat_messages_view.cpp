#include "chat_messages_view.h"
#include "chat_message.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QAbstractTextDocumentLayout>
#include <QResizeEvent>
#include <QPainter>
#include <QLayout>
#include <QHelpEvent>
#include <QLocale>
#include <QMouseEvent>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextBrowser>
#include <QTextDocumentFragment>
#include <QTextFrame>
#include <QTextList>
#include <QToolTip>
#include <QtMath>

class ChatMessagesView::Implementation
{
public:
    // Chat text may contain Markdown image URLs. Rendering must never load local files
    // or make network requests on behalf of message authors.
    class MessageBrowser : public QTextBrowser
    {
    public:
        bool conversationStyle = false;
        QVector<QPair<QTextFrame*, QColor>> bubbles;

    protected:
        QVariant loadResource(int, const QUrl&) override { return {}; }

        void paintEvent(QPaintEvent* _event) override
        {
            if (!conversationStyle) {
                QTextBrowser::paintEvent(_event);
                return;
            }
            QPainter painter(viewport());
            painter.fillRect(viewport()->rect(), palette().brush(QPalette::Base));
            painter.translate(-horizontalScrollBar()->value(), -verticalScrollBar()->value());
            painter.setRenderHint(QPainter::Antialiasing);
            const auto layout = document()->documentLayout();
            for (const auto& bubble : bubbles) {
                painter.setPen(Qt::NoPen);
                painter.setBrush(bubble.second);
                const auto format = bubble.first->frameFormat();
                const auto rect = layout->frameBoundingRect(bubble.first).adjusted(
                    format.leftMargin(), format.topMargin(), -format.rightMargin(), -format.bottomMargin());
                painter.drawRoundedRect(rect, 8, 8);
            }
            QAbstractTextDocumentLayout::PaintContext context;
            context.palette = palette();
            context.clip = QRectF(horizontalScrollBar()->value(), verticalScrollBar()->value(),
                                  viewport()->width(), viewport()->height());
            // Paint transient link hover formatting without modifying message text
            // or the user's selection. The insertion caret remains omitted.
            for (const auto& extra : extraSelections()) {
                QAbstractTextDocumentLayout::Selection selection;
                selection.cursor = extra.cursor;
                selection.format = extra.format;
                context.selections.append(selection);
            }
            if (textCursor().hasSelection()) {
                QAbstractTextDocumentLayout::Selection selection;
                selection.cursor = textCursor();
                selection.format.setBackground(palette().brush(QPalette::Highlight));
                selection.format.setForeground(palette().brush(QPalette::HighlightedText));
                context.selections.append(selection);
            }
            layout->draw(&painter, context);
        }

        void mouseMoveEvent(QMouseEvent* _event) override
        {
            QTextBrowser::mouseMoveEvent(_event);
            const auto link = linkCursorAt(_event->pos());
            QList<QTextEdit::ExtraSelection> hover;
            if (!link.isNull()) {
                QTextEdit::ExtraSelection selection;
                selection.cursor = link;
                selection.format.setFontUnderline(true);
                hover.append(selection);
            }
            setExtraSelections(hover);
            const auto point = _event->pos() + QPoint(horizontalScrollBar()->value(),
                                                       verticalScrollBar()->value());
            const auto position = document()->documentLayout()->hitTest(point, Qt::ExactHit);
            const auto block = document()->findBlock(position);
            const bool overText = position >= 0 && block.isValid() && !block.text().isEmpty();
            viewport()->setCursor(!anchorAt(_event->pos()).isEmpty()
                ? Qt::PointingHandCursor : overText ? Qt::IBeamCursor : Qt::ArrowCursor);
        }

        bool viewportEvent(QEvent* event) override
        {
            if (event->type() == QEvent::Leave) {
                setExtraSelections({});
                QToolTip::hideText();
            } else if (event->type() == QEvent::ToolTip) {
                const auto help = static_cast<QHelpEvent*>(event);
                const auto link = linkCursorAt(help->pos());
                if (!link.isNull()) {
                    const auto format = link.charFormat();
                    const auto title = format.toolTip().isEmpty() ? format.anchorHref() : format.toolTip();
                    QToolTip::showText(help->globalPos(), "<qt>" + title.toHtmlEscaped() + "</qt>", viewport());
                } else { QToolTip::hideText(); }
                return true;
            }
            return QTextBrowser::viewportEvent(event);
        }

        QTextCursor linkCursorAt(const QPoint& point) const
        {
            const auto href = anchorAt(point);
            if (href.isEmpty()) { return {}; }
            const auto cursor = cursorForPosition(point);
            int start = -1, end = -1;
            for (auto it = cursor.block().begin(); !it.atEnd(); ++it) {
                const auto fragment = it.fragment();
                if (fragment.charFormat().anchorHref() == href) {
                    if (start < 0) { start = fragment.position(); }
                    end = fragment.position() + fragment.length();
                } else if (start >= 0) {
                    if (cursor.position() >= start && cursor.position() <= end) { break; }
                    start = end = -1;
                }
            }
            if (start < 0 || cursor.position() < start || cursor.position() > end) { return {}; }
            auto link = cursor;
            link.setPosition(start);
            link.setPosition(end, QTextCursor::KeepAnchor);
            return link;
        }
    };
    class MessageDocument : public QTextDocument
    {
    protected:
        QVariant loadResource(int, const QUrl&) override { return {}; }
    };
    MessageBrowser* browser = nullptr;
    QWidget* footer = nullptr;
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
    connect(d->browser, &QTextBrowser::anchorClicked, this, &ChatMessagesView::linkActivated);
    d->browser->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard
                                         | Qt::LinksAccessibleByMouse | Qt::LinksAccessibleByKeyboard);
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

void ChatMessagesView::setConversationStyle(bool _enabled)
{
    if (d->browser->conversationStyle == _enabled) {
        return;
    }
    d->browser->conversationStyle = _enabled;
    rebuildDocument();
}

void ChatMessagesView::setFooterWidget(QWidget* _widget)
{
    if (d->footer != _widget) {
        if (d->footer) {
            d->footer->hide();
        }
        d->footer = _widget;
        if (_widget) {
            _widget->setParent(this);
        }
    }
    updateFooterGeometry();
    updateGeometry();
}

void ChatMessagesView::updateFooterGeometry()
{
    if (!d->footer || d->footer->isHidden()) {
        return;
    }
    const int footerWidth = qMax(1, width() - 24);
    const auto layout = d->footer->layout();
    const int footerHeight = layout && layout->hasHeightForWidth()
        ? layout->heightForWidth(footerWidth) : d->footer->sizeHint().height();
    d->footer->setGeometry(12, qCeil(d->browser->document()->size().height()) + 8,
                           footerWidth, footerHeight);
    d->footer->raise();
}

void ChatMessagesView::rebuildDocument()
{
    auto document = d->browser->document();
    d->browser->bubbles.clear();
    d->browser->setExtraSelections({});
    document->clear();
    document->setDefaultFont(Ui::DesignSystem::font().body2());
    document->setDocumentMargin(12);
    document->setIndentWidth(20);
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
            cursor.insertText(QLocale().toString(message.dateTime().date(), "d MMMM"));
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
        const auto bubbleColor = d->browser->conversationStyle && own
            ? Ui::DesignSystem::color().accent() : ColorHelper::nearby(backgroundColor());
        if (!d->browser->conversationStyle) {
            frame.setBackground(bubbleColor);
        }
        auto messageFrame = cursor.insertFrame(frame);
        d->browser->bubbles.append({ messageFrame, bubbleColor });
        cursor = messageFrame->firstCursorPosition();
        QTextBlockFormat normal;
        normal.setAlignment(Qt::AlignLeft);
        cursor.setBlockFormat(normal);
        QTextCharFormat textFormat;
        textFormat.setForeground(d->browser->conversationStyle && own
                                     ? Ui::DesignSystem::color().onAccent() : textColor());
        cursor.setCharFormat(textFormat);
        const int start = cursor.position();
        if (!d->browser->conversationStyle) {
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
            QVector<QTextCursor> links;
            for (auto block = fragment.begin(); block.isValid(); block = block.next()) {
                if (block.textList()) {
                    auto format = block.blockFormat();
                    format.setTopMargin(4);
                    format.setBottomMargin(4);
                    format.setLeftMargin(0);
                    format.setRightMargin(0);
                    format.setIndent(0);
                    QTextCursor(block).setBlockFormat(format);
                }
                for (auto it = block.begin(); !it.atEnd(); ++it) {
                    const auto part = it.fragment();
                    if (part.charFormat().isAnchor()) {
                        QTextCursor link(&fragment);
                        link.setPosition(part.position());
                        link.setPosition(part.position() + part.length(), QTextCursor::KeepAnchor);
                        links.append(link);
                    }
                }
            }
            QTextCharFormat linkStyle;
            linkStyle.setForeground(Ui::DesignSystem::color().accent());
            linkStyle.setFontUnderline(false);
            for (auto& link : links) { link.mergeCharFormat(linkStyle); }
            cursor.insertFragment(QTextDocumentFragment(&fragment));
        } else {
            cursor.insertText(message.text(), textFormat);
        }
        if (d->browser->conversationStyle && message.dateTime().isValid()) {
            QTextBlockFormat timestamp;
            timestamp.setAlignment(Qt::AlignRight);
            timestamp.setTopMargin(4);
            QTextCharFormat timeFormat;
            timeFormat.setFont(Ui::DesignSystem::font().caption());
            timeFormat.setForeground(ColorHelper::transparent(textFormat.foreground().color(), 0.55));
            cursor.insertBlock(timestamp, timeFormat);
            cursor.insertText(message.dateTime().toString("HH:mm"));
        }
        d->ranges.append({ start, cursor.position() });
        cursor = messageFrame->lastCursorPosition();
        cursor.movePosition(QTextCursor::NextCharacter);
        QTextBlockFormat separator;
        if (d->browser->conversationStyle) {
            separator.setLineHeight(1, QTextBlockFormat::FixedHeight);
        }
        cursor.setBlockFormat(separator);
        cursor.setCharFormat(QTextCharFormat());
    }
    auto palette = d->browser->palette();
    palette.setColor(QPalette::Base, backgroundColor());
    palette.setColor(QPalette::Text, textColor());
    palette.setColor(QPalette::Link, Ui::DesignSystem::color().accent());
    palette.setColor(QPalette::LinkVisited, Ui::DesignSystem::color().accent());
    d->browser->setPalette(palette);
    document->setTextWidth(qMax(1, width() - 4));
    updateFooterGeometry();
    updateGeometry();
    d->browser->viewport()->update();
}

int ChatMessagesView::heightForWidth(int _width) const
{
    d->browser->document()->setTextWidth(qMax(1, _width - 4));
    int footerHeight = 0;
    if (d->footer && !d->footer->isHidden()) {
        const auto layout = d->footer->layout();
        footerHeight = 16 + (layout && layout->hasHeightForWidth()
            ? layout->heightForWidth(qMax(1, _width - 24)) : d->footer->sizeHint().height());
    }
    return qCeil(d->browser->document()->size().height()) + 4 + footerHeight;
}

void ChatMessagesView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);
    d->browser->setGeometry(rect());
    d->browser->document()->setTextWidth(qMax(1, _event->size().width() - 4));
    updateFooterGeometry();
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

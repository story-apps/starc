#include "chat_messages_view.h"

#include "chat_message.h"

#include <ui/design_system/design_system.h>
#include <ui/modules/avatar_generator/avatar_generator.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
#include <QClipboard>
#include <QLabel>
#include <QMenu>
#include <QPaintEvent>
#include <QPainter>
#include <QResizeEvent>


class ChatMessagesView::Implementation
{
public:
    struct AssistantMessageLayout {
        QRectF bubbleRect;
        QRectF roleRect;
        QRectF textRect;
        bool isUser = false;
    };

    QVector<AssistantMessageLayout> assistantMessageLayouts(int _width) const;

    User currectUser;
    QVector<ChatMessage> messages;
    QVector<QLabel*> assistantTextLabels;
    bool assistantStyle = false;
};

QVector<ChatMessagesView::Implementation::AssistantMessageLayout>
ChatMessagesView::Implementation::assistantMessageLayouts(int _width) const
{
    QVector<AssistantMessageLayout> result;
    result.reserve(messages.size());

    const auto& layout = Ui::DesignSystem::layout();
    const auto textFont = Ui::DesignSystem::font().body2();
    const auto roleFont = Ui::DesignSystem::font().caption();
    qreal y = layout.px8();
    ChatMessage previousMessage;
    for (const auto& message : messages) {
        const bool isUser = message.author() == currectUser;
        const bool isAuthorChanged = previousMessage.author() != message.author();
        const qreal widthRatio = isUser ? 0.76 : 0.84;
        const qreal maximumBubbleWidth = std::max<qreal>(
            layout.px62(), std::min(_width - layout.px24(), _width * widthRatio));
        const qreal maximumTextWidth = maximumBubbleWidth - layout.px24();
        const auto naturalTextWidth
            = TextHelper::fineTextWidthF(message.text(), textFont) + layout.px2();
        const auto textWidth
            = std::max<qreal>(layout.px48(), std::min(maximumTextWidth, naturalTextWidth));
        const auto textHeight = TextHelper::heightForWidth(message.text(), textFont, textWidth);
        const auto bubbleWidth = textWidth + layout.px24();
        const auto bubbleHeight = textHeight + layout.px16() + layout.px4();
        const auto x = isUser ? _width - layout.px12() - bubbleWidth : layout.px12();

        y += isAuthorChanged ? layout.px12() : layout.px4();
        QRectF roleRect;
        if (!isUser && isAuthorChanged) {
            const auto roleHeight = TextHelper::fineLineSpacing(roleFont);
            roleRect = QRectF(x, y, bubbleWidth, roleHeight);
            y += roleHeight + layout.px4();
        }

        const QRectF bubbleRect(x, y, bubbleWidth, bubbleHeight);
        const auto textRect = bubbleRect.adjusted(layout.px12(), layout.px8() + layout.px2(),
                                                  -layout.px12(), -layout.px8() - layout.px2());
        result.append({ bubbleRect, roleRect, textRect, isUser });
        y = bubbleRect.bottom();
        previousMessage = message;
    }
    return result;
}


// ****


ChatMessagesView::ChatMessagesView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFocusPolicy(Qt::StrongFocus);
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);

    connect(
        this, &ChatMessagesView::customContextMenuRequested, this, [this](const QPoint& _position) {
            if (d->assistantStyle) {
                return;
            }

            const auto titleFont = Ui::DesignSystem::font().subtitle2();
            const auto titleFontLineSpacing = TextHelper::fineLineSpacing(titleFont);
            const auto textFont = Ui::DesignSystem::font().body2();
            const auto textFontLineSpacing = TextHelper::fineLineSpacing(textFont);
            const qreal maximumTextWidth = width()
                - Ui::DesignSystem::layout().px48() // отступ слева под авку и между авкой и текстом
                - Ui::DesignSystem::layout().px24() // марджины текста от балуна
                - Ui::DesignSystem::layout().px16(); // отступ справа от границы
            qreal lastY = Ui::DesignSystem::layout().px16();
            bool isDateChanged = false;
            bool isAuthorChanged = false;
            bool isCurrentAuthor = false;
            ChatMessage lastMessage;
            for (int index = 0; index < d->messages.size(); ++index) {
                const auto& message = d->messages[index];
                isDateChanged = lastMessage.dateTime().date() != message.dateTime().date();
                isAuthorChanged = lastMessage.author() != message.author();
                isCurrentAuthor = message.author() == d->currectUser;

                if (isDateChanged) {
                    lastY += Ui::DesignSystem::layout().px16() + textFontLineSpacing;
                }

                const qreal messageTextWidth
                    = std::max(std::min(maximumTextWidth,
                                        TextHelper::fineTextWidthF(message.text(), textFont)),
                               TextHelper::fineTextWidthF(
                                   isCurrentAuthor ? "" : message.author().name(), titleFont));
                const qreal messageTextHeight = TextHelper::heightForWidth(
                    message.text(), Ui::DesignSystem::font().body2(), messageTextWidth);
                const qreal messageTopDelta = isDateChanged
                    ? Ui::DesignSystem::layout().px16()
                    : (isAuthorChanged ? Ui::DesignSystem::layout().px8()
                                       : Ui::DesignSystem::layout().px2());
                const qreal messageHeightDelta = isAuthorChanged && !isCurrentAuthor
                    ? titleFontLineSpacing + Ui::DesignSystem::layout().px4()
                    : 0.0;

                if (lastY <= _position.y()
                    && _position.y()
                        <= (lastY + messageTopDelta + messageTextHeight + messageHeightDelta
                            + Ui::DesignSystem::layout().px24())) {
                    emit messageContextMenuRequested(index);
                    return;
                }

                lastY += messageTopDelta + messageTextHeight + messageHeightDelta
                    + Ui::DesignSystem::layout().px24();

                lastMessage = message;
            }
        });
}

ChatMessagesView::~ChatMessagesView() = default;

void ChatMessagesView::setCurrentUser(const User& _user)
{
    d->currectUser = _user;
    updateAssistantTextLabels();
    updateGeometry();
    update();
}

void ChatMessagesView::setMessages(const QVector<ChatMessage>& _messages)
{
    d->messages = _messages;
    updateAssistantTextLabels();
    updateGeometry();
    update();
}

void ChatMessagesView::setAssistantStyle(bool _enabled)
{
    if (d->assistantStyle == _enabled) {
        return;
    }
    d->assistantStyle = _enabled;
    updateAssistantTextLabels();
    updateGeometry();
    update();
}

void ChatMessagesView::updateAssistantTextLabels()
{
    while (d->assistantTextLabels.size() < d->messages.size()) {
        auto label = new QLabel(this);
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        label->setAlignment(Qt::AlignLeft | Qt::AlignTop);
        label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard);
        label->setFocusPolicy(Qt::ClickFocus);
        label->setCursor(Qt::IBeamCursor);
        label->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(label, &QLabel::customContextMenuRequested, label, [label](const QPoint& _position) {
            QMenu menu(label);
            auto copySelectionAction = menu.addAction(ChatMessagesView::tr("Copy selection"));
            copySelectionAction->setEnabled(label->hasSelectedText());
            if (menu.exec(label->mapToGlobal(_position)) == copySelectionAction
                && label->hasSelectedText()) {
                QApplication::clipboard()->setText(label->selectedText());
            }
        });
        d->assistantTextLabels.append(label);
    }

    const auto messageLayouts = d->assistantMessageLayouts(width());
    const auto textFont = Ui::DesignSystem::font().body2();
    const auto userTextColor = Ui::DesignSystem::color().onAccent();
    for (int index = 0; index < d->assistantTextLabels.size(); ++index) {
        auto label = d->assistantTextLabels.at(index);
        const bool isVisible = d->assistantStyle && index < d->messages.size();
        label->setVisible(isVisible);
        if (!isVisible) {
            continue;
        }

        const auto& message = d->messages.at(index);
        const auto& messageLayout = messageLayouts.at(index);
        if (label->text() != message.text()) {
            label->setText(message.text());
        }
        label->setFont(textFont);
        label->setGeometry(messageLayout.textRect.toAlignedRect());
        auto palette = label->palette();
        palette.setColor(QPalette::WindowText,
                         messageLayout.isUser ? userTextColor : textColor());
        label->setPalette(palette);
        label->setAutoFillBackground(false);
        label->raise();
    }
}

int ChatMessagesView::heightForWidth(int _width) const
{
    if (d->assistantStyle) {
        const auto layouts = d->assistantMessageLayouts(_width);
        if (layouts.isEmpty()) {
            return Ui::DesignSystem::layout().px24();
        }
        return static_cast<int>(layouts.constLast().bubbleRect.bottom()
                                + Ui::DesignSystem::layout().px12());
    }

    //
    // Пересчитываем высоту в зависимости от ширины
    // NOTE: тут многое повторяется с методом отрисовки, при изменении быть внимательным
    //

    const auto titleFont = Ui::DesignSystem::font().subtitle2();
    const auto titleFontLineSpacing = TextHelper::fineLineSpacing(titleFont);
    const auto textFont = Ui::DesignSystem::font().body2();
    const auto textFontLineSpacing = TextHelper::fineLineSpacing(textFont);
    const qreal maximumTextWidth = _width
        - Ui::DesignSystem::layout().px48() // отступ слева под авку и между авкой и текстом
        - Ui::DesignSystem::layout().px24() // марджины текста от балуна
        - Ui::DesignSystem::layout().px16(); // отступ справа от границы
    qreal lastY = Ui::DesignSystem::layout().px16();
    bool isDateChanged = false;
    bool isAuthorChanged = false;
    bool isCurrentAuthor = false;
    ChatMessage lastMessage;
    for (const auto& message : std::as_const(d->messages)) {
        isDateChanged = lastMessage.dateTime().date() != message.dateTime().date();
        isAuthorChanged = lastMessage.author() != message.author();
        isCurrentAuthor = message.author() == d->currectUser;

        if (isDateChanged) {
            lastY += Ui::DesignSystem::layout().px16() + textFontLineSpacing;
        }

        const qreal messageTextWidth = std::max(
            std::min(maximumTextWidth, TextHelper::fineTextWidthF(message.text(), textFont)),
            TextHelper::fineTextWidthF(isCurrentAuthor ? "" : message.author().name(), titleFont));
        const qreal messageTextHeight = TextHelper::heightForWidth(
            message.text(), Ui::DesignSystem::font().body2(), messageTextWidth);
        const qreal messageTopDelta = isDateChanged
            ? Ui::DesignSystem::layout().px16()
            : (isAuthorChanged ? Ui::DesignSystem::layout().px8()
                               : Ui::DesignSystem::layout().px2());
        const qreal messageHeightDelta = isAuthorChanged && !isCurrentAuthor
            ? titleFontLineSpacing + Ui::DesignSystem::layout().px4()
            : 0.0;
        lastY += messageTopDelta + messageTextHeight + messageHeightDelta
            + Ui::DesignSystem::layout().px24();

        lastMessage = message;
    }

    return lastY + Ui::DesignSystem::layout().px16();
}

void ChatMessagesView::paintEvent(QPaintEvent* _event)
{
    if (d->assistantStyle) {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(_event->rect(), backgroundColor());

        const auto& layout = Ui::DesignSystem::layout();
        const auto roleFont = Ui::DesignSystem::font().caption();
        const auto accent = Ui::DesignSystem::color().accent();
        const auto assistantBubbleColor = ColorHelper::nearby(backgroundColor());
        const auto assistantOutlineColor = ColorHelper::transparent(textColor(), 0.10);
        ChatMessage previousMessage;

        const auto messageLayouts = d->assistantMessageLayouts(width());
        for (int messageIndex = 0; messageIndex < d->messages.size(); ++messageIndex) {
            const auto& message = d->messages.at(messageIndex);
            const auto& messageLayout = messageLayouts.at(messageIndex);
            const bool isUser = messageLayout.isUser;
            const bool isAuthorChanged = previousMessage.author() != message.author();

            if (!isUser && isAuthorChanged) {
                const auto dotSize = layout.px8();
                const QRectF dotRect(
                    messageLayout.roleRect.left() + layout.px4(),
                    messageLayout.roleRect.top()
                        + (messageLayout.roleRect.height() - dotSize) / 2.0,
                    dotSize, dotSize);
                painter.setPen(Qt::NoPen);
                painter.setBrush(accent);
                painter.drawEllipse(dotRect);

                painter.setFont(roleFont);
                painter.setPen(ColorHelper::transparent(textColor(), 0.72));
                const QRectF roleRect(dotRect.right() + layout.px8(),
                                      messageLayout.roleRect.top(),
                                      messageLayout.roleRect.width() - layout.px16(),
                                      messageLayout.roleRect.height());
                painter.drawText(roleRect, Qt::AlignLeft | Qt::AlignVCenter, tr("Codex"));
            }

            const auto bubbleRect = messageLayout.bubbleRect;
            painter.setPen(isUser ? QPen(Qt::NoPen) : QPen(assistantOutlineColor, 1.0));
            painter.setBrush(isUser ? accent : assistantBubbleColor);
            painter.drawRoundedRect(bubbleRect, layout.px16(), layout.px16());
            previousMessage = message;
        }
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setFont(Ui::DesignSystem::font().body2());

    //
    // Рисуем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем сообщения
    //

    const auto titleFont = Ui::DesignSystem::font().subtitle2();
    const auto titleFontLineSpacing = TextHelper::fineLineSpacing(titleFont);
    const auto textFont = Ui::DesignSystem::font().body2();
    const auto textFontLineSpacing = TextHelper::fineLineSpacing(textFont);
    const QColor defaultBaloonColor = ColorHelper::nearby(backgroundColor());
    const QColor currentAuthorBaloonColor = ColorHelper::nearby(defaultBaloonColor);
    const qreal maximumTextWidth = width()
        - Ui::DesignSystem::layout().px48() // отступ слева под авку и между авкой и текстом
        - Ui::DesignSystem::layout().px24() // марджины текста от балуна
        - Ui::DesignSystem::layout().px16(); // отступ справа от границы
    qreal lastY = 0.0;
    bool isDateChanged = true;
    bool isAuthorChanged = true;
    bool isCurrentAuthor = false;
    ChatMessage lastMessage;
    auto drawAvatar = [this, &painter, &lastY, &isDateChanged, &isAuthorChanged, &isCurrentAuthor,
                       &lastMessage] {
        if (!lastMessage.author().isValid()) {
            return;
        }

        //
        // Если предыдущее сообщение было не своё, и при этом изменяется пользователь, или
        // день, то отрисуем авку
        //
        if (!isCurrentAuthor && (isDateChanged || isAuthorChanged)) {
            const auto avatarSize = Ui::DesignSystem::treeOneLineItem().iconSize();
            const QRectF avatarRect(
                QPointF(isLeftToRight()
                            ? Ui::DesignSystem::layout().px12()
                            : (width() - Ui::DesignSystem::layout().px12() - avatarSize.width()),
                        lastY - avatarSize.height()),
                avatarSize);
            const auto avatar = Ui::AvatarGenerator::avatar(lastMessage.author().name(),
                                                            lastMessage.author().email());
            painter.drawPixmap(avatarRect, avatar, avatar.rect());
        }
    };

    for (const auto& message : std::as_const(d->messages)) {
        //
        // Определим изменилась ли дата
        //
        isDateChanged = lastMessage.dateTime().date() != message.dateTime().date();

        //
        // Определим изменился ли пользователь
        //
        isAuthorChanged = lastMessage.author() != message.author();

        //
        // Рисуем авку
        //
        drawAvatar();

        //
        // Определим, принадлежит ли текущее сообщение пользователю программы
        //
        isCurrentAuthor = message.author() == d->currectUser;


        //
        // Если дата изменилась, отрисуем новую дату
        //
        if (isDateChanged) {
            painter.setPen(textColor());
            const QRectF dateRect(0.0, lastY + Ui::DesignSystem::layout().px16(), width(),
                                  textFontLineSpacing);
            painter.drawText(dateRect, Qt::AlignCenter, message.dateTime().toString("d MMMM"));

            lastY = dateRect.bottom();
        }


        //
        // Определим область текста
        //
        const qreal messageTextWidth = std::max(
            std::min(maximumTextWidth, TextHelper::fineTextWidthF(message.text(), textFont)),
            TextHelper::fineTextWidthF(isCurrentAuthor ? "" : message.author().name(), titleFont));
        const qreal messageTextHeight = TextHelper::heightForWidth(
            message.text(), Ui::DesignSystem::font().body2(), messageTextWidth);
        //
        // Определим область балуна под текст
        //
        const qreal messageTopDelta = isDateChanged
            ? Ui::DesignSystem::layout().px16()
            : (isAuthorChanged ? Ui::DesignSystem::layout().px8()
                               : Ui::DesignSystem::layout().px2());
        const qreal messageWidth = messageTextWidth + Ui::DesignSystem::layout().px24();
        const qreal messageX = isCurrentAuthor && messageTextWidth < maximumTextWidth
            ? (isLeftToRight() ? (width() - messageWidth - Ui::DesignSystem::layout().px16())
                               : Ui::DesignSystem::layout().px16())
            : (isLeftToRight() ? Ui::DesignSystem::layout().px48()
                               : (width() - messageWidth - Ui::DesignSystem::layout().px48()));
        const qreal messageHeightDelta = isAuthorChanged && !isCurrentAuthor
            ? titleFontLineSpacing + Ui::DesignSystem::layout().px4()
            : 0.0;
        const QRectF messageRect(messageX, lastY + messageTopDelta, messageWidth,
                                 messageTextHeight + messageHeightDelta
                                     + Ui::DesignSystem::layout().px24());
        const QRectF messageTextRect = messageRect.adjusted(
            Ui::DesignSystem::layout().px12(),
            Ui::DesignSystem::layout().px12() + messageHeightDelta,
            -Ui::DesignSystem::layout().px12(), -Ui::DesignSystem::layout().px12());
        painter.setPen(Qt::NoPen);
        painter.setBrush(isCurrentAuthor ? currentAuthorBaloonColor : defaultBaloonColor);
        painter.drawRoundedRect(messageRect, Ui::DesignSystem::card().borderRadius(),
                                Ui::DesignSystem::card().borderRadius());
        painter.setPen(textColor());
        //
        QTextOption textOption;
        textOption.setAlignment(isLeftToRight() ? Qt::AlignLeft : Qt::AlignRight);
        if (isAuthorChanged && !isCurrentAuthor) {
            painter.setFont(Ui::DesignSystem::font().subtitle2());
            painter.setPen(message.author().avatarColor());
            painter.drawText(QRectF(QPointF(messageTextRect.left(),
                                            messageRect.top() + Ui::DesignSystem::layout().px8()),
                                    QSizeF(messageTextRect.width(), titleFontLineSpacing)),
                             message.author().name(), textOption);
            painter.setFont(Ui::DesignSystem::font().body2());
            painter.setPen(textColor());
        }
        //
        textOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        painter.drawText(messageTextRect, message.text(), textOption);

        lastY = messageRect.bottom();

        //
        // Запомним текущее обработанное сообщение
        //
        lastMessage = message;
    }

    //
    // Рисуем авку после того, как нарисовали последнее сообщение
    //
    isAuthorChanged = true;
    drawAvatar();
}

void ChatMessagesView::resizeEvent(QResizeEvent* _event)
{
    Widget::resizeEvent(_event);
    updateAssistantTextLabels();
}

void ChatMessagesView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);
    updateAssistantTextLabels();
}

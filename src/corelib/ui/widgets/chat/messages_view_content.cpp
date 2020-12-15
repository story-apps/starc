#include "messages_view_content.h"

#include "message.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPainter>
#include <QPaintEvent>


class MessagesViewContent::Implementation
{
public:
    User currectUser;
    QVector<Message> messages;
};


// ****


MessagesViewContent::MessagesViewContent(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
}

MessagesViewContent::~MessagesViewContent() = default;

void MessagesViewContent::setCurrectUser(const User& _user)
{
    d->currectUser = _user;
    updateGeometry();
    update();
}

void MessagesViewContent::setMessages(const QVector<Message>& _messages)
{
    d->messages = _messages;
    updateGeometry();
    update();
}

int MessagesViewContent::heightForWidth(int _width) const
{
    //
    // Пересчитываем высоту в зависимости от ширины
    // NOTE: тут многое повторяется с методом отрисовки, при изменении быть внимательным
    //

    const QFontMetricsF titleFontMetrics(Ui::DesignSystem::font().subtitle2());
    const QFontMetricsF textFontMetrics(Ui::DesignSystem::font().body2());
    const qreal maximumTextWidth = _width
                                   - Ui::DesignSystem::layout().px48() // отступ слева под авку и между авкой и текстом
                                   - Ui::DesignSystem::layout().px24() // марджины текста от балуна
                                   - Ui::DesignSystem::layout().px16(); // отступ справа от границы
    qreal lastY = Ui::DesignSystem::layout().px16();
    bool isDateChanged = false;
    bool isAuthorChanged = false;
    bool isCurrentAuthor = false;
    Message lastMessage;

    for (const auto& message : d->messages) {
        isDateChanged = lastMessage.dateTime().date() != message.dateTime().date();
        isAuthorChanged = lastMessage.author() != message.author();
        isCurrentAuthor = message.author() == d->currectUser;

        if (isDateChanged) {
            lastY += Ui::DesignSystem::layout().px16() + textFontMetrics.lineSpacing();
        }

        const qreal messageTextWidth = std::max(std::min(maximumTextWidth,
                                                         textFontMetrics.width(message.text())),
                                                TextHelper::fineTextWidth(isCurrentAuthor
                                                                          ? ""
                                                                          : message.author().name(),
                                                                          titleFontMetrics));
        const qreal messageTextHeight = TextHelper::heightForWidth(message.text(), Ui::DesignSystem::font().body2(), messageTextWidth);
        const qreal messageTopDelta = isDateChanged ? Ui::DesignSystem::layout().px16()
                                                    : isAuthorChanged
                                                      ? Ui::DesignSystem::layout().px8()
                                                      : Ui::DesignSystem::layout().px2();
        const qreal messageHeightDelta = isAuthorChanged && !isCurrentAuthor
                                         ? titleFontMetrics.lineSpacing() + Ui::DesignSystem::layout().px4()
                                         : 0.0;
        lastY += messageTopDelta + messageTextHeight + messageHeightDelta + Ui::DesignSystem::layout().px24();

        lastMessage = message;
    }

    return lastY + Ui::DesignSystem::layout().px16();
}

void MessagesViewContent::paintEvent(QPaintEvent* _event)
{
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

    const QFontMetricsF titleFontMetrics(Ui::DesignSystem::font().subtitle2());
    const QFontMetricsF textFontMetrics(Ui::DesignSystem::font().body2());
    const QColor defaultBaloonColor = ColorHelper::nearby(Ui::DesignSystem::color().background());
    const QColor currentAuthorBaloonColor = ColorHelper::nearby(defaultBaloonColor);
    const qreal maximumTextWidth = width()
                                     - Ui::DesignSystem::layout().px48() // отступ слева под авку и между авкой и текстом
                                     - Ui::DesignSystem::layout().px24() // марджины текста от балуна
                                     - Ui::DesignSystem::layout().px16(); // отступ справа от границы
    qreal lastY = 0.0;
    bool isDateChanged = false;
    bool isAuthorChanged = false;
    bool isCurrentAuthor = false;
    Message lastMessage;
    auto drawAvatar = [&painter, &lastY, &isDateChanged, &isAuthorChanged, &isCurrentAuthor, &lastMessage] {
        if (!lastMessage.author().isValid()) {
            return;
        }

        //
        // Если предыдущее сообщение было не своё, и при этом изменяется пользователь, или день,
        // то отрисуем авку
        //
        if (!isCurrentAuthor && (isDateChanged || isAuthorChanged)) {
            const auto avatarSize = Ui::DesignSystem::treeOneLineItem().iconSize();
            const QRectF avatarRect(QPointF(Ui::DesignSystem::layout().px12(), lastY - avatarSize.height()), avatarSize);
            const auto avatar = ImageHelper::makeAvatar(lastMessage.author().name(), Ui::DesignSystem::font().body2(), avatarSize.toSize(), Qt::white);
            painter.drawPixmap(avatarRect, avatar, avatar.rect());
        }
    };

    for (const auto& message : d->messages) {
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
            const QRectF dateRect(0.0, lastY + Ui::DesignSystem::layout().px16(), width(), textFontMetrics.lineSpacing());
            painter.drawText(dateRect, Qt::AlignCenter, message.dateTime().toString("d MMMM"));

            lastY = dateRect.bottom();
        }


        //
        // Определим область текста
        //
        const qreal messageTextWidth = std::max(std::min(maximumTextWidth,
                                                         textFontMetrics.width(message.text())),
                                                TextHelper::fineTextWidth(isCurrentAuthor
                                                                          ? ""
                                                                          : message.author().name(),
                                                                          titleFontMetrics));
        const qreal messageTextHeight = TextHelper::heightForWidth(message.text(), Ui::DesignSystem::font().body2(), messageTextWidth);
        //
        // Определим область балуна под текст
        //
        const qreal messageTopDelta = isDateChanged ? Ui::DesignSystem::layout().px16()
                                             : isAuthorChanged
                                               ? Ui::DesignSystem::layout().px8()
                                               : Ui::DesignSystem::layout().px2();
        const qreal messageWidth = messageTextWidth + Ui::DesignSystem::layout().px24();
        const qreal messageX = isCurrentAuthor && messageTextWidth < maximumTextWidth
                               ? width() - messageWidth - Ui::DesignSystem::layout().px16()
                               : Ui::DesignSystem::layout().px48();
        const qreal messageHeightDelta = isAuthorChanged && !isCurrentAuthor
                                         ? titleFontMetrics.lineSpacing() + Ui::DesignSystem::layout().px4()
                                         : 0.0;
        const QRectF messageRect(messageX, lastY + messageTopDelta,
                                 messageWidth, messageTextHeight + messageHeightDelta + Ui::DesignSystem::layout().px24());
        const QRectF messageTextRect = messageRect.adjusted(Ui::DesignSystem::layout().px12(),
                                                            Ui::DesignSystem::layout().px12() + messageHeightDelta,
                                                            -Ui::DesignSystem::layout().px12(),
                                                            -Ui::DesignSystem::layout().px12());
        painter.setPen(Qt::NoPen);
        painter.setBrush(isCurrentAuthor ? currentAuthorBaloonColor : defaultBaloonColor);
        painter.drawRoundedRect(messageRect, Ui::DesignSystem::card().borderRadius(), Ui::DesignSystem::card().borderRadius());
        painter.setPen(textColor());
        //
        if (isAuthorChanged && !isCurrentAuthor) {
            painter.setFont(Ui::DesignSystem::font().subtitle2());
            painter.setPen(Ui::DesignSystem::color().secondary());
            painter.drawText(QPointF(messageTextRect.left(),
                                     messageRect.top()
                                     + Ui::DesignSystem::layout().px4()
                                     + titleFontMetrics.lineSpacing()),
                             message.author().name());
            painter.setFont(Ui::DesignSystem::font().body2());
            painter.setPen(textColor());
        }
        //
        painter.drawText(messageTextRect, message.text());

        lastY = messageRect.bottom();

        //
        // Запомним текущее обработанное сообщение
        //
        lastMessage = message;
    }

    //
    // Рисуем авку после того, как нарисовали последнее сообщение
    //
    drawAvatar();
}

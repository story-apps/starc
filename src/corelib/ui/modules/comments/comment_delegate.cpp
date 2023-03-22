#include "comment_delegate.h"

#include "comments_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/avatar_generator/avatar_generator.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemView>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>

using BusinessLayer::CommentsModel;


namespace Ui {

CommentDelegate::CommentDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
}

void CommentDelegate::setSingleCommentMode(bool _isSingleComment)
{
    m_isSingleCommentMode = _isSingleComment;
}

void CommentDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option,
                            const QModelIndex& _index) const
{
    //
    // Получим настройки стиля
    //
    QStyleOptionViewItem opt = _option;
    initStyleOption(&opt, _index);

    //
    // Рисуем ручками
    //
    _painter->setRenderHint(QPainter::Antialiasing, true);

    auto backgroundColor = opt.palette.color(QPalette::Base);
    auto textColor = opt.palette.color(QPalette::Text);
    const auto isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = opt.rect;
    const auto backgroundRectRight = backgroundRect.right()
        + (!isLeftToRight && !m_isSingleCommentMode ? DesignSystem::tree().indicatorWidth() : 0.0);
    if (opt.state.testFlag(QStyle::State_Selected)) {
        //
        // ... для выделенных элементов
        //
        backgroundColor = opt.palette.color(QPalette::Highlight);
        textColor = opt.palette.color(QPalette::HighlightedText);
    } else if (opt.state.testFlag(QStyle::State_MouseOver)) {
        //
        // ... для элементов на которые наведена мышь
        //
        backgroundColor = opt.palette.color(QPalette::AlternateBase);
    } else {
        //
        // ... для остальных элементов
        //
        textColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... цвет заметки
    //
    const QRectF colorRect(
        QPointF(isLeftToRight ? 0.0 : (backgroundRectRight - DesignSystem::layout().px4()),
                backgroundRect.top()),
        QSizeF(DesignSystem::layout().px4(), backgroundRect.height()));
    _painter->fillRect(colorRect, _index.data(CommentsModel::ReviewMarkColorRole).value<QColor>());

    //
    // ... аватар
    //
    const QRectF avatarRect(
        QPointF(isLeftToRight ? (colorRect.right() + DesignSystem::layout().px16())
                              : (colorRect.left() - DesignSystem::layout().px16()
                                 - DesignSystem::treeOneLineItem().avatarSize().width()),
                backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()),
        DesignSystem::treeOneLineItem().avatarSize());
    const auto avatar
        = AvatarGenerator::avatar(_index.data(CommentsModel::ReviewMarkAuthorNameRole).toString(),
                                  _index.data(CommentsModel::ReviewMarkAuthorEmailRole).toString());
    _painter->drawPixmap(avatarRect, avatar, avatar.rect());

    //
    // ... галочка выполнено
    //
    const auto done = _index.data(CommentsModel::ReviewMarkIsDoneRole).toBool();
    QRectF doneIconRect;
    if (m_isSingleCommentMode || done) {
        //
        // ... в режиме единичного комментария также рисуем крестик, который будет закрывать
        // представление с комментарием
        //
        const QSizeF iconSize = DesignSystem::treeOneLineItem().iconSize();
        doneIconRect
            = QRectF(QPointF(isLeftToRight ? (backgroundRectRight - iconSize.width()
                                              - DesignSystem::treeOneLineItem().margins().right())
                                           : (backgroundRect.left()
                                              + DesignSystem::treeOneLineItem().margins().left()),
                             backgroundRect.top() + DesignSystem::treeOneLineItem().margins().top()
                                 + (avatarRect.height() - iconSize.height()) / 2.0),
                     iconSize);
        _painter->setFont(DesignSystem::font().iconsMid());
        _painter->setPen(m_isSingleCommentMode ? textColor : DesignSystem::color().accent());
        _painter->drawText(doneIconRect, Qt::AlignCenter,
                           m_isSingleCommentMode ? u8"\U000f0156" : u8"\U000F012C");
        if (m_isSingleCommentMode && done) {
            if (isLeftToRight) {
                doneIconRect.moveRight(doneIconRect.left());
            } else {
                doneIconRect.moveLeft(doneIconRect.right());
            }
            _painter->setPen(DesignSystem::color().accent());
            _painter->drawText(doneIconRect, Qt::AlignCenter, u8"\U000F012C");
        }
    }

    //
    // ... пользователь
    //
    _painter->setFont(DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal textLeft = isLeftToRight
        ? (avatarRect.right() + DesignSystem::treeOneLineItem().spacing())
        : ((doneIconRect.isEmpty() ? 0.0 : doneIconRect.right())
           + DesignSystem::treeOneLineItem().spacing());
    const qreal textWidth = isLeftToRight
        ? ((doneIconRect.isEmpty() ? backgroundRectRight : doneIconRect.left()) - textLeft
           - DesignSystem::treeOneLineItem().spacing())
        : (avatarRect.left() - textLeft - DesignSystem::treeOneLineItem().spacing());
    const QRectF textRect(QPointF(textLeft, avatarRect.top()),
                          QSizeF(textWidth, avatarRect.height() / 2));
    const auto text = _painter->fontMetrics().elidedText(
        _index.data(CommentsModel::ReviewMarkAuthorNameRole).toString(), Qt::ElideRight,
        static_cast<int>(textRect.width()));
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignBottom, text);
    //
    // ... дата
    //
    _painter->setPen(ColorHelper::transparent(textColor, DesignSystem::disabledTextOpacity()));
    const QRectF dateRect(textRect.bottomLeft(), textRect.size());
    const auto date = _index.data(CommentsModel::ReviewMarkCreationDateRole).toDateTime();
    auto dateText = _painter->fontMetrics().elidedText(date.toString("HH:mm d MMM"), Qt::ElideRight,
                                                       static_cast<int>(dateRect.width()));
    if (_index.data(CommentsModel::ReviewMarkIsEditedRole).toBool()) {
        dateText.append(QString(" (%1)").arg(tr("edited")));
    }
    _painter->drawText(dateRect, Qt::AlignLeft | Qt::AlignTop, dateText);

    //
    // ... комментарий
    //
    const auto comment = _index.data(CommentsModel::ReviewMarkCommentRole).toString();
    QRectF commentRect;
    if (m_isSingleCommentMode || !done) {
        const auto commentWidth = backgroundRectRight - colorRect.width()
            - DesignSystem::layout().px16() - DesignSystem::treeOneLineItem().margins().right();
        if (!comment.isEmpty()) {
            commentRect = QRectF(
                QPointF(isLeftToRight ? avatarRect.left()
                                      : (backgroundRect.left()
                                         + DesignSystem::treeOneLineItem().margins().left()),
                        avatarRect.bottom() + DesignSystem::compactLayout().px8()),
                QSizeF(commentWidth,
                       TextHelper::heightForWidth(comment, DesignSystem::font().body2(),
                                                  commentWidth)));
            _painter->setFont(DesignSystem::font().body2());
            _painter->setPen(textColor);
            QTextOption commentTextOption;
            commentTextOption.setAlignment(isLeftToRight ? Qt::AlignLeft : Qt::AlignRight);
            commentTextOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            _painter->drawText(commentRect, comment, commentTextOption);
        } else {
            commentRect = QRectF(QPointF(isLeftToRight
                                             ? avatarRect.left()
                                             : (backgroundRect.left()
                                                + DesignSystem::treeOneLineItem().margins().left()),
                                         avatarRect.bottom() + DesignSystem::compactLayout().px8()),
                                 QSizeF(commentWidth, 0));
        }
    }

    //
    // ... ответы
    //
    const auto comments = _index.data(CommentsModel::ReviewMarkRepliesRole)
                              .value<QVector<BusinessLayer::TextModelTextItem::ReviewComment>>();
    if (m_isSingleCommentMode || comments.size() <= 1 || done) {
        return;
    }

    const auto avatarSize = DesignSystem::treeOneLineItem().iconSize();
    //
    // ... суммарная информация
    //
    QRectF summaryRect;
    if (comments.size() > 2) {
        summaryRect = QRectF(commentRect.left(),
                             commentRect.bottom() + DesignSystem::compactLayout().px12(),
                             commentRect.width(), avatarSize.height());
        //
        // Формируем список комментарторов
        //
        QVector<QPair<QString, QString>> commentators;
        for (const auto& comment : comments) {
            if (comment == comments.constFirst() || comment == comments.constLast()) {
                continue;
            }

            if (!commentators.contains({ comment.author, comment.authorEmail })) {
                commentators.append({ comment.author, comment.authorEmail });
            }

            if (commentators.size() == 3) {
                break;
            }
        }

        //
        // Рисуем авки комментаторов
        //
        const auto avatarsDistance = avatarSize.width() * 0.7;
        auto avatarXDelta = DesignSystem::layout().px12();
        QRectF avatarRect;
        for (const auto& commentator : commentators) {
            avatarRect = QRectF(isLeftToRight ? (summaryRect.topLeft() + QPointF(avatarXDelta, 0.0))
                                              : (summaryRect.topRight()
                                                 - QPointF(avatarXDelta + avatarSize.width(), 0.0)),
                                avatarSize);
            if (commentator != commentators.constFirst()) {
                QPainterPath path;
                const QPointF clipDelta(DesignSystem::layout().px2(), 0.0);
                if (isLeftToRight) {
                    path.moveTo(avatarRect.topLeft() + clipDelta);
                    path.lineTo(avatarRect.topRight() + clipDelta);
                    path.lineTo(avatarRect.bottomRight() + clipDelta);
                    path.lineTo(avatarRect.bottomLeft() + clipDelta);
                    path.quadTo(avatarRect.center() + clipDelta * 2,
                                avatarRect.topLeft() + clipDelta);
                } else {
                    path.lineTo(avatarRect.topRight() - clipDelta);
                    path.moveTo(avatarRect.topLeft() - clipDelta);
                    path.lineTo(avatarRect.bottomLeft() - clipDelta);
                    path.lineTo(avatarRect.bottomRight() - clipDelta);
                    path.quadTo(avatarRect.center() - clipDelta * 2,
                                avatarRect.topRight() - clipDelta);
                }
            }
            const auto avatar = AvatarGenerator::avatar(commentator.first, commentator.second);
            _painter->drawPixmap(avatarRect, avatar, avatar.rect());
            avatarXDelta += avatarsDistance;
        }

        //
        // Суммарная информация по количеству комментариев
        //
        const QRectF commentsSummaryRect(
            isLeftToRight ? (avatarRect.right() + DesignSystem::treeOneLineItem().spacing())
                          : DesignSystem::treeOneLineItem().spacing(),
            avatarRect.top(),
            (isLeftToRight ? (summaryRect.right() - avatarRect.right()) : avatarRect.left())
                - DesignSystem::layout().px24(),
            avatarRect.height());
        _painter->drawText(commentsSummaryRect, Qt::AlignVCenter | Qt::AlignLeft,
                           tr("%n reply(s)", "", comments.size() - 2));
    }

    //
    // ... последний ответ
    //
    const auto lastComment = comments.constLast();
    QRectF lastCommentAvatarRect(
        QPointF(isLeftToRight
                    ? (commentRect.left() + DesignSystem::layout().px12())
                    : (commentRect.right() - avatarSize.width() - DesignSystem::layout().px12()),
                (summaryRect.isEmpty() ? commentRect.bottom() : summaryRect.bottom())
                    + DesignSystem::compactLayout().px12()),
        avatarSize);

    _painter->setFont(DesignSystem::font().subtitle2());
    const auto titleFontLineSpacing = TextHelper::fineLineSpacing(_painter->font());
    const auto maximumTextWidth
        = (isLeftToRight ? (commentRect.right() - lastCommentAvatarRect.right())
                         : (lastCommentAvatarRect.left() - commentRect.left()))
        - DesignSystem::compactLayout().px24() // марджины текста от балуна
        - DesignSystem::treeOneLineItem().spacing();
    const auto lastCommentTextWidth = std::max(
        std::min(maximumTextWidth,
                 TextHelper::fineTextWidthF(lastComment.text, DesignSystem::font().body2())),
        TextHelper::fineTextWidthF(lastComment.author, _painter->font()));
    const auto lastCommentTextHeight = TextHelper::heightForWidth(
        lastComment.text, DesignSystem::font().body2(), lastCommentTextWidth);
    const auto lastCommentHeightDelta = titleFontLineSpacing + DesignSystem::compactLayout().px4();
    const auto lastCommentWidth = lastCommentTextWidth + DesignSystem::compactLayout().px24();
    const auto lastCommentHeight = lastCommentTextHeight + DesignSystem::compactLayout().px24();
    const QRectF lastCommentRect(
        (isLeftToRight ? (lastCommentAvatarRect.right() + DesignSystem::treeOneLineItem().spacing())
                       : (lastCommentAvatarRect.left() - commentRect.left() - lastCommentWidth)),
        lastCommentAvatarRect.top(), lastCommentWidth, lastCommentHeight + lastCommentHeightDelta);
    const QRectF lastCommentTextRect = lastCommentRect.adjusted(
        DesignSystem::compactLayout().px12(),
        DesignSystem::compactLayout().px12() + lastCommentHeightDelta,
        -DesignSystem::compactLayout().px12(), -DesignSystem::compactLayout().px12());
    _painter->setPen(Qt::NoPen);
    _painter->setBrush(ColorHelper::nearby(backgroundColor));
    _painter->drawRoundedRect(lastCommentRect, DesignSystem::card().borderRadius(),
                              DesignSystem::card().borderRadius());
    _painter->setPen(textColor);
    QTextOption commentTextOption;
    commentTextOption.setAlignment(isLeftToRight ? Qt::AlignLeft : Qt::AlignRight);
    _painter->drawText(QRectF(QPointF(lastCommentTextRect.left(),
                                      lastCommentRect.top() + DesignSystem ::compactLayout().px8()),
                              QSizeF(lastCommentTextRect.width(), titleFontLineSpacing)),
                       lastComment.author, commentTextOption);
    _painter->setFont(DesignSystem::font().body2());
    _painter->setPen(textColor);
    commentTextOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
    _painter->drawText(lastCommentTextRect, lastComment.text, commentTextOption);

    lastCommentAvatarRect.moveBottom(lastCommentRect.bottom());
    const auto lastAvatar = AvatarGenerator::avatar(lastComment.author, lastComment.authorEmail);
    _painter->drawPixmap(lastCommentAvatarRect, lastAvatar, lastAvatar.rect());

    //
    // ... декорация слева
    //
    _painter->setPen(QPen(textColor, DesignSystem::scaleFactor()));
    _painter->setOpacity(DesignSystem::focusBackgroundOpacity());
    _painter->drawLine(QPointF(isLeftToRight ? commentRect.left() : commentRect.right(),
                               commentRect.bottom() + DesignSystem::layout().px16()),
                       QPointF(isLeftToRight ? commentRect.left() : commentRect.right(),
                               lastCommentRect.bottom()));
    _painter->setOpacity(1.0);
}

QSize CommentDelegate::sizeHint(const QStyleOptionViewItem& _option,
                                const QModelIndex& _index) const
{
    //
    // Ширина
    //
    int width = _option.rect.width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }

    //
    // Считаем высоту
    //
    const auto isDone = _index.data(CommentsModel::ReviewMarkIsDoneRole).toBool();
    const auto comment = _index.data(CommentsModel::ReviewMarkCommentRole).toString();
    const auto comments
        = _index.data(CommentsModel::ReviewMarkRepliesRole)
              .value<QVector<BusinessLayer::ScreenplayTextModelTextItem::ReviewComment>>();

    //
    // ... высота заголовка: отступ сверху + высота аватара + отступ снизу
    //
    const int headerHeight = DesignSystem::treeOneLineItem().margins().top()
        + DesignSystem::treeOneLineItem().avatarSize().height()
        + DesignSystem::treeOneLineItem().margins().bottom();
    //
    // ... высота без комментария
    //
    if ((!m_isSingleCommentMode && (isDone || (comment.isEmpty() && comments.size() == 1)))
        || (m_isSingleCommentMode && comment.isEmpty())) {
        return { width, headerHeight };
    }
    //
    // ... полная высота
    //
    int height = headerHeight;
    if (!comment.isEmpty()) {
        //
        // ... ширина - ширина области цвета - левый отступ (фиксированный) - правое поле
        //
        const auto commentWidth = width - DesignSystem::layout().px4()
            - DesignSystem::layout().px16() - DesignSystem::treeOneLineItem().margins().right();
        height += DesignSystem::compactLayout().px8()
            + TextHelper::heightForWidth(comment, DesignSystem::font().body2(), commentWidth);
    }
    //
    // ... комментарии
    //
    if (!m_isSingleCommentMode) {
        if (comments.size() > 1) {
            if (comments.size() > 2) {
                height += DesignSystem::treeOneLineItem().iconSize().height()
                    + DesignSystem::compactLayout().px12() * 2;
            } else {
                height += DesignSystem::compactLayout().px12();
            }

            height += TextHelper::fineLineSpacing(DesignSystem::font().subtitle2())
                + DesignSystem::compactLayout().px4();

            const auto lastComment = comments.constLast();
            const auto maximumTextWidth = width
                - DesignSystem::layout().px4() // ширина области цвета
                - DesignSystem::layout().px16() // левый отступ (фиксированный)
                - DesignSystem::layout().px12() // отступ до декорации коментов
                - DesignSystem::treeOneLineItem().iconSize().width() // аватарка
                - DesignSystem::treeOneLineItem().spacing() // отступ от аватарки
                - DesignSystem::compactLayout().px24() // марджины текста от балуна
                - DesignSystem::treeOneLineItem().margins().right(); // отступ до правого края
            const auto lastCommentTextWidth = std::min(
                maximumTextWidth,
                TextHelper::fineTextWidthF(lastComment.text, DesignSystem::font().body2()));
            const auto lastCommentTextHeight = TextHelper::heightForWidth(
                lastComment.text, DesignSystem::font().body2(), lastCommentTextWidth);
            height += lastCommentTextHeight + DesignSystem::compactLayout().px8()
                + DesignSystem::compactLayout().px24();
        }
    }

    return { width, height };
}

} // namespace Ui

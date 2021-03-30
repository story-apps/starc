#include "screenplay_text_comment_delegate.h"

#include "screenplay_text_comments_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractItemView>
#include <QDateTime>
#include <QPainter>
#include <QPainterPath>

using BusinessLayer::ScreenplayTextCommentsModel;


namespace Ui
{

ScreenplayTextCommentDelegate::ScreenplayTextCommentDelegate(QObject* _parent)
    : QStyledItemDelegate(_parent)
{
}

void ScreenplayTextCommentDelegate::setSingleCommentMode(bool _isSingleComment)
{
    m_isSingleCommentMode = _isSingleComment;
}

void ScreenplayTextCommentDelegate::paint(QPainter* _painter, const QStyleOptionViewItem& _option, const QModelIndex& _index) const
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

    //
    // Рисуем
    //

    //
    // ... фон
    //
    const QRectF backgroundRect = opt.rect;
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
        textColor.setAlphaF(Ui::DesignSystem::inactiveTextOpacity());
    }
    _painter->fillRect(backgroundRect, backgroundColor);

    //
    // ... цвет заметки
    //
    const QRectF colorRect(QPointF(0.0, backgroundRect.top()),
                           QSizeF(Ui::DesignSystem::layout().px4(), backgroundRect.height()));
    _painter->fillRect(colorRect, _index.data(ScreenplayTextCommentsModel::ReviewMarkColorRole).value<QColor>());

    //
    // ... аватар
    //
    const QRectF avatarRect(QPointF(colorRect.right() + Ui::DesignSystem::layout().px16(),
                                    backgroundRect.top() + Ui::DesignSystem::layout().px16()),
                            Ui::DesignSystem::treeOneLineItem().avatarSize());
    const auto avatar = ImageHelper::makeAvatar(_index.data(ScreenplayTextCommentsModel::ReviewMarkAuthorEmailRole).toString(),
                                                Ui::DesignSystem::font().body1(),
                                                avatarRect.size().toSize(),
                                                Qt::white);
    _painter->drawPixmap(avatarRect, avatar, avatar.rect());

    //
    // ... галочка выполнено
    //
    const auto done = _index.data(ScreenplayTextCommentsModel::ReviewMarkIsDoneRole).toBool();
    QRectF iconRect;
    if (m_isSingleCommentMode || done) {
        //
        // ... в режиме единичного комментария также рисуем крестик, который будет закрывать представление с комментарием
        //
        const QSizeF iconSize = Ui::DesignSystem::treeOneLineItem().iconSize();
        iconRect = QRectF(QPointF(backgroundRect.right() - iconSize.width() - Ui::DesignSystem::layout().px12(),
                                  backgroundRect.top() + Ui::DesignSystem::layout().px16() + Ui::DesignSystem::layout().px4()),
                          iconSize);
        _painter->setFont(Ui::DesignSystem::font().iconsMid());
        _painter->setPen(m_isSingleCommentMode ? textColor : Ui::DesignSystem::color().secondary());
        _painter->drawText(iconRect, Qt::AlignCenter, m_isSingleCommentMode ? u8"\U000f0156" : u8"\U000F012C");
        if (m_isSingleCommentMode && done) {
            iconRect.moveRight(iconRect.left());
            _painter->setPen(Ui::DesignSystem::color().secondary());
            _painter->drawText(iconRect, Qt::AlignCenter, u8"\U000F012C");
        }
    }

    //
    // ... пользователь
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    _painter->setPen(textColor);
    const qreal textLeft = avatarRect.right() + Ui::DesignSystem::layout().px12();
    const qreal textWidth = (iconRect.isEmpty() ? backgroundRect.right() : iconRect.left())
                            - textLeft - Ui::DesignSystem::layout().px12();

    const QRectF textRect(QPointF(textLeft, avatarRect.top()),
                          QSizeF(textWidth, avatarRect.height() / 2));
    const auto text = _painter->fontMetrics().elidedText(
                          _index.data(ScreenplayTextCommentsModel::ReviewMarkAuthorEmailRole).toString(),
                          Qt::ElideRight,
                          static_cast<int>(textRect.width()));
    _painter->drawText(textRect, Qt::AlignLeft | Qt::AlignBottom, text);
    //
    // ... дата
    //
    _painter->setPen(ColorHelper::transparent(textColor, Ui::DesignSystem::disabledTextOpacity()));
    const QRectF dateRect(textRect.bottomLeft(), textRect.size());
    const auto date = _index.data(ScreenplayTextCommentsModel::ReviewMarkCreationDateRole).toDateTime();
    const auto dateText = _painter->fontMetrics().elidedText(date.toString("HH:mm d MMM"), Qt::ElideRight, static_cast<int>(dateRect.width()));
    _painter->drawText(dateRect, Qt::AlignLeft | Qt::AlignTop, dateText);

    //
    // ... комментарий
    //
    const auto comment = _index.data(ScreenplayTextCommentsModel::ReviewMarkCommentRole).toString();
    QRectF commentRect;
    if (m_isSingleCommentMode || !done) {
        const auto commentWidth = backgroundRect.right() - Ui::DesignSystem::layout().px16() - Ui::DesignSystem::layout().px16() - Ui::DesignSystem::layout().px8();
        if (!comment.isEmpty()) {
            commentRect = QRectF(QPointF(avatarRect.left(),
                                         avatarRect.bottom() + Ui::DesignSystem::layout().px12()),
                                 QSizeF(commentWidth,
                                        TextHelper::heightForWidth(comment, Ui::DesignSystem::font().body2(), commentWidth)));
            _painter->setFont(Ui::DesignSystem::font().body2());
            _painter->setPen(textColor);
            QTextOption commentTextOption;
            commentTextOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
            _painter->drawText(commentRect, comment, commentTextOption);
        } else {
            commentRect = QRectF(avatarRect.bottomLeft(), QSizeF(commentWidth, 0));
        }
    }

    //
    // ... ответы
    //
    const auto comments = _index.data(ScreenplayTextCommentsModel::ReviewMarkCommentsRole)
                                .value<QVector<BusinessLayer::ScreenplayTextModelTextItem::ReviewComment>>();
    if (!m_isSingleCommentMode && comments.size() > 1 && !done) {
        const auto avatarSize = Ui::DesignSystem::treeOneLineItem().iconSize();
        //
        // ... суммарная информация
        //
        QRectF summaryRect;
        if (comments.size() > 2) {
            summaryRect = QRectF(commentRect.left(),
                                 commentRect.bottom() + Ui::DesignSystem::layout().px16(),
                                 commentRect.width(),
                                 avatarSize.height());
            //
            // Формируем список комментарторов
            //
            QVector<QString> commentators;
            for (const auto& comment : comments) {
                if (comment == comments.constFirst()
                    || comment == comments.constLast()) {
                    continue;
                }

                if (!commentators.contains(comment.author)) {
                    commentators.append(comment.author);
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
                avatarRect = QRectF(summaryRect.topLeft() + QPointF(avatarXDelta, 0.0), avatarSize);
                if (commentator != commentators.constFirst()) {
                    QPainterPath path;
                    const QPointF clipDelta(Ui::DesignSystem::layout().px2(), 0.0);
                    path.moveTo(avatarRect.topLeft() + clipDelta);
                    path.lineTo(avatarRect.topRight() + clipDelta);
                    path.lineTo(avatarRect.bottomRight() + clipDelta);
                    path.lineTo(avatarRect.bottomLeft() + clipDelta);
                    path.quadTo(avatarRect.center() + clipDelta*2, avatarRect.topLeft() + clipDelta);
                    _painter->setClipPath(path);
                }
                const auto avatar = ImageHelper::makeAvatar(commentator, Ui::DesignSystem::font().body2(), avatarSize.toSize(), Qt::white);
                _painter->drawPixmap(avatarRect.topLeft(), avatar);
                avatarXDelta += avatarsDistance;
                _painter->setClipping(false);
            }

            //
            // Суммарная информация по количеству комментариев
            //
            const QRectF commentsSummaryRect(avatarRect.right() + Ui::DesignSystem::layout().px12(),
                                             avatarRect.top(),
                                             summaryRect.right() - avatarRect.right() - Ui::DesignSystem::layout().px24(),
                                             avatarRect.height());
            _painter->drawText(commentsSummaryRect, Qt::AlignVCenter | Qt::AlignLeft, tr("%n reply(s)", "", comments.size() - 2));
        }

        //
        // ... последний ответ
        //
        const auto lastComment = comments.constLast();
        QRectF lastCommentAvatarRect(QPointF(commentRect.left() + Ui::DesignSystem::layout().px12(),
                                             (summaryRect.isEmpty() ? commentRect.bottom()
                                                                    : summaryRect.bottom()) + Ui::DesignSystem::layout().px16()),
                                     avatarSize);

        const QFontMetricsF titleFontMetrics(Ui::DesignSystem::font().subtitle2());
        const auto maximumTextWidth = commentRect.right()
                                      - lastCommentAvatarRect.right()
                                      - Ui::DesignSystem::layout().px24() // марджины текста от балуна
                                      - Ui::DesignSystem::layout().px12();
        const auto lastCommentTextWidth = std::max(std::min(maximumTextWidth,
                                                            TextHelper::fineTextWidth(lastComment.text, Ui::DesignSystem::font().body2())),
                                                   TextHelper::fineTextWidth(lastComment.author, titleFontMetrics));
        const auto lastCommentTextHeight = TextHelper::heightForWidth(lastComment.text, Ui::DesignSystem::font().body2(), lastCommentTextWidth);
        const auto lastCommentHeightDelta = titleFontMetrics.lineSpacing() + Ui::DesignSystem::layout().px4();
        const auto lastCommentWidth = lastCommentTextWidth + Ui::DesignSystem::layout().px24();
        const auto lastCommentHeight = lastCommentTextHeight + Ui::DesignSystem::layout().px24();
        const QRectF lastCommentRect(lastCommentAvatarRect.right() + Ui::DesignSystem::layout().px12(),
                                     lastCommentAvatarRect.top(),
                                     lastCommentWidth,
                                     lastCommentHeight + lastCommentHeightDelta);
        const QRectF lastCommentTextRect = lastCommentRect.adjusted(Ui::DesignSystem::layout().px12(),
                                                                    Ui::DesignSystem::layout().px12() + lastCommentHeightDelta,
                                                                    -Ui::DesignSystem::layout().px12(),
                                                                    -Ui::DesignSystem::layout().px12());
        _painter->setPen(Qt::NoPen);
        _painter->setBrush(ColorHelper::nearby(backgroundColor));
        _painter->drawRoundedRect(lastCommentRect, Ui::DesignSystem::card().borderRadius(), Ui::DesignSystem::card().borderRadius());
        _painter->setFont(Ui::DesignSystem::font().subtitle2());
        _painter->setPen(textColor);
        _painter->drawText(QPointF(lastCommentTextRect.left(),
                                   lastCommentRect.top()
                                   + Ui::DesignSystem::layout().px4()
                                   + titleFontMetrics.lineSpacing()),
                           lastComment.author);
        _painter->setFont(Ui::DesignSystem::font().body2());
        _painter->setPen(textColor);
        QTextOption commentTextOption;
        commentTextOption.setWrapMode(QTextOption::WrapAtWordBoundaryOrAnywhere);
        _painter->drawText(lastCommentTextRect, lastComment.text, commentTextOption);

        lastCommentAvatarRect.moveBottom(lastCommentRect.bottom());
        const auto avatar = ImageHelper::makeAvatar(lastComment.author, Ui::DesignSystem::font().body2(), avatarSize.toSize(), Qt::white);
        _painter->drawPixmap(lastCommentAvatarRect.topLeft(), avatar);


        //
        // ... декорация слева
        //
        _painter->setPen(QPen(textColor, Ui::DesignSystem::scaleFactor()));
        _painter->setOpacity(Ui::DesignSystem::focusBackgroundOpacity());
        _painter->drawLine(QPointF(commentRect.left(),
                                   commentRect.bottom() + Ui::DesignSystem::layout().px16()),
                           QPointF(commentRect.left(),
                                   lastCommentRect.bottom()));
        _painter->setOpacity(1.0);
    }
}

QSize ScreenplayTextCommentDelegate::sizeHint(const QStyleOptionViewItem& _option, const QModelIndex& _index) const
{
    //
    // Ширина
    //
    int width = _option.rect.width();
    if (const QAbstractItemView* view = qobject_cast<const QAbstractItemView*>(_option.widget)) {
        width = view->viewport()->width();
    }
    width -= Ui::DesignSystem::layout().px8()
             + Ui::DesignSystem::layout().px16()
             + Ui::DesignSystem::layout().px16();

    //
    // Считаем высоту
    //
    const auto isDone = _index.data(ScreenplayTextCommentsModel::ReviewMarkIsDoneRole).toBool();
    const auto comment = _index.data(ScreenplayTextCommentsModel::ReviewMarkCommentRole).toString();
    const auto comments = _index.data(ScreenplayTextCommentsModel::ReviewMarkCommentsRole)
                          .value<QVector<BusinessLayer::ScreenplayTextModelTextItem::ReviewComment>>();

    //
    // ... высота заголовка: отступ сверху + высота аватара + отступ снизу
    //
    const int headerHeight = Ui::DesignSystem::layout().px16()
                             + Ui::DesignSystem::treeOneLineItem().avatarSize().height()
                             + Ui::DesignSystem::layout().px16();
    //
    // ... высота без комментария
    //
    if ((!m_isSingleCommentMode
         && (isDone
             || (comment.isEmpty()
                 && comments.size() == 1)))
        || (m_isSingleCommentMode
            && comment.isEmpty())) {
        return { width, headerHeight };
    }
    //
    // ... полная высота
    //
    int height = headerHeight
                 + (comment.isEmpty() ? 0.0
                                      : Ui::DesignSystem::layout().px12()
                                        + TextHelper::heightForWidth(comment, Ui::DesignSystem::font().body2(), width));
    //
    // ... комментарии
    //
    if (!m_isSingleCommentMode) {
        if (comments.size() > 1) {
            if (comments.size() > 2) {
                height += Ui::DesignSystem::treeOneLineItem().iconSize().height()
                          + Ui::DesignSystem::layout().px16() * 2;
            } else {
                height += Ui::DesignSystem::layout().px16();
            }

            height += QFontMetricsF(Ui::DesignSystem::font().subtitle2()).lineSpacing() + Ui::DesignSystem::layout().px4();

            const auto lastComment = comments.constLast();
            const auto maximumTextWidth = width
                                          - Ui::DesignSystem::layout().px12() // отступ до декорации коментов
                                          - Ui::DesignSystem::treeOneLineItem().iconSize().width() // аватарка
                                          - Ui::DesignSystem::layout().px12() // отступ от аватарки
                                          - Ui::DesignSystem::layout().px24() // марджины текста от балуна
                                          - Ui::DesignSystem::layout().px12(); // отступ до правого края
            const auto lastCommentTextWidth = std::min(maximumTextWidth,
                                                       TextHelper::fineTextWidth(lastComment.text, Ui::DesignSystem::font().body2()));
            const auto lastCommentTextHeight = TextHelper::heightForWidth(lastComment.text, Ui::DesignSystem::font().body2(), lastCommentTextWidth);
            height += lastCommentTextHeight
                      + Ui::DesignSystem::layout().px8()
                      + Ui::DesignSystem::layout().px24();
        }
    }

    return { width, height };
}

} // namespace Ui

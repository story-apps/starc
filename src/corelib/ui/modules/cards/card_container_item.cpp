#include "card_container_item.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QVariantAnimation>


namespace Ui {

class CardContainerItem::Implementation
{
public:
    explicit Implementation(CardContainerItem* _q);


    /**
     * @brief Владелец
     */
    CardContainerItem* q = nullptr;

    /**
     * @brief Декорации при клике
     */
    ClickAnimation decorationAnimation;
};

CardContainerItem::Implementation::Implementation(CardContainerItem* _q)
    : q(_q)
{
    decorationAnimation.setFast(false);
}

// ****

CardContainerItem::CardContainerItem(QGraphicsItem* _parent)
    : AbstractCardItem(_parent)
    , d(new Implementation(this))
{
    setRect(QRectF({ 0, 0 }, Ui::DesignSystem::projectCard().size()));
    setAcceptHoverEvents(true);

    setFlag(QGraphicsItem::ItemIsSelectable, true);
    setFlag(QGraphicsItem::ItemIsMovable, true);
    setFlag(QGraphicsItem::ItemSendsScenePositionChanges, true);
    setFlag(QGraphicsItem::ItemSendsGeometryChanges, true);
    setCacheMode(QGraphicsItem::DeviceCoordinateCache);

    QObject::connect(&d->decorationAnimation, &ClickAnimation::valueChanged,
                     &d->decorationAnimation, [this] { update(); });
}

int CardContainerItem::type() const
{
    return Type;
}

qreal CardContainerItem::headerHeight() const
{
    qreal descriptionHeight = 0.0;
    if (const auto description = modelItemIndex().data(Qt::WhatsThisRole).toString();
        isOpened() && !description.isEmpty()) {
        const QRectF backgroundRect
            = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
        const auto availableWidth = backgroundRect.width() - Ui::DesignSystem::layout().px16() * 2;
        descriptionHeight
            = std::min(TextHelper::heightForWidth(description.simplified(),
                                                  Ui::DesignSystem::font().body2(), availableWidth),
                       TextHelper::fineLineSpacing(Ui::DesignSystem::font().body2()) * 3)
            + Ui::DesignSystem::layout().px8();
    }
    return Ui::DesignSystem::card().shadowMargins().top() + Ui::DesignSystem::layout().px16()
        + TextHelper::fineLineSpacing(Ui::DesignSystem::font().subtitle2())
        + Ui::DesignSystem::layout().px12() + descriptionHeight;
}

bool CardContainerItem::isContainer() const
{
    return true;
}

bool CardContainerItem::isOpened() const
{
    return modelItemIndex().data(Qt::UserRole).toBool();
}

void CardContainerItem::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option,
                              QWidget* _widget)
{
    Q_UNUSED(_widget)

    const bool isLeftToRight = _option->direction == Qt::LeftToRight;
    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Готовим фон
    //
    auto backgroundPixmap = QPixmap(backgroundRect.size().toSize());
    const auto backgroundColor = isReadyForEmbed() ? Ui::DesignSystem::color().accent()
                                                   : Ui::DesignSystem::color().background();
    backgroundPixmap.fill(Qt::transparent);
    QPainter backgroundImagePainter(&backgroundPixmap);
    backgroundImagePainter.setRenderHint(QPainter::Antialiasing);
    backgroundImagePainter.setPen(Qt::NoPen);
    backgroundImagePainter.setBrush(backgroundColor.darker(130));
    const auto borderRadius = Ui::DesignSystem::card().borderRadius();
    const auto decorationWidth = Ui::DesignSystem::layout().px(124);
    const auto decorationHeight = Ui::DesignSystem::layout().px16();
    const auto borderDelta = borderRadius * 2;
    backgroundImagePainter.drawRoundedRect(
        QRectF(QPointF(0, decorationHeight),
               QSizeF(backgroundPixmap.width(), backgroundPixmap.height() - decorationHeight)),
        borderRadius, borderRadius);
    backgroundImagePainter.drawRoundedRect(
        QRectF(QPointF(0, 0), QSizeF(decorationWidth, decorationHeight + borderRadius)),
        borderRadius, borderRadius);
    QPainterPath decorationTriangle({ decorationWidth - borderRadius, 0.0 });
    decorationTriangle.lineTo(decorationWidth, 0.0);
    decorationTriangle.lineTo(decorationWidth + decorationHeight, decorationHeight);
    decorationTriangle.lineTo(decorationWidth - borderRadius, decorationHeight);
    decorationTriangle.lineTo(decorationWidth - borderRadius, 0.0);
    backgroundImagePainter.drawPath(decorationTriangle);
    if (!modelItemIndex().data(Qt::UserRole).toBool()) {
        //
        // ... рисуем вложения папки, при подсчётах учитываем, что в папках всегда вложены как
        //     минимум 2 элемента - это заголовок и окончание папки
        //
        if (childCount() >= 4) {
            backgroundImagePainter.setBrush(backgroundColor.darker(114));
            backgroundImagePainter.drawRoundedRect(
                QRectF(
                    QPointF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px(58)),
                    QSizeF(backgroundPixmap.width() - Ui::DesignSystem::layout().px24() * 2,
                           backgroundPixmap.height() - Ui::DesignSystem::layout().px(58))),
                borderRadius, borderRadius);
        }
        if (childCount() >= 3) {
            backgroundImagePainter.setBrush(backgroundColor.darker(107));
            backgroundImagePainter.drawRoundedRect(
                QRectF(
                    QPointF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px(70)),
                    QSizeF(backgroundPixmap.width() - Ui::DesignSystem::layout().px12() * 2,
                           backgroundPixmap.height() - Ui::DesignSystem::layout().px(70))),
                borderRadius, borderRadius);
        }
        backgroundImagePainter.setBrush(backgroundColor);
        backgroundImagePainter.drawRoundedRect(
            QRectF(QPointF(0, Ui::DesignSystem::layout().px(82)),
                   QSizeF(backgroundPixmap.width(),
                          backgroundPixmap.height() - Ui::DesignSystem::layout().px(82))),
            borderRadius, borderRadius);
    }
    //
    // ... рисуем тень
    //
    const qreal shadowHeight = Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius();
    const QPixmap shadow = ImageHelper::dropShadow(
        backgroundPixmap, Ui::DesignSystem::floatingToolBar().shadowMargins(), shadowHeight,
        Ui::DesignSystem::color().shadow());
    _painter->drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    _painter->drawPixmap(backgroundRect, backgroundPixmap, backgroundPixmap.rect());
    //
    // ... если карточка выбрана, рисуем обводку
    //
    QPainterPath borderPath(backgroundRect.topLeft() + QPointF(borderDelta, 0));
    borderPath.lineTo(backgroundRect.topLeft() + QPointF(decorationWidth, 0.0));
    borderPath.lineTo(backgroundRect.topLeft()
                      + QPointF(decorationWidth + decorationHeight, decorationHeight));
    borderPath.lineTo(backgroundRect.topRight() - QPointF(borderDelta, -decorationHeight));
    borderPath.cubicTo(backgroundRect.topRight() + QPointF(0, decorationHeight),
                       backgroundRect.topRight() + QPointF(0, decorationHeight),
                       backgroundRect.topRight() + QPointF(0, decorationHeight + borderDelta));
    borderPath.lineTo(backgroundRect.bottomRight() - QPointF(0, borderDelta));
    borderPath.cubicTo(backgroundRect.bottomRight(), backgroundRect.bottomRight(),
                       backgroundRect.bottomRight() - QPointF(borderDelta, 0));
    borderPath.lineTo(backgroundRect.bottomLeft() + QPointF(borderDelta, 0));
    borderPath.cubicTo(backgroundRect.bottomLeft(), backgroundRect.bottomLeft(),
                       backgroundRect.bottomLeft() - QPointF(0, borderDelta));
    borderPath.lineTo(backgroundRect.topLeft() + QPointF(0, borderDelta));
    borderPath.cubicTo(backgroundRect.topLeft(), backgroundRect.topLeft(),
                       backgroundRect.topLeft() + QPointF(borderDelta, 0));
    if (isSelected()) {
        _painter->setBrush(Qt::NoBrush);
        _painter->setPen(
            QPen(Ui::DesignSystem::color().accent(), Ui::DesignSystem::layout().px2()));
        _painter->drawPath(borderPath);
    }

    //
    // Иконка состояния группы
    //
    const auto textColor = isReadyForEmbed() ? Ui::DesignSystem::color().onAccent()
                                             : Ui::DesignSystem::color().onBackground();
    _painter->setPen(textColor);
    _painter->setFont(Ui::DesignSystem::font().iconsMid());
    const QSizeF iconSize(
        QSizeF(Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16()));
    const QRectF iconRect(
        QPointF(isLeftToRight
                    ? backgroundRect.left() + Ui::DesignSystem::layout().px12()
                    : backgroundRect.right() - iconSize.width() - Ui::DesignSystem::layout().px12(),
                backgroundRect.top() + decorationHeight + Ui::DesignSystem::layout().px(13)),
        iconSize);
    _painter->drawText(iconRect, Qt::AlignCenter,
                       modelItemIndex().data(Qt::UserRole).toBool()
                           ? u8"\U000F035D"
                           : (isLeftToRight ? u8"\U000F035F" : u8"\U000F035E"));

    //
    // Заголовок или название
    //
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    const QRectF headerRect(
        isLeftToRight ? iconRect.right() + Ui::DesignSystem::layout().px8()
                      : backgroundRect.left() + Ui::DesignSystem::layout().px16(),
        backgroundRect.top() + decorationHeight + Ui::DesignSystem::layout().px12(),
        backgroundRect.width() - Ui::DesignSystem::layout().px16() * 2 - iconSize.width()
            - Ui::DesignSystem::layout().px8(),
        TextHelper::fineLineSpacing(_painter->font()));
    _painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignVCenter,
                       TextHelper::elidedText(modelItemIndex().data(Qt::DisplayRole).toString(),
                                              _painter->font(), headerRect.width()));

    //
    // Описание
    //
    if (const auto description = modelItemIndex().data(Qt::WhatsThisRole).toString();
        !description.isEmpty()) {
        _painter->setFont(Ui::DesignSystem::font().body2());

        qreal descriptionHeight = 0.0;
        qreal descriptionY = 0.0;
        const auto descriptionWidth
            = backgroundRect.width() - Ui::DesignSystem::layout().px16() * 2;
        if (modelItemIndex().data(Qt::UserRole).toBool()) {
            descriptionHeight = std::min(
                TextHelper::heightForWidth(description.simplified(),
                                           Ui::DesignSystem::font().body2(), descriptionWidth),
                TextHelper::fineLineSpacing(Ui::DesignSystem::font().body2()) * 3);
            descriptionY = headerRect.bottom() + Ui::DesignSystem::layout().px8();
        } else {
            const auto stampTopMargin = Ui::DesignSystem::layout().px(82);
            descriptionHeight
                = backgroundRect.height() - stampTopMargin - Ui::DesignSystem::layout().px8() * 2;
            descriptionY = backgroundRect.top() + stampTopMargin + Ui::DesignSystem::layout().px8();
        }
        QRectF descriptionRect(backgroundRect.left() + Ui::DesignSystem::layout().px16(),
                               descriptionY, descriptionWidth, descriptionHeight);
        const QString descriptionCorrected
            = TextHelper::elidedText(description.simplified(), _painter->font(), descriptionRect);
        _painter->drawText(descriptionRect, Qt::AlignLeft | Qt::TextWordWrap, descriptionCorrected);
    }

    //
    // Декорация
    //
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        _painter->setPen(Qt::NoPen);
        _painter->setBrush(Ui::DesignSystem::color().accent());
        _painter->setClipPath(borderPath);
        _painter->setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        _painter->drawEllipse(d->decorationAnimation.clickPosition(), radius, radius);
        _painter->setClipRect(QRectF(), Qt::NoClip);
        _painter->setOpacity(1.0);
    }
}

void CardContainerItem::mousePressEvent(QGraphicsSceneMouseEvent* _event)
{
    AbstractCardItem::mousePressEvent(_event);

    const auto backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    if (!backgroundRect.contains(_event->pos())) {
        return;
    }

    d->decorationAnimation.setClickPosition(_event->pos());
    d->decorationAnimation.setRadiusInterval(
        0.0, std::max(backgroundRect.width(), backgroundRect.height()));
    d->decorationAnimation.start();
}

void CardContainerItem::mouseReleaseEvent(QGraphicsSceneMouseEvent* _event)
{
    AbstractCardItem::mouseReleaseEvent(_event);

    const bool isLeftToRight = _event->widget() == nullptr || _event->widget()->isLeftToRight();
    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    const auto decorationHeight = Ui::DesignSystem::layout().px16();
    const QSizeF iconSizeWithMargins(
        QSizeF(Ui::DesignSystem::layout().px(40), Ui::DesignSystem::layout().px(40)));
    const QRectF iconRect(QPointF(isLeftToRight
                                      ? backgroundRect.left()
                                      : backgroundRect.right() - iconSizeWithMargins.width(),
                                  backgroundRect.top() + decorationHeight),
                          iconSizeWithMargins);
    if (iconRect.contains(_event->pos())) {
        auto model = const_cast<QAbstractItemModel*>(modelItemIndex().model());
        model->setData(modelItemIndex(), !modelItemIndex().data(Qt::UserRole).toBool(),
                       Qt::UserRole);

        prepareGeometryChange();
        update();
    }
}

void CardContainerItem::mouseDoubleClickEvent(QGraphicsSceneMouseEvent* _event)
{
    AbstractCardItem::mouseDoubleClickEvent(_event);

    auto model = const_cast<QAbstractItemModel*>(modelItemIndex().model());
    model->setData(modelItemIndex(), !modelItemIndex().data(Qt::UserRole).toBool(), Qt::UserRole);

    prepareGeometryChange();
    update();

    _event->accept();
}

} // namespace Ui

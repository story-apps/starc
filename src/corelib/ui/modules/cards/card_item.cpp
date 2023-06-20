#include "card_item.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QGraphicsSceneMouseEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <QVariantAnimation>


namespace Ui {

class CardItem::Implementation
{
public:
    explicit Implementation(CardItem* _q);


    /**
     * @brief Владелец
     */
    CardItem* q = nullptr;

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation shadowHeightAnimation;

    /**
     * @brief  Декорации при клике
     */
    ClickAnimation decorationAnimation;
};

CardItem::Implementation::Implementation(CardItem* _q)
    : q(_q)
{
    decorationAnimation.setFast(false);
}


// ****


CardItem::CardItem(QGraphicsItem* _parent)
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

    d->shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    d->shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());
    d->shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    d->shadowHeightAnimation.setDuration(160);
    QObject::connect(&d->shadowHeightAnimation, &QVariantAnimation::valueChanged,
                     &d->shadowHeightAnimation, [this] { update(); });
    QObject::connect(&d->decorationAnimation, &ClickAnimation::valueChanged,
                     &d->decorationAnimation, [this] { update(); });
}

CardItem::~CardItem()
{
    d->shadowHeightAnimation.disconnect();
    d->shadowHeightAnimation.stop();
}

int CardItem::type() const
{
    return Type;
}

void CardItem::paint(QPainter* _painter, const QStyleOptionGraphicsItem* _option, QWidget* _widget)
{
    Q_UNUSED(_widget)
    Q_UNUSED(_option)

    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Готовим фон
    //
    auto backgroundPixmap = QPixmap(backgroundRect.size().toSize());
    const auto backgroundColor = Ui::DesignSystem::color().background();
    backgroundPixmap.fill(Qt::transparent);
    QPainter backgroundImagePainter(&backgroundPixmap);
    backgroundImagePainter.setRenderHint(QPainter::Antialiasing);
    backgroundImagePainter.setPen(Qt::NoPen);
    backgroundImagePainter.setBrush(backgroundColor);
    const qreal borderRadius = Ui::DesignSystem::card().borderRadius();
    backgroundImagePainter.drawRoundedRect(QRect({ 0, 0 }, backgroundPixmap.size()), borderRadius,
                                           borderRadius);
    //
    // ... рисуем тень
    //
    const qreal shadowHeight
        = std::max(Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius(),
                   d->shadowHeightAnimation.currentValue().toReal());
    const QPixmap shadow = ImageHelper::dropShadow(
        backgroundPixmap, Ui::DesignSystem::floatingToolBar().shadowMargins(), shadowHeight,
        Ui::DesignSystem::color().shadow(), true);
    _painter->drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    _painter->drawPixmap(backgroundRect, backgroundPixmap, backgroundPixmap.rect());
    //
    // ... если карточка выбрана, рисуем обводку
    //
    if (isSelected()) {
        _painter->setBrush(Qt::NoBrush);
        _painter->setPen(
            QPen(Ui::DesignSystem::color().accent(), Ui::DesignSystem::layout().px2()));
        _painter->drawRoundedRect(backgroundRect, borderRadius, borderRadius);
    }

    //
    // Заголовок
    //
    const auto headerText = modelItemIndex().data(Qt::DisplayRole).toString();
    const auto textColor = Ui::DesignSystem::color().onBackground();
    _painter->setPen(textColor);
    _painter->setFont(Ui::DesignSystem::font().subtitle2());
    const auto headerWidth = backgroundRect.width() - Ui::DesignSystem::layout().px16() * 2;
    const auto idealHeaderHeight
        = TextHelper::heightForWidth(headerText, _painter->font(), headerWidth);
    const auto maximumHeaderHeight = backgroundRect.height() - Ui::DesignSystem::layout().px24();
    const QRectF headerRect(backgroundRect.left() + Ui::DesignSystem::layout().px16(),
                            backgroundRect.top() + Ui::DesignSystem::layout().px12(), headerWidth,
                            std::min(idealHeaderHeight, maximumHeaderHeight));
    _painter->drawText(headerRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                       TextHelper::elidedText(headerText, _painter->font(), headerRect));

    //
    // Текст или описание
    //
    _painter->setFont(Ui::DesignSystem::font().body2());
    const auto descriptionRectTop = headerRect.bottom() + Ui::DesignSystem::layout().px8();
    const auto descriptionBottomMargin = Ui::DesignSystem::layout().px12();
    const auto descriptionHeight
        = backgroundRect.bottom() - descriptionRectTop - descriptionBottomMargin;
    if (descriptionHeight > 0) {
        const auto descriptionText = modelItemIndex().data(Qt::WhatsThisRole).toString();
        const QRectF descriptionRect(headerRect.left(), descriptionRectTop, headerRect.width(),
                                     std::max(0.0, descriptionHeight));
        const QString descriptionCorrected
            = TextHelper::elidedText(descriptionText, _painter->font(), descriptionRect);
        _painter->drawText(descriptionRect, Qt::AlignLeft | Qt::AlignTop | Qt::TextWordWrap,
                           descriptionCorrected);
    }

    //
    // Декорация
    //
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        _painter->setPen(Qt::NoPen);
        _painter->setBrush(Ui::DesignSystem::color().accent());
        _painter->setClipRect(backgroundRect);
        _painter->setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        _painter->drawEllipse(d->decorationAnimation.clickPosition(), radius, radius);
        _painter->setClipRect(QRectF(), Qt::NoClip);
        _painter->setOpacity(1.0);
    }
}

void CardItem::hoverEnterEvent(QGraphicsSceneHoverEvent* _event)
{
    AbstractCardItem::hoverEnterEvent(_event);
    d->shadowHeightAnimation.setDirection(QVariantAnimation::Forward);
    d->shadowHeightAnimation.start();
}

void CardItem::hoverLeaveEvent(QGraphicsSceneHoverEvent* _event)
{
    AbstractCardItem::hoverLeaveEvent(_event);
    d->shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
    d->shadowHeightAnimation.start();
}

void CardItem::mousePressEvent(QGraphicsSceneMouseEvent* _event)
{
    if (!boundingRect().contains(_event->pos())) {
        if (d->shadowHeightAnimation.direction() == QVariantAnimation::Forward) {
            d->shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
            d->shadowHeightAnimation.start();
        }
        return;
    }

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

} // namespace Ui

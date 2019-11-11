#include "card.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QPainter>
#include <QVariantAnimation>


class Card::Implementation
{
public:
    Implementation();

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation m_shadowHeightAnimation;
};

Card::Implementation::Implementation()
{
    m_shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    m_shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());
    m_shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    m_shadowHeightAnimation.setDuration(160);
}


Card::Card(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setAttribute(Qt::WA_Hover);

    connect(&d->m_shadowHeightAnimation, &QVariantAnimation::valueChanged, [this] { update(); });
}

Card::~Card() = default;

void Card::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    const QRectF backgroundRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Заливаем фон
    //
    QPixmap backgroundImage(backgroundRect.size().toSize());
    backgroundImage.fill(Qt::transparent);
    QPainter backgroundImagePainter(&backgroundImage);
    backgroundImagePainter.setPen(Qt::NoPen);
    backgroundImagePainter.setBrush(Ui::DesignSystem::color().background());
    const qreal borderRadius = Ui::DesignSystem::card().borderRadius();
    backgroundImagePainter.drawRoundedRect(QRect({0,0}, backgroundImage.size()), borderRadius, borderRadius);
    //
    // ... рисуем тень
    //
    const qreal shadowHeight = std::max(Ui::DesignSystem::floatingToolBar().minimumShadowBlurRadius(),
                                        d->m_shadowHeightAnimation.currentValue().toReal());
    const QPixmap shadow
            = ImageHelper::dropShadow(backgroundImage,
                                      Ui::DesignSystem::floatingToolBar().shadowMargins(),
                                      shadowHeight,
                                      Ui::DesignSystem::color().shadow());

    QPainter painter(this);
    painter.drawPixmap(0, 0, shadow);
    //
    // ... рисуем сам фон
    //
    painter.setPen(Qt::NoPen);
    painter.setBrush(Ui::DesignSystem::color().background());
    painter.drawRoundedRect(backgroundRect, borderRadius, borderRadius);
}

void Card::enterEvent(QEvent* _event)
{
    Widget::enterEvent(_event);

    d->m_shadowHeightAnimation.setDirection(QVariantAnimation::Forward);
    d->m_shadowHeightAnimation.start();
}

void Card::leaveEvent(QEvent* _event)
{
    Widget::leaveEvent(_event);

    d->m_shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
    d->m_shadowHeightAnimation.start();
}

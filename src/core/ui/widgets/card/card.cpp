#include "card.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

#include <QApplication>
#include <QPainter>
#include <QVariantAnimation>
#include <QVBoxLayout>


class Card::Implementation
{
public:
    Implementation();

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation shadowHeightAnimation;

    /**
     * @brief Компоновщик карточки
     */
    QVBoxLayout* layout = nullptr;
};

Card::Implementation::Implementation()
    : layout(new QVBoxLayout)
{
    layout->setSpacing(0);
    layout->setContentsMargins({});

    shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());
    shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    shadowHeightAnimation.setDuration(160);
}


Card::Card(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setAttribute(Qt::WA_Hover);

    setLayout(d->layout);

    connect(&d->shadowHeightAnimation, &QVariantAnimation::valueChanged, [this] { update(); });

    designSystemChangeEvent(nullptr);
}

void Card::setLayoutReimpl(QLayout* _layout) const
{
    Q_ASSERT_X(d->layout->count() == 0, Q_FUNC_INFO, "Widget already contains layout");

    d->layout->addLayout(_layout);
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
                                        d->shadowHeightAnimation.currentValue().toReal());
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

    //
    // Если карточка и так уже выла поднята, нет необходимости делать это повторно
    //
    if (d->shadowHeightAnimation.direction() == QVariantAnimation::Forward
        && d->shadowHeightAnimation.currentValue() == d->shadowHeightAnimation.endValue()) {
        return;
    }

    if (testAttribute(Qt::WA_Hover)) {
        d->shadowHeightAnimation.setDirection(QVariantAnimation::Forward);
        d->shadowHeightAnimation.start();
    }
}

void Card::leaveEvent(QEvent* _event)
{
    Widget::leaveEvent(_event);

    //
    // Если фокус получил один из дочерних виджетов, то не нужно опускать карточку
    //
    if (findChildren<QWidget*>().contains(QApplication::focusWidget())) {
        return;
    }

    if (testAttribute(Qt::WA_Hover)) {
        d->shadowHeightAnimation.setDirection(QVariantAnimation::Backward);
        d->shadowHeightAnimation.start();
    }
}

void Card::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->layout->setContentsMargins(Ui::DesignSystem::card().shadowMargins().toMargins());
}

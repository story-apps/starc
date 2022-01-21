#include "card.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/image_helper.h>

#include <QApplication>
#include <QPainter>
#include <QScreen>
#include <QVBoxLayout>
#include <QVariantAnimation>


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

    shadowHeightAnimation.setEasingCurve(QEasingCurve::OutQuad);
    shadowHeightAnimation.setDuration(160);
}


Card::Card(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);

    Widget::setLayout(d->layout);

    connect(&d->shadowHeightAnimation, &QVariantAnimation::valueChanged, [this] { update(); });

    designSystemChangeEvent(nullptr);
}

Card::~Card() = default;

void Card::setLayout(QLayout* _layout)
{
    Q_ASSERT_X(false, Q_FUNC_INFO, "You should use setLayoutReimpl method");
    Widget::setLayout(_layout);
}

void Card::setLayoutReimpl(QLayout* _layout) const
{
    Q_ASSERT_X(d->layout->count() == 0, Q_FUNC_INFO, "Widget already contains layout");

    d->layout->addLayout(_layout);
}

void Card::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    QRectF backgroundRect
        = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    //
    // Скорректируем геометрию фона, т.к. карточка может занимать несколько экранов
    // в высоту и нет смысла рисовать такой огромный фон
    //
    const auto screenGeometry = QApplication::screenAt(mapToGlobal(pos()))->geometry();
    const auto backgroundGlobalTopLeft = mapToGlobal(backgroundRect.topLeft().toPoint());
    const auto backgroundGlobalTopRight = mapToGlobal(backgroundRect.topLeft().toPoint());
    const auto backgroundGlobalBottomLeft = mapToGlobal(backgroundRect.bottomLeft().toPoint());
    const auto backgroundGlobalBottomRight = mapToGlobal(backgroundRect.bottomRight().toPoint());
    if (backgroundRect.width() > screenGeometry.width()) {
        if (screenGeometry.contains(backgroundGlobalTopLeft)
            || screenGeometry.contains(backgroundGlobalBottomLeft)) {
            backgroundRect.setWidth(screenGeometry.width());
        } else if (screenGeometry.contains(backgroundGlobalTopRight)
                   || screenGeometry.contains(backgroundGlobalBottomRight)) {
            const auto right = backgroundRect.right();
            backgroundRect.setWidth(screenGeometry.width());
            backgroundRect.moveRight(right);
        } else {
            backgroundRect.setWidth(screenGeometry.width());
            backgroundRect.moveLeft(mapFromGlobal(screenGeometry.topLeft()).x());
        }
    }
    if (backgroundRect.height() > screenGeometry.height()) {
        if (screenGeometry.contains(backgroundGlobalTopLeft)
            || screenGeometry.contains(backgroundGlobalTopRight)) {
            backgroundRect.setHeight(screenGeometry.height());
        } else if (screenGeometry.contains(backgroundGlobalBottomLeft)
                   || screenGeometry.contains(backgroundGlobalBottomRight)) {
            const auto bottom = backgroundRect.bottom();
            backgroundRect.setHeight(screenGeometry.height());
            backgroundRect.moveBottom(bottom);
        } else {
            backgroundRect.setHeight(screenGeometry.height());
            backgroundRect.moveTop(mapFromGlobal(screenGeometry.topLeft()).y());
        }
    }

    //
    // Заливаем фон
    //
    QPixmap backgroundImage(backgroundRect.size().toSize());
    backgroundImage.fill(Qt::transparent);
    QPainter backgroundImagePainter(&backgroundImage);
    backgroundImagePainter.setPen(Qt::NoPen);
    backgroundImagePainter.setBrush(backgroundColor());
    const auto borderRadius = Ui::DesignSystem::card().borderRadius();
    backgroundImagePainter.drawRoundedRect(QRect({ 0, 0 }, backgroundImage.size()), borderRadius,
                                           borderRadius);
    //
    // ... рисуем тень
    //
    const auto shadowHeight = std::max(Ui::DesignSystem::card().minimumShadowBlurRadius(),
                                       d->shadowHeightAnimation.currentValue().toReal());
    const auto cacheShadow
        = qFuzzyCompare(shadowHeight, Ui::DesignSystem::card().minimumShadowBlurRadius());
    const auto shadow
        = ImageHelper::dropShadow(backgroundImage, Ui::DesignSystem::card().shadowMargins(),
                                  shadowHeight, Ui::DesignSystem::color().shadow(), cacheShadow);

    QPainter painter(this);
    painter.drawPixmap(
        backgroundRect.marginsAdded(Ui::DesignSystem::card().shadowMargins().toMargins()).topLeft(),
        shadow);
    //
    // ... рисуем сам фон
    //
    painter.setPen(Qt::NoPen);
    painter.setBrush(backgroundColor());
    painter.drawRoundedRect(backgroundRect, borderRadius, borderRadius);
}

void Card::enterEvent(QEnterEvent* _event)
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

    if (rect().contains(mapFromGlobal(QCursor::pos()))) {
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

    d->shadowHeightAnimation.setStartValue(Ui::DesignSystem::card().minimumShadowBlurRadius());
    d->shadowHeightAnimation.setEndValue(Ui::DesignSystem::card().maximumShadowBlurRadius());

    d->layout->setContentsMargins(Ui::DesignSystem::card().shadowMargins().toMargins());
}

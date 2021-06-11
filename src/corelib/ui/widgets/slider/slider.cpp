#include "slider.h"

#include <ui/design_system/design_system.h>

#include <QApplication>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QVariantAnimation>

#include <optional>

class Slider::Implementation
{
public:
    Implementation();

    /**
     * @brief Анимировать клик
     */
    void animateClick();

    /**
     * @brief Вычислить текущее значение
     */
    int calcValue(int _mousePosition, int _trackWidth, bool _isRightToLeft);

    /**
     * @brief Вычислить значение defaultPositionDelta
     */
    void calcDefaultPositionDelta();

    const int minimum = 0;
    int maximum = 100;
    int current = 50;

    std::optional<int> defaultPosition;
    qreal defaultPositionDelta = 0;

    /**
     * @brief  Декорации слайдера при клике
     */
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

Slider::Implementation::Implementation()
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationRadiusAnimation.setDuration(160);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.6);
    decorationOpacityAnimation.setEndValue(0.4);
    decorationOpacityAnimation.setDuration(160);
}

void Slider::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}

int Slider::Implementation::calcValue(int _mousePosition, int _trackWidth, bool _isRightToLeft)
{
    int value = maximum * _mousePosition / _trackWidth;

    return _isRightToLeft ? maximum - value : value;
}

void Slider::Implementation::calcDefaultPositionDelta()
{
    defaultPositionDelta = maximum * 0.01;
}


// ****


Slider::Slider(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            [this] { update(); });

    designSystemChangeEvent(nullptr);
}

void Slider::setMaximumValue(int _maximum)
{
    if (_maximum <= 0 || d->maximum == _maximum) {
        return;
    }

    d->maximum = _maximum;

    if (d->defaultPosition.has_value()) {
        d->calcDefaultPositionDelta();
    }

    update();
}

void Slider::setValue(int _value)
{
    if (d->minimum > _value || _value > d->maximum || d->current == _value) {
        return;
    }

    if (d->defaultPosition.has_value()
        && (_value >= d->defaultPosition.value() - d->defaultPositionDelta)
        && (_value <= d->defaultPosition.value() + d->defaultPositionDelta)) {
        d->current = d->defaultPosition.value();
    } else {
        d->current = _value;
    }

    emit valueChanged(d->current);
    update();
}

void Slider::calcDefaultPosition(int _value)
{
    if (d->minimum > _value || _value > d->maximum) {
        return;
    }

    d->defaultPosition = _value;
    d->calcDefaultPositionDelta();
}

void Slider::resetDefaultPosition()
{
    d->defaultPosition.reset();
}

QSize Slider::sizeHint() const
{
    return QSize(10, Ui::DesignSystem::slider().height());
}

Slider::~Slider() = default;

void Slider::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем полосу слайдера
    //
    // ... слева
    //
    const int leftMargin = contentsRect().left();
    const qreal trackWidth = contentsRect().width();
    const qreal leftTrackWidth = trackWidth * d->current / d->maximum;
    const QRectF leftTrackRect(
        QPointF(isRightToLeft() ? trackWidth - leftTrackWidth + leftMargin : leftMargin,
                (height() - Ui::DesignSystem::slider().trackHeight()) / 2.0),
        QSizeF(leftTrackWidth, Ui::DesignSystem::slider().trackHeight()));
    painter.fillRect(leftTrackRect, Ui::DesignSystem::color().secondary());

    //
    // ... справа
    //
    const QRectF rightTrackRect(isRightToLeft() ? QPointF(leftMargin, leftTrackRect.y())
                                                : leftTrackRect.topRight(),
                                QSizeF(trackWidth - leftTrackWidth, leftTrackRect.height()));
    QColor rightTrackColor = Ui::DesignSystem::color().secondary();
    rightTrackColor.setAlphaF(Ui::DesignSystem::slider().unfilledPartOpacity());
    painter.fillRect(rightTrackRect, rightTrackColor);

    if (d->defaultPosition.has_value()) {
        const qreal startTrackWidth = trackWidth * d->defaultPosition.value() / d->maximum;

        const QPointF startCenter(isRightToLeft() ? trackWidth - startTrackWidth + leftMargin
                                                  : leftMargin + startTrackWidth,
                                  rightTrackRect.center().y());

        const qreal defaultPointRadious = Ui::DesignSystem::slider().thumbRadius() / 2;

        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().secondary());
        painter.drawEllipse(startCenter, defaultPointRadious, defaultPointRadious);
    }

    const QPointF thumbCenter(isRightToLeft() ? rightTrackRect.right() : rightTrackRect.left(),
                              rightTrackRect.center().y());

    //
    // Рисуем hower
    //
    {
        if (underMouse() && !hasFocus()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().secondary());
            painter.setOpacity(Ui::DesignSystem::hoverBackgroundOpacity());
            painter.drawEllipse(thumbCenter, d->decorationRadiusAnimation.endValue().toReal(),
                                d->decorationRadiusAnimation.endValue().toReal());
            painter.setOpacity(1.0);

        } else if (hasFocus()) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().secondary());
            painter.setOpacity(Ui::DesignSystem::focusBackgroundOpacity());
            painter.drawEllipse(thumbCenter, d->decorationRadiusAnimation.endValue().toReal(),
                                d->decorationRadiusAnimation.endValue().toReal());
            painter.setOpacity(1.0);
        }
    }

    //
    // Рисуем декорацию слайдера
    //
    if (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
        || d->decorationOpacityAnimation.state() == QVariantAnimation::Running
        || (underMouse() && !QApplication::mouseButtons().testFlag(Qt::NoButton))) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().secondary());
        painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
        painter.drawEllipse(thumbCenter, d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(1.0);
    }

    //
    // Рисуем пипку
    //
    painter.setPen(Qt::NoPen);
    painter.setBrush(Ui::DesignSystem::color().secondary());
    painter.drawEllipse(thumbCenter, Ui::DesignSystem::slider().thumbRadius(),
                        Ui::DesignSystem::slider().thumbRadius());
}

void Slider::mousePressEvent(QMouseEvent* _event)
{
    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    updateValue(_event->pos());
    d->animateClick();
}

void Slider::mouseMoveEvent(QMouseEvent* _event)
{
    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    updateValue(_event->pos());
}

void Slider::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    update();
}

void Slider::keyPressEvent(QKeyEvent* _event)
{
    switch (_event->key()) {
    case Qt::Key_Left:
        setValue(d->current + (isRightToLeft() ? 1 : -1));
        break;
    case Qt::Key_Right:
        setValue(d->current + (isRightToLeft() ? -1 : 1));
        break;
    default:
        break;
    }
}

void Slider::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->decorationRadiusAnimation.setStartValue(Ui::DesignSystem::slider().thumbRadius() / 2.0);
    d->decorationRadiusAnimation.setEndValue(Ui::DesignSystem::slider().height() / 2.5);

    updateGeometry();
    update();
}

void Slider::updateValue(const QPoint& _mousePosition)
{
    const int trackWidth = contentsRect().width();
    const int mousePosition = _mousePosition.x() - contentsMargins().left();
    const int value = d->calcValue(mousePosition, trackWidth, isRightToLeft());
    setValue(qBound(d->minimum, value, d->maximum));
}

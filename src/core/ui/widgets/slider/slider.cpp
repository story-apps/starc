#include "slider.h"

#include <ui/design_system/design_system.h>

#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPaintEvent>
#include <QVariantAnimation>


class Slider::Implementation
{
public:
    Implementation();

    /**
     * @brief Анимировать клик
     */
    void animateClick();

    const int minimum = 0;
    int maximum = 100;
    int current = 50;

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
    decorationOpacityAnimation.setEndValue(0.2);
    decorationOpacityAnimation.setDuration(160);
}

void Slider::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****


Slider::Slider(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });

    designSystemChangeEvent(nullptr);
}

void Slider::setMaximumValue(int _maximum)
{
    if (_maximum <= 0
        || d->maximum == _maximum) {
        return;
    }

    d->maximum = _maximum;
    update();
}

void Slider::setValue(int _value)
{
    if (d->minimum > _value || _value > d->maximum
        || d->current == _value) {
        return;
    }

    d->current = _value;
    emit valueChanged(d->current);
    update();
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
    const qreal trackWidth = contentsRect().width();
    const qreal leftTrackWidth = trackWidth * d->current / d->maximum;
    const QRectF leftTrackRect(QPointF(contentsMargins().left(),
                                       (height() - Ui::DesignSystem::slider().trackHeight()) / 2.0),
                               QSizeF(leftTrackWidth, Ui::DesignSystem::slider().trackHeight()));
    painter.fillRect(leftTrackRect, Ui::DesignSystem::color().secondary());
    //
    // ... справа
    //
    const QRectF rightTrackRect(leftTrackRect.topRight(),
                                QSizeF(trackWidth - leftTrackWidth, leftTrackRect.height()));
    QColor rightTrackColor = Ui::DesignSystem::color().secondary();
    rightTrackColor.setAlphaF(0.24);
    painter.fillRect(rightTrackRect, rightTrackColor);

    //
    // Рисуем декорацию слайдера
    //
    const QPointF thumbCenter(rightTrackRect.left(), rightTrackRect.center().y());
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
    painter.drawEllipse(thumbCenter,
                        Ui::DesignSystem::slider().thumbRadius(),
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
    const int value = d->maximum * mousePosition / trackWidth;
    setValue(qBound(d->minimum, value, d->maximum));
}

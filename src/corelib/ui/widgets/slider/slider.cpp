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

    /**
     * @brief Анимировать тень
     */
    void animateHoverIn();
    void animateHoverOut();

    const int minimum = 0;
    int maximum = 100;
    int current = 50;

    /**
     * @brief  Декорации слайдера при клике
     */
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;

    QVariantAnimation howerRadiusAnimation;
    QVariantAnimation howerOpacityAnimation;
};

Slider::Implementation::Implementation()
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationRadiusAnimation.setDuration(160);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.6);
    decorationOpacityAnimation.setEndValue(0.4);
    decorationOpacityAnimation.setDuration(160);

    howerRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    howerRadiusAnimation.setDuration(160);

    howerOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    howerOpacityAnimation.setStartValue(0.0);
    howerOpacityAnimation.setEndValue(0.2);
    howerOpacityAnimation.setDuration(160);
}

void Slider::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}

void Slider::Implementation::animateHoverIn()
{
    howerOpacityAnimation.setDirection(QVariantAnimation::Forward);
    howerRadiusAnimation.setDirection(QVariantAnimation::Forward);

    howerRadiusAnimation.start();
    howerOpacityAnimation.start();
}

void Slider::Implementation::animateHoverOut()
{
    howerOpacityAnimation.setDirection(QVariantAnimation::Backward);
    howerRadiusAnimation.setDirection(QVariantAnimation::Backward);

    howerRadiusAnimation.start();
    howerOpacityAnimation.start();
}


// ****


Slider::Slider(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->howerRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->howerOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });

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
    const QRectF leftTrackRect(QPointF((isRightToLeft()? contentsRect().width() + contentsMargins().left() : contentsMargins().left()),
                                       (height() - Ui::DesignSystem::slider().trackHeight()) / 2.0),
                               QSizeF((isRightToLeft()? -1 : 1) * leftTrackWidth, Ui::DesignSystem::slider().trackHeight()));
    painter.fillRect(leftTrackRect, Ui::DesignSystem::color().secondary());

    //
    // ... справа
    //
    const QRectF rightTrackRect(leftTrackRect.topRight(),
                                QSizeF((isRightToLeft()? -1 : 1) * (trackWidth - leftTrackWidth), leftTrackRect.height()));
    QColor rightTrackColor = Ui::DesignSystem::color().secondary();
    rightTrackColor.setAlphaF(Ui::DesignSystem::slider().unfilledPartOpacity());
    painter.fillRect(rightTrackRect, rightTrackColor);

    const QPointF thumbCenter(rightTrackRect.left(), rightTrackRect.center().y());

    //
    // Рисуем hower
    //
    {
        if (underMouse() && !hasFocus()){
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().secondary());

            if ((d->howerOpacityAnimation.state() == QVariantAnimation::Running
                 ||d->howerRadiusAnimation.state() == QVariantAnimation::Running)){
                painter.setOpacity(d->howerOpacityAnimation.currentValue().toReal());
                painter.drawEllipse(thumbCenter, d->howerRadiusAnimation.currentValue().toReal(),
                                    d->howerRadiusAnimation.currentValue().toReal());
            } else{
                painter.setOpacity(d->howerOpacityAnimation.endValue().toReal());
                painter.drawEllipse(thumbCenter, d->howerRadiusAnimation.endValue().toReal(),
                                    d->howerRadiusAnimation.endValue().toReal());
            }

            painter.setOpacity(1.0);

        } else if (d->howerOpacityAnimation.state() == QVariantAnimation::Running
                   ||d->howerRadiusAnimation.state() == QVariantAnimation::Running){
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().secondary());

            painter.setOpacity(d->howerOpacityAnimation.currentValue().toReal());

            painter.drawEllipse(thumbCenter, d->howerRadiusAnimation.currentValue().toReal(),
                                d->howerRadiusAnimation.currentValue().toReal());
            painter.setOpacity(1.0);
        }else if (hasFocus()){
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().secondary());
            painter.setOpacity(d->howerOpacityAnimation.endValue().toReal() + 0.1);
            painter.drawEllipse(thumbCenter, d->howerRadiusAnimation.endValue().toReal(),
                                d->howerRadiusAnimation.endValue().toReal());
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
    painter.drawEllipse(thumbCenter,
                        Ui::DesignSystem::slider().thumbRadius(),
                        Ui::DesignSystem::slider().thumbRadius());
}

void Slider::enterEvent(QEvent *_event)
{
    Q_UNUSED(_event)
    if (hasFocus()) return;

    d->animateHoverIn();
}

void Slider::leaveEvent(QEvent *_event)
{
    Q_UNUSED(_event);
    if (hasFocus()) return;

    d->animateHoverOut();
}

void Slider::mousePressEvent(QMouseEvent* _event)
{
    if (_event->buttons().testFlag(Qt::NoButton)) {
        return;
    }

    clearFocus();
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

void Slider::keyPressEvent(QKeyEvent *_event)
{
    switch (_event->key()) {
    case Qt::Key_Left:
        setValue(d->current + (isRightToLeft()? 1: -1));
        break;
    case Qt::Key_Right:
        setValue(d->current + (isRightToLeft()? -1: 1));
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

    d->howerRadiusAnimation.setStartValue(Ui::DesignSystem::slider().thumbRadius() / 2.0);
    d->howerRadiusAnimation.setEndValue(Ui::DesignSystem::slider().height() / 2.5);

    updateGeometry();
    update();
}

void Slider::updateValue(const QPoint& _mousePosition)
{
    const int trackWidth = contentsRect().width();
    const int mousePosition = _mousePosition.x() - contentsMargins().left();
    const int value = calcValue(mousePosition, trackWidth);
    setValue(qBound(d->minimum, value, d->maximum));
}

int Slider::calcValue(int _mousePosition, int _trackWidth)
{
    int value = d->maximum * _mousePosition / _trackWidth;

    return isRightToLeft()? d->maximum - value : value;
}

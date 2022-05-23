#include "click_animation.h"

#include <QRectF>
#include <QVariantAnimation>


class ClickAnimation::Implementation
{
public:
    /**
     * @brief Позиция клика
     */
    QPointF clickPosition;

    /**
     * @brief Доступная область для отображения
     */
    QRectF clipRect;

    /**
     * @brief Радиус декорации
     */
    QVariantAnimation radius;

    /**
     * @brief Прозрачность декорации
     */
    QVariantAnimation opacity;
};

ClickAnimation::ClickAnimation(QObject* _parent)
    : QParallelAnimationGroup(_parent)
    , d(new Implementation)
{
    d->radius.setEasingCurve(QEasingCurve::InOutQuad);
    addAnimation(&d->opacity);

    d->opacity.setEasingCurve(QEasingCurve::InQuad);
    d->opacity.setStartValue(0.5);
    d->opacity.setEndValue(0.0);
    addAnimation(&d->radius);

    setFast(true);

    connect(&d->radius, &QVariantAnimation::valueChanged, this, &ClickAnimation::valueChanged);
    connect(&d->opacity, &QVariantAnimation::valueChanged, this, &ClickAnimation::valueChanged);
}

ClickAnimation::~ClickAnimation() = default;

void ClickAnimation::setFast(bool _fast)
{
    if (_fast) {
        d->radius.setDuration(160);
        d->opacity.setDuration(160);
    } else {
        d->radius.setDuration(240);
        d->opacity.setDuration(420);
    }
}

void ClickAnimation::setRadiusInterval(qreal _from, qreal _to)
{
    d->radius.setStartValue(_from);
    d->radius.setEndValue(_to);
}

void ClickAnimation::setClickPosition(const QPointF& _position)
{
    d->clickPosition = _position;
}

QPointF ClickAnimation::clickPosition() const
{
    return d->clickPosition;
}

void ClickAnimation::setClipRect(const QRectF& _rect)
{
    d->clipRect = _rect;
}

QRectF ClickAnimation::clipRect() const
{
    return d->clipRect;
}

qreal ClickAnimation::radius() const
{
    return d->radius.currentValue().isValid() ? d->radius.currentValue().toReal()
                                              : d->radius.startValue().toReal();
}

qreal ClickAnimation::minimumRadius() const
{
    return d->radius.startValue().toReal();
}

qreal ClickAnimation::maximumRadius() const
{
    return d->radius.endValue().toReal();
}

qreal ClickAnimation::opacity() const
{
    return d->opacity.currentValue().isValid() ? d->opacity.currentValue().toReal()
                                               : d->opacity.startValue().toReal();
}

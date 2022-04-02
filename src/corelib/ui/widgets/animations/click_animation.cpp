#include "click_animation.h"

#include <QVariantAnimation>


class ClickAnimation::Implementation
{
public:
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
    d->radius.setDuration(160);
    addAnimation(&d->opacity);

    d->opacity.setEasingCurve(QEasingCurve::InQuad);
    d->opacity.setStartValue(0.5);
    d->opacity.setEndValue(0.0);
    d->opacity.setDuration(160);
    addAnimation(&d->radius);

    connect(&d->radius, &QVariantAnimation::valueChanged, this, &ClickAnimation::valueChanged);
    connect(&d->opacity, &QVariantAnimation::valueChanged, this, &ClickAnimation::valueChanged);
}

ClickAnimation::~ClickAnimation() = default;

void ClickAnimation::setRadiusInterval(qreal _from, qreal _to)
{
    d->radius.setStartValue(_from);
    d->radius.setEndValue(_to);
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

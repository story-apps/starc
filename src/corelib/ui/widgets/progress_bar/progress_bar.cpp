#include "progress_bar.h"

#include <ui/design_system/design_system.h>

#include <QPaintEvent>
#include <QPainter>
#include <QVariantAnimation>


class ProgressBar::Implementation
{
public:
    Implementation();

    qreal progress = 0;
    QVariantAnimation progressAnimation;
};

ProgressBar::Implementation::Implementation()
{
    progressAnimation.setDuration(180);
    progressAnimation.setEasingCurve(QEasingCurve::OutQuad);
}


ProgressBar::ProgressBar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    connect(&d->progressAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                d->progress = _value.toReal();
                update();
            });
}

ProgressBar::~ProgressBar() = default;

qreal ProgressBar::progress() const
{
    return d->progress;
}

void ProgressBar::setProgress(qreal _progress)
{
    if (qFuzzyCompare(d->progress, _progress)) {
        return;
    }

    d->progressAnimation.setStartValue(d->progress);
    d->progressAnimation.setEndValue(_progress);
    d->progressAnimation.start();
}

QSize ProgressBar::sizeHint() const
{
    return QSize(100, Ui::DesignSystem::layout().px4());
}

void ProgressBar::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.fillRect(_event->rect(), backgroundColor());

    const QRectF progressRect(0, 0, width() * d->progress, height());
    painter.fillRect(progressRect, Ui::DesignSystem::color().secondary());
}

#include "progress_bar.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

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
    if (_progress < 0 || _progress > 1.0) {
        return;
    }

    if (qFuzzyCompare(d->progress, _progress)) {
        return;
    }

    if (!isVisible()) {
        d->progressAnimation.setEndValue(_progress);
        d->progressAnimation.setCurrentTime(d->progressAnimation.duration());
        return;
    }

    if (d->progressAnimation.state() == QVariantAnimation::Stopped) {
        d->progressAnimation.setStartValue(d->progress);
        d->progressAnimation.setEndValue(_progress);
        d->progressAnimation.start();
    } else {
        d->progressAnimation.pause();
        d->progressAnimation.setEndValue(_progress);
        d->progressAnimation.resume();
    }
}

QSize ProgressBar::sizeHint() const
{
    return QSize(100, Ui::DesignSystem::layout().px4());
}

void ProgressBar::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.fillRect(_event->rect(), backgroundColor());

    painter.fillRect(contentsRect(),
                     ColorHelper::transparent(Ui::DesignSystem::color().secondary(),
                                              Ui::DesignSystem::hoverBackgroundOpacity()));
    const QRectF progressRect(
        contentsRect().x()
            + (isLeftToRight() ? 0 : contentsRect().width() - contentsRect().width() * d->progress),
        contentsRect().y(), contentsRect().width() * d->progress, contentsRect().height());
    painter.fillRect(progressRect, Ui::DesignSystem::color().secondary());
}

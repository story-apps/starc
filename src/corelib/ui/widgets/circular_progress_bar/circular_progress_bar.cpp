#include "circular_progress_bar.h"

#include <ui/design_system/design_system.h>

#include <QPaintEvent>
#include <QPainter>
#include <QVariantAnimation>


class CircularProgressBar::Implementation
{
public:
    Implementation();

    QColor barColor;
    QString text;
    qreal progress = 0;
    QVariantAnimation progressAnimation;
};

CircularProgressBar::Implementation::Implementation()
{
    progressAnimation.setDuration(180);
    progressAnimation.setEasingCurve(QEasingCurve::OutQuad);
}


// ****


CircularProgressBar::CircularProgressBar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    connect(&d->progressAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                d->progress = _value.toReal();
                update();
            });
}

CircularProgressBar::~CircularProgressBar() = default;

void CircularProgressBar::setBarColor(const QColor& _color)
{
    if (d->barColor == _color) {
        return;
    }

    d->barColor = _color;
    update();
}

void CircularProgressBar::setProgress(qreal _progress)
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
        d->progress = _progress;
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

void CircularProgressBar::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    update();
}

QSize CircularProgressBar::sizeHint() const
{
    return QSize(
        contentsMargins().left() + Ui::DesignSystem::layout().px24() + contentsMargins().right(),
        contentsMargins().top() + Ui::DesignSystem::layout().px24() + contentsMargins().bottom());
}

void CircularProgressBar::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Рисуем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем прогресс
    //

    const qreal minSideLength = qMin(contentsRect().width(), contentsRect().height())
        - Ui::DesignSystem::progressBar().circularTrackHeight();
    const QRectF rectangle(contentsRect().left() + (contentsRect().width() - minSideLength) / 2,
                           contentsRect().top() + (contentsRect().height() - minSideLength) / 2,
                           minSideLength, minSideLength);

    QPen pen;
    pen.setColor(d->barColor.isValid() ? d->barColor : textColor());
    pen.setWidthF(Ui::DesignSystem::progressBar().circularTrackHeight());
    painter.setPen(pen);

    //
    // Фон
    //
    int startAngle = 90 * 16;
    int spanAngle = 360 * 360 * 16;
    //    painter.setOpacity(Ui::DesignSystem::progressBar().unfilledPartOpacity());
    //    painter.drawArc(rectangle, startAngle, spanAngle);

    //
    // Заполнение
    //
    painter.setOpacity(1.0);
    startAngle = 90 * 16;
    qreal valueCorrected = d->progress;
    while (valueCorrected > 1.0) {
        valueCorrected -= 1.0;
    }
    spanAngle = static_cast<int>(valueCorrected * 360 * 16);
    painter.drawArc(rectangle, startAngle, spanAngle);

    //
    // Текст
    //
    if (!d->text.isEmpty()) {
        pen.setColor(textColor());
        painter.setPen(pen);
        painter.setFont(Ui::DesignSystem::font().button());
        painter.drawText(rectangle, Qt::AlignCenter, d->text);
    }
}

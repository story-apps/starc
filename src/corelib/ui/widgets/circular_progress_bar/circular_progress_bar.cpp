#include "circular_progress_bar.h"

#include <ui/design_system/design_system.h>

#include <QPaintEvent>
#include <QPainter>


class CircularProgressBar::Implementation
{
public:
    QColor barColor;
    qreal value = 0.17;
    QString text;
};

CircularProgressBar::CircularProgressBar(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
}

void CircularProgressBar::setBarColor(const QColor& _color)
{
    if (d->barColor == _color) {
        return;
    }

    d->barColor = _color;
    update();
}

CircularProgressBar::~CircularProgressBar() = default;

void CircularProgressBar::setValue(qreal _value)
{
    if (qFuzzyCompare(d->value, _value)) {
        return;
    }

    d->value = _value;
    update();
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
    return QSize(width(), width());
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
    pen.setColor(d->barColor);
    pen.setWidthF(Ui::DesignSystem::progressBar().circularTrackHeight());
    painter.setPen(pen);

    //
    // Фон
    //
    painter.setOpacity(Ui::DesignSystem::progressBar().unfilledPartOpacity());
    int startAngle = 90 * 16;
    int spanAngle = 360 * 360 * 16;
    painter.drawArc(rectangle, startAngle, spanAngle);

    //
    // Заполнение
    //
    painter.setOpacity(1.0);
    startAngle = 90 * 16;
    qreal valueCorrected = d->value;
    while (valueCorrected > 1.0) {
        valueCorrected -= 1.0;
    }
    spanAngle = static_cast<int>(valueCorrected * 360 * 16);
    painter.drawArc(rectangle, startAngle, spanAngle);

    //
    // Текст
    //
    pen.setColor(textColor());
    painter.setPen(pen);
    painter.setFont(Ui::DesignSystem::font().button());
    painter.drawText(rectangle, Qt::AlignCenter,
                     d->text.isEmpty() ? QString::number(d->value * 100, 'f', 1) + "%" : d->text);
}

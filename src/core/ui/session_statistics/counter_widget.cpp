#include "counter_widget.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>

#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>


class CounterWidget::Implementation
{
public:
    ValueStyle valueStyle = ValueStyle::OneLine;

    QColor color;
    QString label;
    QString value;
};


// ****


CounterWidget::CounterWidget(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
}

CounterWidget::~CounterWidget() = default;

void CounterWidget::setValueStyle(ValueStyle _style)
{
    if (d->valueStyle == _style) {
        return;
    }

    d->valueStyle = _style;
    update();
}

void CounterWidget::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    update();
}

void CounterWidget::setLabel(const QString& _label)
{
    if (d->label == _label) {
        return;
    }

    d->label = _label;
    updateGeometry();
    update();
}

void CounterWidget::setValue(const QString& _value)
{
    if (d->value == _value) {
        return;
    }

    d->value = _value;
    updateGeometry();
    update();
}

QSize CounterWidget::sizeHint() const
{
    return QSize(
        contentsMargins().left() + Ui::DesignSystem::layout().px(58) + contentsMargins().right(),
        contentsMargins().top() + Ui::DesignSystem::layout().px62() + contentsMargins().bottom());
}

void CounterWidget::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Закрашиваем фон прозрачным
    //
    painter.fillRect(_event->rect(), Qt::transparent);

    //
    // Будем рисовать в области со скруглёнными краями
    //
    QPainterPath clipPath;
    clipPath.addRoundedRect(contentsRect(), Ui::DesignSystem::card().borderRadius(),
                            Ui::DesignSystem::card().borderRadius());
    painter.setClipPath(clipPath);
    painter.translate(contentsRect().topLeft());
    //
    auto backgroundRect = contentsRect();
    backgroundRect.moveTopLeft({ 0, 0 });

    //
    // Цвет фона самого счётчика
    //
    painter.fillRect(backgroundRect, backgroundColor());

    //
    // Цвет счётчика
    //
    const QRectF colorRect(0, 0, Ui::DesignSystem::layout().px4(), backgroundRect.height());
    painter.fillRect(colorRect, d->color);

    //
    // Лейбл
    //
    const auto margin = Ui::DesignSystem::layout().px(30);
    const QRectF labelRect(colorRect.width() + margin, 0,
                           backgroundRect.width() - colorRect.width() - margin,
                           backgroundRect.height());
    painter.setPen(ColorHelper::transparent(textColor(), Ui::DesignSystem::inactiveTextOpacity()));
    painter.setFont(Ui::DesignSystem::font().caption());
    painter.drawText(labelRect, Qt::AlignLeft | Qt::AlignVCenter, d->label);

    //
    // Значение
    //
    const QRectF counterRect(0, 0, backgroundRect.width() - margin, backgroundRect.height());
    painter.setPen(textColor());
    painter.setFont(d->valueStyle == ValueStyle::OneLine ? Ui::DesignSystem::font().h5()
                                                         : Ui::DesignSystem::font().subtitle2());
    painter.drawText(counterRect, Qt::AlignRight | Qt::AlignVCenter, d->value);
}

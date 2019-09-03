#include "widget.h"

#include <custom_events.h>

#include <QPaintEvent>
#include <QPainter>


class Widget::Implementation
{
public:
    QColor backgroundColor = Qt::red;
    QColor textColor = Qt::red;
};


// ****


Widget::Widget(QWidget *_parent)
    : QWidget(_parent),
      d(new Implementation)
{
}

Widget::~Widget() = default;

QColor Widget::backgroundColor() const
{
    return d->backgroundColor;
}

void Widget::setBackgroundColor(const QColor& _color)
{
    if (d->backgroundColor == _color) {
        return;
    }

    d->backgroundColor = _color;
    update();
}

QColor Widget::textColor() const
{
    return d->textColor;
}

void Widget::setTextColor(const QColor& _color)
{
    if (d->textColor == _color) {
        return;
    }

    d->textColor = _color;
    update();
}

bool Widget::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
        case static_cast<int>(EventType::DesignSystemChangeEvent): {
            DesignSystemChangeEvent* event = static_cast<DesignSystemChangeEvent*>(_event);
            designSysemChangeEvent(event);
            return false;
        }

        default: {
            return QWidget::event(_event);
        }
    }
}

void Widget::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.fillRect(_event->rect(), d->backgroundColor);
}

void Widget::designSysemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);
    update();
}

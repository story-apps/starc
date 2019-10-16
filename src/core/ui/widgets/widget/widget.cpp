#include "widget.h"

#include <custom_events.h>

#include <QEvent>
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
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground);
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

void Widget::setVisible(bool _visible)
{
    QWidget::setVisible(_visible);

    if (_visible) {
        emit appeared();
    } else {
        emit disappeared();
    }
}

bool Widget::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
        case static_cast<int>(EventType::DesignSystemChangeEvent): {
            DesignSystemChangeEvent* event = static_cast<DesignSystemChangeEvent*>(_event);
            designSystemChangeEvent(event);
            return false;
        }

        default: {
            return QWidget::event(_event);
        }
    }
}

void Widget::changeEvent(QEvent* _event)
{
    switch (_event->type()) {
        case QEvent::LanguageChange: {
            updateTranslations();
            break;
        }

        default: {
            QWidget::changeEvent(_event);
            break;
        }
    }
}

void Widget::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.fillRect(_event->rect(), d->backgroundColor);
}

void Widget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    updateGeometry();
    update();
}

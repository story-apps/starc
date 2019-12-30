#include "splitter_handle.h"

#include <QPaintEvent>
#include <QPainter>


class SplitterHandle::Implementation
{
public:
    QColor backgroundColor = Qt::red;
};


// ****


SplitterHandle::SplitterHandle(Qt::Orientation _orientation, QSplitter* _parent)
    : QSplitterHandle(_orientation, _parent),
      d(new Implementation)
{
    setAutoFillBackground(false);
    setAttribute(Qt::WA_NoSystemBackground);
}

SplitterHandle::~SplitterHandle() = default;

QColor SplitterHandle::backgroundColor() const
{
    return d->backgroundColor;
}

void SplitterHandle::setBackgroundColor(const QColor& _color)
{
    if (d->backgroundColor == _color) {
        return;
    }

    d->backgroundColor = _color;
    update();
}

void SplitterHandle::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.fillRect(_event->rect(), d->backgroundColor);
}

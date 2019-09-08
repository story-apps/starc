#include "splitter.h"

#include "splitter_handle.h"


class Splitter::Implementation
{
public:
    QColor handleColor = Qt::red;
};


// ****


Splitter::Splitter(QWidget* _parent)
    : QSplitter(_parent),
      d(new Implementation)
{
}

Splitter::~Splitter() = default;

void Splitter::setHandleColor(const QColor& _color)
{
    if (d->handleColor == _color) {
        return;
    }

    d->handleColor = _color;
    for (int handleIndex = 0; handleIndex < count(); ++handleIndex) {
        auto handle = qobject_cast<SplitterHandle*>(this->handle(handleIndex));
        if (handle == nullptr) {
            continue;
        }

        handle->setBackgroundColor(d->handleColor);
    }

    update();
}

QSplitterHandle* Splitter::createHandle()
{
    SplitterHandle* handle = new SplitterHandle(orientation(), this);
    handle->setBackgroundColor(d->handleColor);
    return handle;
}

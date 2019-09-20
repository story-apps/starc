#include "dialog_content.h"

#include <QMoveEvent>
#include <QResizeEvent>
#include <QVariantAnimation>


class DialogContent::Implementation
{
public:
    Implementation();

    QVariantAnimation sizeAnimation;
    QVariantAnimation moveAnimation;
};

DialogContent::Implementation::Implementation()
{
    sizeAnimation.setDuration(160);
    sizeAnimation.setEasingCurve(QEasingCurve::OutQuad);
    moveAnimation.setDuration(160);
    moveAnimation.setEasingCurve(QEasingCurve::OutQuad);
}


// ****


DialogContent::DialogContent(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    connect(&d->sizeAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        resize(_value.toSize());
    });
    connect(&d->moveAnimation, &QVariantAnimation::valueChanged, this, [this] (const QVariant& _value) {
        move(_value.toPoint());
    });
}

DialogContent::~DialogContent() = default;

void DialogContent::resizeEvent(QResizeEvent* _event)
{
    //
    // Если мы в процессе выполнения анимации, то меняем размер
    //
    if (!isVisible()
        || d->sizeAnimation.state() == QVariantAnimation::Running) {
        Widget::resizeEvent(_event);
    }
    //
    // А если это событие пришло от компоновщика, то запускаем анимацию
    //
    else {
        d->sizeAnimation.blockSignals(true);
        d->sizeAnimation.setStartValue(_event->oldSize());
        d->sizeAnimation.setEndValue(_event->size());
        d->sizeAnimation.blockSignals(false);
        d->sizeAnimation.start();
        resize(_event->oldSize());
    }
}

void DialogContent::moveEvent(QMoveEvent* _event)
{
    //
    // Если мы в процессе выполнения анимации, то меняем размер
    //
    if (!isVisible()
        || d->moveAnimation.state() == QVariantAnimation::Running) {
        Widget::moveEvent(_event);
    }
    //
    // А если это событие пришло от компоновщика, то запускаем анимацию
    //
    else {
        d->moveAnimation.blockSignals(true);
        d->moveAnimation.setStartValue(_event->oldPos());
        d->moveAnimation.setEndValue(_event->pos());
        d->moveAnimation.blockSignals(false);
        d->moveAnimation.start();
        move(_event->oldPos());
    }
}

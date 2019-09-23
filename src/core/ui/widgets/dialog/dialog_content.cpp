#include "dialog_content.h"

#include <QMoveEvent>
#include <QResizeEvent>
#include <QTimer>
#include <QVariantAnimation>

#include <QDebug>
class DialogContent::Implementation
{
public:
    Implementation();

    QTimer sizeAnimationStartTimer;
    QTimer moveAnimationStartTimer;
    QVariantAnimation sizeAnimation;
    QVariantAnimation moveAnimation;
};

DialogContent::Implementation::Implementation()
{
    sizeAnimationStartTimer.setSingleShot(true);
    sizeAnimationStartTimer.setInterval(0);
    moveAnimationStartTimer.setSingleShot(true);
    moveAnimationStartTimer.setInterval(0);
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
    connect(&d->sizeAnimationStartTimer, &QTimer::timeout, this, [this] { d->sizeAnimation.start(); });
    connect(&d->moveAnimationStartTimer, &QTimer::timeout, this, [this] { d->moveAnimation.start(); });
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
    if (!isVisible()
        || !_event->oldSize().isValid()
        || d->sizeAnimation.state() == QVariantAnimation::Running) {
        return;
    }

    QSignalBlocker animationSignalBlocker(d->sizeAnimation);

    //
    // Если старт анимации ещё не запланирован
    //
    if (!d->sizeAnimationStartTimer.isActive()) {
        //
        // ... запланируем
        //
        d->sizeAnimation.setStartValue(_event->oldSize());
        d->sizeAnimation.setEndValue(_event->size());
        d->sizeAnimationStartTimer.start();
        //
        // ... и сбросим текущий размер
        //
        resize(_event->oldSize());
    }
    //
    // Если старт анимации уже запланирован
    //
    else {
        //
        // ... и это пришло событие, которое мы сами запустили, чтобы размер виджета пока не дёргался
        //
        if (d->sizeAnimation.startValue().toSize() == _event->size()
            && d->sizeAnimation.endValue().toSize() == _event->oldSize()) {
            //
            // ... ничего не делаем
            //
        }
        //
        // ... а если же это событие от системы, например нужно показать ещё виджеты на форме
        //
        else {
            //
            // ... корректируем финальный размер и перезапланируем изменение
            //
            d->sizeAnimation.setEndValue(_event->size());
            d->sizeAnimationStartTimer.start();
            //
            // ... а затем опять сбросим текущий размер
            //
            resize(_event->oldSize());
        }
    }
}

void DialogContent::moveEvent(QMoveEvent* _event)
{
    if (!isVisible()
        || _event->oldPos().isNull()
        || d->moveAnimation.state() == QVariantAnimation::Running) {
        return;
    }

    QSignalBlocker animationSignalBlocker(d->moveAnimation);

    //
    // Если старт анимации ещё не запланирован
    //
    if (!d->moveAnimationStartTimer.isActive()) {
        //
        // ... запланируем
        //
        d->moveAnimation.setStartValue(_event->oldPos());
        d->moveAnimation.setEndValue(_event->pos());
        d->moveAnimationStartTimer.start();
        //
        // ... и сбросим текущий размер
        //
        move(_event->oldPos());
    }
    //
    // Если старт анимации уже запланирован
    //
    else {
        //
        // ... и это пришло событие, которое мы сами запустили, чтобы размер виджета пока не дёргался
        //
        if (d->moveAnimation.startValue().toPoint() == _event->pos()
            && d->moveAnimation.endValue().toPoint() == _event->oldPos()) {
            //
            // ... ничего не делаем
            //
        }
        //
        // ... а если же это событие от системы, например нужно показать ещё виджеты на форме
        //
        else {
            //
            // ... корректируем финальный размер и перезапланируем изменение
            //
            d->moveAnimation.setEndValue(_event->pos());
            d->moveAnimationStartTimer.start();
            //
            // ... а затем опять сбросим текущий размер
            //
            move(_event->oldPos());
        }
    }
}

#include "stack_widget.h"

#include <QPainter>
#include <QResizeEvent>
#include <QVariantAnimation>


class StackWidget::Implementation
{
public:
    explicit Implementation();

    QVector<QWidget *> widgets;
    QWidget *previousWidget = nullptr;
    QWidget *currentWidget = nullptr;

    QPixmap previousWidgetImage;
    QPixmap currentWidgetImage;
    QVariantAnimation fadeAnimation;
};

StackWidget::Implementation::Implementation()
{
    fadeAnimation.setDuration(240);
    fadeAnimation.setEasingCurve(QEasingCurve::OutQuad);
    fadeAnimation.setStartValue(0.0);
    fadeAnimation.setEndValue(1.0);
}

// ****

StackWidget::StackWidget(QWidget *_parent)
    : Widget(_parent)
    , d(new Implementation)
{
    connect(&d->fadeAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->fadeAnimation, &QVariantAnimation::finished, this, [this] {
        d->currentWidget->show();
    });
}

StackWidget::~StackWidget() = default;

void StackWidget::setCurrentWidget(QWidget *widget)
{
    if (d->currentWidget == widget) {
        return;
    }

    //
    // Сделать снимок текущего виджета
    //
    if (d->currentWidget != nullptr) {
        d->previousWidget = d->currentWidget;
        d->previousWidgetImage = d->previousWidget->grab();
        d->previousWidget->hide();
    }

    //
    // Устанавливаем новый виджет в качестве текущего
    //
    d->currentWidget = widget;
    if (!d->widgets.contains(d->currentWidget)) {
        d->currentWidget->setParent(this);
        d->currentWidget->resize(size());
        d->widgets.append(d->currentWidget);
    }
    d->currentWidgetImage = d->currentWidget->grab();
    d->currentWidget->hide();

    //
    // Если виджет виден на экране, запускаем анимацию отображения нового текущего виджета
    //
    if (isVisible()) {
        d->fadeAnimation.start();
    }
    //
    // В противном случае, просто отображаем новый текущий виджет
    //
    else {
        d->currentWidget->show();
    }
}

void StackWidget::paintEvent(QPaintEvent *_event)
{
    Widget::paintEvent(_event);

    //
    // Если анимация закончена, ничего дополнительного рисовать не нужно
    //
    if (d->fadeAnimation.state() != QAbstractAnimation::Running) {
        return;
    }

    //
    // Если анимация в процессе, рисуем изображения виджетов
    //
    QPainter painter(this);
    if (!d->previousWidgetImage.isNull()) {
        painter.drawPixmap(0, 0, d->previousWidgetImage);
    }
    if (!d->currentWidgetImage.isNull()) {
        painter.setOpacity(d->fadeAnimation.currentValue().toReal());
        painter.drawPixmap(0, 0, d->currentWidgetImage);
    }
}

void StackWidget::resizeEvent(QResizeEvent *_event)
{
    if (d->currentWidget == nullptr) {
        return;
    }

    if (d->currentWidget->size() == _event->size()) {
        return;
    }

    d->currentWidget->resize(_event->size());

    //
    // Если изменение размера виджета происходит в момент анимации,
    // корректируем размер сграбленных изображений
    //
    if (d->fadeAnimation.state() == QAbstractAnimation::Running) {
        if (d->previousWidget != nullptr) {
            d->previousWidgetImage = d->previousWidget->grab(QRect({}, _event->size()));
        }
        d->currentWidgetImage = d->currentWidget->grab();
    }
}

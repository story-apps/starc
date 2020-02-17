#include "stack_widget.h"

#include <QPainter>
#include <QResizeEvent>
#include <QVariantAnimation>


class StackWidget::Implementation
{
public:
    explicit Implementation();

    const QAbstractAnimation& currentAnimation() const;


    QVector<QWidget *> widgets;
    QWidget *previousWidget = nullptr;
    QWidget *currentWidget = nullptr;

    QPixmap previousWidgetImage;
    QPixmap currentWidgetImage;

    AnimationType animationType = AnimationType::Fade;
    QVariantAnimation fadeAnimation;
    QVariantAnimation slideAnimation;
};

StackWidget::Implementation::Implementation()
{
    fadeAnimation.setDuration(240);
    fadeAnimation.setEasingCurve(QEasingCurve::OutQuad);
    fadeAnimation.setStartValue(0.0);
    fadeAnimation.setEndValue(1.0);

    slideAnimation.setDuration(240);
    slideAnimation.setEasingCurve(QEasingCurve::InOutQuad);
}

const QAbstractAnimation& StackWidget::Implementation::currentAnimation() const
{
    switch (animationType) {
        default:
        case AnimationType::Fade: {
            return fadeAnimation;
        }

        case AnimationType::Slide: {
            return slideAnimation;
        }
    }
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
    connect(&d->slideAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->slideAnimation, &QVariantAnimation::finished, this, [this] {
        d->currentWidget->show();
    });
}

void StackWidget::setAnimationType(StackWidget::AnimationType _type)
{
    d->animationType = _type;
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
        d->widgets.append(d->currentWidget);
    }
    d->currentWidget->setParent(this);
    d->currentWidget->resize(size());
    d->currentWidgetImage = d->currentWidget->grab();
    d->currentWidget->hide();

    //
    // Если виджет не виден на экране, просто отображаем новый текущий виджет
    //
    if (!isVisible()) {
        d->currentWidget->show();
        return;
    }

    //
    // А если виджет виден, то запускаем анимацию отображения нового текущего виджета
    //
    switch (d->animationType) {
        case AnimationType::Fade: {
            d->fadeAnimation.start();
            break;
        }

        case AnimationType::Slide: {
            const int previousIndex = d->widgets.indexOf(d->previousWidget);
            const int currentIndex = d->widgets.indexOf(d->currentWidget);
            if (currentIndex > previousIndex) {
                d->slideAnimation.setStartValue(width());
                d->slideAnimation.setEndValue(0);
            } else {
                d->slideAnimation.setStartValue(-width());
                d->slideAnimation.setEndValue(0);
            }
            d->slideAnimation.start();
            break;
        }
    }
}

QWidget* StackWidget::currentWidget() const
{
    return d->currentWidget;
}

void StackWidget::paintEvent(QPaintEvent *_event)
{
    Widget::paintEvent(_event);

    //
    // Если анимация закончена, ничего дополнительного рисовать не нужно
    //
    if (d->currentAnimation().state() != QAbstractAnimation::Running) {
        return;
    }

    //
    // Если анимация в процессе, рисуем изображения виджетов
    //
    QPainter painter(this);
    switch (d->animationType) {
        case AnimationType::Fade: {
            if (!d->previousWidgetImage.isNull()) {
                painter.drawPixmap(0, 0, d->previousWidgetImage);
            }
            if (!d->currentWidgetImage.isNull()) {
                painter.setOpacity(d->fadeAnimation.currentValue().toReal());
                painter.drawPixmap(0, 0, d->currentWidgetImage);
            }
            break;
        }

        case AnimationType::Slide: {
            if (!d->previousWidgetImage.isNull()) {
                int x = 0;
                if (d->slideAnimation.startValue().toInt() < d->slideAnimation.endValue().toInt()) {
                    x = (width() + d->slideAnimation.currentValue().toInt()) / 3;
                } else {
                    x = (d->slideAnimation.currentValue().toInt() - width()) / 3;
                }
                painter.drawPixmap(x, 0, d->previousWidgetImage);
            }
            if (!d->currentWidgetImage.isNull()) {
                painter.drawPixmap(d->slideAnimation.currentValue().toInt(), 0, d->currentWidgetImage);
            }
            break;
        }
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
    if (d->currentAnimation().state() == QAbstractAnimation::Running) {
        if (d->previousWidget != nullptr) {
            d->previousWidgetImage = d->previousWidget->grab(QRect({}, _event->size()));
        }
        d->currentWidgetImage = d->currentWidget->grab();
    }
}

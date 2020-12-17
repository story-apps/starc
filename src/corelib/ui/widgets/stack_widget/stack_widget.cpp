#include "stack_widget.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>

#include <utils/tools/run_once.h>

#include <QPainter>
#include <QParallelAnimationGroup>
#include <QResizeEvent>
#include <QSequentialAnimationGroup>
#include <QVariantAnimation>

namespace {

/**
 * @brief Анимация разъезжающегося контейнера
 *        https://material.io/design/motion/the-motion-system.html#container-transform
 */
class ExpandAnimation : public QParallelAnimationGroup
{
public:
    explicit ExpandAnimation(QObject* _parent = nullptr);

    QVariantAnimation outgoingContentFadeOut;
    QVariantAnimation incomingContentFadeIn;
    QSequentialAnimationGroup contentFadeGroup;

    QVariantAnimation contentGeometry;
    QHash<QWidget*, QRect> widgetToRect;

    QVariantAnimation scrimFadeIn;
    QSequentialAnimationGroup scrimFadeGroup;

protected:
    /**
      * @brief Обновим настройки анимаций, при смене направления анимирования
      */
     void updateDirection(QAbstractAnimation::Direction _direction) override;
};

ExpandAnimation::ExpandAnimation(QObject* _parent)
    : QParallelAnimationGroup(_parent)
{
    outgoingContentFadeOut.setDuration(90);
    outgoingContentFadeOut.setEasingCurve(QEasingCurve::InQuad);
    outgoingContentFadeOut.setStartValue(1.0);
    outgoingContentFadeOut.setEndValue(0.0);
    //
    incomingContentFadeIn.setDuration(210);
    incomingContentFadeIn.setEasingCurve(QEasingCurve::OutQuart);
    incomingContentFadeIn.setStartValue(0.0);
    incomingContentFadeIn.setEndValue(1.0);
    //
    contentFadeGroup.addAnimation(&outgoingContentFadeOut);
    contentFadeGroup.addAnimation(&incomingContentFadeIn);
    addAnimation(&contentFadeGroup);

    contentGeometry.setDuration(300);
    contentGeometry.setEasingCurve(QEasingCurve::InOutQuart);
    addAnimation(&contentGeometry);

    scrimFadeIn.setDuration(90);
    scrimFadeIn.setEasingCurve(QEasingCurve::Linear);
    scrimFadeIn.setStartValue(0.0);
    scrimFadeIn.setEndValue(0.4);
    scrimFadeGroup.addAnimation(&scrimFadeIn);
    scrimFadeGroup.addPause(210);
    addAnimation(&scrimFadeGroup);
}

void ExpandAnimation::updateDirection(QAbstractAnimation::Direction _direction)
{
    QParallelAnimationGroup::updateDirection(_direction);

    if (_direction == QAbstractAnimation::Forward) {
        outgoingContentFadeOut.setDuration(90);
        outgoingContentFadeOut.setEasingCurve(QEasingCurve::InQuad);

        incomingContentFadeIn.setDuration(210);
        incomingContentFadeIn.setEasingCurve(QEasingCurve::OutQuart);

        scrimFadeIn.setStartValue(0.0);
        scrimFadeIn.setEndValue(0.4);
    } else {
        outgoingContentFadeOut.setDuration(210);
        outgoingContentFadeOut.setEasingCurve(QEasingCurve::InQuart);

        incomingContentFadeIn.setDuration(90);
        incomingContentFadeIn.setEasingCurve(QEasingCurve::OutQuad);

        scrimFadeIn.setStartValue(0.0);
        scrimFadeIn.setEndValue(0.2);
    }
}

}


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
    ExpandAnimation expandAnimation;
};

StackWidget::Implementation::Implementation()
{
    //
    // TODO: Сделать эти анимации тоже по гайдам
    //

    fadeAnimation.setDuration(240);
    fadeAnimation.setEasingCurve(QEasingCurve::OutQuad);
    fadeAnimation.setStartValue(0.0);
    fadeAnimation.setEndValue(1.0);

    slideAnimation.setDuration(160);
    slideAnimation.setEasingCurve(QEasingCurve::OutQuad);
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

        case AnimationType::Expand: {
            return expandAnimation;
        }
    }
}

// ****

StackWidget::StackWidget(QWidget *_parent)
    : Widget(_parent)
    , d(new Implementation)
{
    auto update = [this] { this->update(); };
    auto showCurrentWidget = [this] { d->currentWidget->show(); };

    connect(&d->fadeAnimation, &QVariantAnimation::valueChanged, this, update);
    connect(&d->fadeAnimation, &QVariantAnimation::finished, this, showCurrentWidget);

    connect(&d->slideAnimation, &QVariantAnimation::valueChanged, this, update);
    connect(&d->slideAnimation, &QVariantAnimation::finished, this, showCurrentWidget);

    connect(&d->expandAnimation.outgoingContentFadeOut, &QVariantAnimation::valueChanged, this, update);
    connect(&d->expandAnimation.incomingContentFadeIn, &QVariantAnimation::valueChanged, this, update);
    connect(&d->expandAnimation.contentGeometry, &QVariantAnimation::valueChanged, this, update);
    connect(&d->expandAnimation.scrimFadeIn, &QVariantAnimation::valueChanged, this, update);
    connect(&d->expandAnimation, &ExpandAnimation::finished, this, showCurrentWidget);
}

void StackWidget::setAnimationType(StackWidget::AnimationType _type)
{
    d->animationType = _type;
}

void StackWidget::setAnimationRect(QWidget* _widget, const QRect& _animationRect)
{
    d->expandAnimation.widgetToRect[_widget] = _animationRect;
}

void StackWidget::addWidget(QWidget* _widget)
{
    if (d->widgets.contains(_widget)) {
        return;
    }

    d->widgets.append(_widget);
    _widget->setParent(this);
    _widget->resize(size());
    _widget->hide();
}

StackWidget::~StackWidget() = default;

void StackWidget::setCurrentWidget(QWidget *_widget)
{
    if (d->currentWidget == _widget) {
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
    d->currentWidget = _widget;
    if (!d->widgets.contains(d->currentWidget)) {
        d->widgets.append(d->currentWidget);
    }
    d->currentWidget->setParent(this);
    d->currentWidget->resize(size());

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
    d->currentWidgetImage = d->currentWidget->grab();
    d->currentWidget->hide();
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

        case AnimationType::Expand: {
            QAbstractAnimation::Direction direction;
            if (d->expandAnimation.widgetToRect.contains(d->previousWidget)) {
                d->expandAnimation.contentGeometry.setStartValue(d->expandAnimation.widgetToRect[d->previousWidget]);
                d->expandAnimation.contentGeometry.setEndValue(rect());
                direction = QAbstractAnimation::Forward;
            } else {
                d->expandAnimation.contentGeometry.setStartValue(d->expandAnimation.widgetToRect[d->currentWidget]);
                d->expandAnimation.contentGeometry.setEndValue(rect());
                direction = QAbstractAnimation::Backward;
            }
            d->expandAnimation.setDirection(direction);
            d->expandAnimation.start();
            break;
        }
    }
}

QWidget* StackWidget::currentWidget() const
{
    return d->currentWidget;
}

QSize StackWidget::sizeHint() const
{
    QSize sizeHint;
    for (auto widget : d->widgets) {
        sizeHint.setWidth(std::max(sizeHint.width(), widget->sizeHint().width()));
        sizeHint.setHeight(std::max(sizeHint.height(), widget->sizeHint().height()));
    }
    return sizeHint;
}

int StackWidget::animationDuration() const
{
    return d->currentAnimation().duration();
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

        case AnimationType::Expand: {
            if (d->previousWidgetImage.isNull() || d->currentWidgetImage.isNull()) {
                break;
            }

            const auto backgroundImage
                    = d->currentAnimation().direction() == QAbstractAnimation::Forward
                      ? d->previousWidgetImage
                      : d->currentWidgetImage;
            const auto foregroundImage
                    = d->currentAnimation().direction() == QAbstractAnimation::Forward
                      ? d->currentWidgetImage
                      : d->previousWidgetImage;

            //
            // Фоновое изображение старого виджета
            //
            painter.drawPixmap(0, 0, backgroundImage);
            //
            // Затемнение
            //
            painter.setOpacity(d->expandAnimation.scrimFadeIn.currentValue().toReal());
            qDebug(QString("%1").arg(painter.opacity()).toUtf8());
            painter.fillRect(rect(), Ui::DesignSystem::color().shadow());
            painter.setOpacity(1.0);
            //
            // Анимируемая область
            //
            const auto animatedRect = d->expandAnimation.contentGeometry.currentValue().toRect();
            painter.fillRect(animatedRect, backgroundColor());
            //
            // Часть старого виджета
            //
            if (d->expandAnimation.outgoingContentFadeOut.state() == QAbstractAnimation::Running) {
                painter.setOpacity(d->expandAnimation.outgoingContentFadeOut.currentValue().toReal());
                painter.drawPixmap(animatedRect.topLeft(), backgroundImage, d->expandAnimation.contentGeometry.startValue().toRect());
            }
            //
            // Новый виджет
            //
            painter.setOpacity(d->expandAnimation.incomingContentFadeIn.currentValue().toReal());
            const auto currentWidgetRect = QRect(QPoint(0, 0), animatedRect.size());
            painter.drawPixmap(animatedRect.topLeft(), foregroundImage, currentWidgetRect);
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

    const auto canRun = RunOnce::tryRun(Q_FUNC_INFO);
    if (!canRun) {
        return;
    }

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

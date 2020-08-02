#include "scroller_helper.h"

#include <QScroller>
#include <QScrollerProperties>
#include <QWidget>


void ScrollerHelper::addScroller(QWidget* _forWidget)
{
    //
    // Добавляем скроллер
    //
    QScroller::grabGesture(_forWidget, QScroller::LeftMouseButtonGesture);
    QScroller* scroller = QScroller::scroller(_forWidget);

    //
    // Настраиваем параметры скроллера
    //
    QScrollerProperties properties = scroller->scrollerProperties();
    //
    // ... время, через которое на прокручиваемый виджет отправляется события нажатия мыши,
    //     если прокрутку начали, не отпустили, но и не сместили в течении этого времени
    //
    properties.setScrollMetric(QScrollerProperties::MousePressEventDelay, 1.0);
    //
    // ... минимальная дистанция смещения для определения жестка прокрутки
    //
    properties.setScrollMetric(QScrollerProperties::DragStartDistance, 0.001);
    //
    // ... минимальная и максимальная скорости прокрутки
    //
    properties.setScrollMetric(QScrollerProperties::MinimumVelocity, 1.0 / 1000);
    properties.setScrollMetric(QScrollerProperties::MaximumVelocity, 550.0 / 1000);
    //
    // ... максимальная скорость, при которой события клика отправляются прокручиваемому виджету
    //
    properties.setScrollMetric(QScrollerProperties::MaximumClickThroughVelocity, 0.0001);
    //
    // ... время жест в течении которого воспринимается, как ускоряющий прокрутку
    //
    properties.setScrollMetric(QScrollerProperties::AcceleratingFlickMaximumTime, 0.1);
    //
    // ... во сколько раз ускоряется прокрутка жестом ускорения
    //
    properties.setScrollMetric(QScrollerProperties::AcceleratingFlickSpeedupFactor, 2.0);
    //
    // ... секция настроек завершения прокрутки и отскока от границы
    //
    QVariant overshootPolicy = QVariant::fromValue<QScrollerProperties::OvershootPolicy>(QScrollerProperties::OvershootAlwaysOff);
    properties.setScrollMetric(QScrollerProperties::HorizontalOvershootPolicy, overshootPolicy);
    properties.setScrollMetric(QScrollerProperties::OvershootDragResistanceFactor, 0.3);
    properties.setScrollMetric(QScrollerProperties::OvershootDragDistanceFactor, 0.02);
    properties.setScrollMetric(QScrollerProperties::OvershootScrollDistanceFactor, 0.3);
    properties.setScrollMetric(QScrollerProperties::OvershootScrollTime, 0.4);
    scroller->setScrollerProperties(properties);
}

#include "CircleFillAnimator.h"
#include "CircleFillDecorator.h"

#include <cmath>

#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

using WAF::CircleFillAnimator;
using WAF::CircleFillDecorator;


CircleFillAnimator::CircleFillAnimator(QWidget* _widgetForFill) :
    AbstractAnimator(_widgetForFill),
    m_decorator(new CircleFillDecorator(_widgetForFill)),
    m_animation(new QPropertyAnimation(m_decorator, "radius"))
{
    Q_ASSERT(_widgetForFill);

    m_animation->setEasingCurve(QEasingCurve::InCubic);
    m_animation->setDuration(400);

    m_decorator->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_decorator->hide();

    connect(m_animation, &QPropertyAnimation::finished, this, &CircleFillAnimator::finalize);
}

void CircleFillAnimator::setStartPoint(const QPoint& _globalPoint)
{
    m_decorator->setStartPoint(_globalPoint);
}

void CircleFillAnimator::setFillColor(const QColor& _color)
{
    m_decorator->setFillColor(_color);
}

void CircleFillAnimator::setHideAfterFinish(bool _hide)
{
    m_hideAfterFinish = _hide;
}

int CircleFillAnimator::animationDuration() const
{
    return m_animation->duration();
}

void CircleFillAnimator::animateForward()
{
    fillIn();
}

void CircleFillAnimator::fillIn()
{
    //
    // Прерываем выполнение, если клиент хочет повторить его
    //
    if (isAnimated() && isAnimatedForward()) return;
    setAnimatedForward();

    //
    // Определим стартовые и финальные позиции для декораций
    //
    const int startRadius = 0;
    //
    const QPoint startPoint = widgetForFill()->mapFromGlobal(m_decorator->startPoint());
    const int w = qMax(startPoint.x(), widgetForFill()->width() - startPoint.x());
    const int h = qMax(startPoint.y(), widgetForFill()->height() - startPoint.y());
    const int finalRadius = sqrt(w*w + h*h);

    //
    // Позиционируем декораторы
    //
    m_decorator->move(0, 0);
    m_decorator->show();
    m_decorator->raise();

    //
    // Анимируем виджет
    //
    if (m_animation->state() == QPropertyAnimation::Running) {
        //
        // ... если ещё не закончилась предыдущая анимация реверсируем её
        //
        m_animation->pause();
        m_animation->setDirection(QPropertyAnimation::Backward);
        m_animation->resume();
    } else {
        //
        // ... если предыдущая анимация закончилась, запускаем новую анимацию
        //
        m_animation->setEasingCurve(QEasingCurve::OutQuart);
        m_animation->setDirection(QPropertyAnimation::Forward);
        m_animation->setStartValue(startRadius);
        m_animation->setEndValue(finalRadius);
        m_animation->start();
    }
}

void CircleFillAnimator::animateBackward()
{
    fillOut();
}

void CircleFillAnimator::fillOut()
{
    //
    // Прерываем выполнение, если клиент хочет повторить его
    //
    if (isAnimated() && isAnimatedBackward()) return;
    setAnimatedBackward();

    //
    // Определим стартовые и финальные позиции для декораций
    //
    int startRadius = sqrt(widgetForFill()->height() * widgetForFill()->height()
                           + widgetForFill()->width() * widgetForFill()->width());
    int finalRadius = 0;

    //
    // Позиционируем декораторы
    //
    m_decorator->move(0, 0);
    m_decorator->show();
    m_decorator->raise();

    //
    // Анимируем виджет
    //
    if (m_animation->state() == QPropertyAnimation::Running) {
        //
        // ... если ещё не закончилась предыдущая анимация реверсируем её
        //
        m_animation->pause();
        m_animation->setDirection(QPropertyAnimation::Backward);
        m_animation->resume();
    } else {
        //
        // ... если предыдущая анимация закончилась, запускаем новую анимацию
        //
        m_animation->setEasingCurve(QEasingCurve::OutQuart);
        m_animation->setDirection(QPropertyAnimation::Forward);
        m_animation->setStartValue(startRadius);
        m_animation->setEndValue(finalRadius);
        m_animation->start();
    }
}

void CircleFillAnimator::finalize()
{
    setAnimatedStopped();
    if (m_hideAfterFinish) {
        hideDecorator();
    }
}

void CircleFillAnimator::hideDecorator()
{
    QGraphicsOpacityEffect* hideEffect = qobject_cast<QGraphicsOpacityEffect*>(m_decorator->graphicsEffect());
    if (hideEffect == nullptr) {
        hideEffect = new QGraphicsOpacityEffect(m_decorator);
        m_decorator->setGraphicsEffect(hideEffect);
    }
    hideEffect->setOpacity(1.);

    QPropertyAnimation* hideAnimation = new QPropertyAnimation(hideEffect, "opacity", m_decorator);
    hideAnimation->setDuration(200);
    hideAnimation->setStartValue(1.);
    hideAnimation->setEndValue(0.);
    connect(hideAnimation, &QPropertyAnimation::finished, [=] {
        m_decorator->hide();
        hideEffect->setOpacity(1.);
    });

    hideAnimation->start(QAbstractAnimation::DeleteWhenStopped);
}

QWidget* CircleFillAnimator::widgetForFill() const
{
    return qobject_cast<QWidget*>(parent());
}

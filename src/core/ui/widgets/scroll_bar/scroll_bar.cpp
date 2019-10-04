#include "scroll_bar.h"

#include <ui/design_system/design_system.h>

#include <QPainter>
#include <QTimer>
#include <QVariantAnimation>


class ScrollBarPrivate
{
public:
    ScrollBarPrivate();

    void maximizeScrollbar();
    void minimizeScrollbar();

    QVariantAnimation widthAnimation;
};

ScrollBarPrivate::ScrollBarPrivate()
{
    widthAnimation.setDuration(120);
    widthAnimation.setStartValue(Ui::DesignSystem::scrollBar().minimumSize());
    widthAnimation.setEndValue(Ui::DesignSystem::scrollBar().maximumSize());
    widthAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void ScrollBarPrivate::maximizeScrollbar()
{
    widthAnimation.stop();
    widthAnimation.setDirection(QVariantAnimation::Forward);
    widthAnimation.start();
}

void ScrollBarPrivate::minimizeScrollbar()
{
    widthAnimation.stop();
    widthAnimation.setDirection(QVariantAnimation::Backward);
    widthAnimation.start();
}


// ****


ScrollBar::ScrollBar(QWidget* _parent)
    : QScrollBar(_parent),
      d(new ScrollBarPrivate)
{
    connect(&d->widthAnimation, &QVariantAnimation::valueChanged, this, [this] { updateGeometry(); });
}

ScrollBar::~ScrollBar() = default;

QSize ScrollBar::sizeHint() const
{
    const QSize marginsDelta = QSizeF(Ui::DesignSystem::scrollBar().margins().left()
                                      + Ui::DesignSystem::scrollBar().margins().right(),
                                      Ui::DesignSystem::scrollBar().margins().top()
                                      + Ui::DesignSystem::scrollBar().margins().bottom()).toSize();
    const qreal directedSize = std::max(Ui::DesignSystem::scrollBar().minimumSize(),
                                        d->widthAnimation.currentValue().toReal());
    if (orientation() == Qt::Vertical) {
        return QSize(static_cast<int>(directedSize), 10) + marginsDelta;
    } else {
        return QSize(10, static_cast<int>(directedSize)) + marginsDelta;
    }
}

void ScrollBar::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    QPainter painter(this);
    painter.fillRect(rect(), Qt::transparent);

    //
    // Настраиваем видимость полосы прокрутки
    //
    const qreal opacity = d->widthAnimation.currentValue().toReal();
    painter.setOpacity(opacity);

    //
    // Рисуем фон скролбара
    //
    const QRectF contentRect = QRectF(rect()).marginsRemoved(Ui::DesignSystem::scrollBar().margins());
    painter.fillRect(contentRect, Ui::DesignSystem::scrollBar().backgroundColor());

    //
    // Рисуем хэндл
    //
    qreal handleDelta = (Qt::Horizontal == orientation() ? contentRect.width()
                                                         : contentRect.height())
                        / static_cast<qreal>(maximum() - minimum() + pageStep() - 1);
    qreal additionalHandleMovement = orientation() == Qt::Horizontal
                                     ? contentRect.left()
                                     : contentRect.top();
    if (pageStep() * handleDelta < Ui::DesignSystem::scrollBar().minimumHandleLength()) {
        handleDelta = (Qt::Horizontal == orientation() ? contentRect.width()
                                                       : contentRect.height())
                      / static_cast<qreal>(maximum() - minimum());
        additionalHandleMovement -= Ui::DesignSystem::scrollBar().minimumHandleLength() * value()
                                    / static_cast<qreal>(maximum() - minimum());
    }
    const QRectF handle = orientation() == Qt::Horizontal
                          ? QRectF(sliderPosition() * handleDelta + additionalHandleMovement, contentRect.top(),
                                   std::max(pageStep() * handleDelta,
                                            Ui::DesignSystem::scrollBar().minimumHandleLength()), contentRect.height())
                          : QRectF(contentRect.left(), sliderPosition() * handleDelta + additionalHandleMovement,
                                   contentRect.width(), std::max(pageStep() * handleDelta,
                                                                 Ui::DesignSystem::scrollBar().minimumHandleLength()));
    painter.fillRect(handle, Ui::DesignSystem::scrollBar().handleColor());
}

void ScrollBar::enterEvent(QEvent* _event)
{
    Q_UNUSED(_event);
    d->maximizeScrollbar();
}

void ScrollBar::leaveEvent(QEvent* _event)
{
    Q_UNUSED(_event);
    d->minimizeScrollbar();
}

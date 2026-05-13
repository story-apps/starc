#include "CircleFillDecorator.h"

#include <QPainter>

using WAF::CircleFillDecorator;


CircleFillDecorator::CircleFillDecorator(QWidget* _parent)
    : QWidget(_parent)
    , m_radius(0)
    , m_fillColor(Qt::white)
{
}

void CircleFillDecorator::setStartPoint(const QPoint& _globalPoint)
{
    QPoint localStartPoint = mapFromGlobal(_globalPoint);
    if (m_startPoint != localStartPoint) {
        m_startPoint = localStartPoint;
    }
}

QPoint CircleFillDecorator::startPoint() const
{
    return mapToGlobal(m_startPoint);
}

int CircleFillDecorator::radius() const
{
    return m_radius;
}

void CircleFillDecorator::setRadius(int _radius)
{
    if (m_radius == _radius) {
        return;
    }

    const auto updateRectForRadius = [this](int radius) {
        constexpr int padding = 2;
        const auto diameter = radius * 2;
        return QRect(m_startPoint - QPoint(radius, radius), QSize(diameter, diameter))
            .adjusted(-padding, -padding, padding, padding);
    };

    const QRect dirtyRect = updateRectForRadius(m_radius).united(updateRectForRadius(_radius));
    m_radius = _radius;
    update(dirtyRect);
}

void CircleFillDecorator::setFillColor(const QColor& _fillColor)
{
    if (m_fillColor != _fillColor) {
        m_fillColor = _fillColor;
    }
}

void CircleFillDecorator::paintEvent(QPaintEvent* _event)
{
    if (size() != parentWidget()->size()) {
        resize(parentWidget()->size());
        return;
    }

    QPainter painter(this);
    painter.setPen(m_fillColor);
    painter.setBrush(m_fillColor);
    const qreal opacity = (qreal)m_radius / ((qreal)(width() + height()) / 4.);
    painter.setOpacity(opacity);
    painter.drawEllipse(m_startPoint, m_radius, m_radius);

    QWidget::paintEvent(_event);
}

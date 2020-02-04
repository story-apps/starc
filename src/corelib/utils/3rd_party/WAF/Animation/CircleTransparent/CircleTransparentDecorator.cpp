#include "CircleTransparentDecorator.h"

#include <QPainter>

using WAF::CircleTransparentDecorator;


CircleTransparentDecorator::CircleTransparentDecorator(QWidget* _parent) :
    QWidget(_parent),
    m_radius(0)
{
}

void CircleTransparentDecorator::setStartPoint(const QPoint& _globalPoint)
{
    QPoint localStartPoint = mapFromGlobal(_globalPoint);
    if (m_startPoint != localStartPoint) {
        m_startPoint = localStartPoint;
    }
}

QPoint CircleTransparentDecorator::startPoint() const
{
    return mapToGlobal(m_startPoint);
}

int CircleTransparentDecorator::radius() const
{
    return m_radius;
}

void CircleTransparentDecorator::setRadius(int _radius)
{
    if (m_radius == _radius) {
        return;
    }

    m_radius = _radius;
    update();
}

void CircleTransparentDecorator::setFillImage(const QPixmap& _fillImage)
{
    m_fillImage = _fillImage;
    update();
}

void CircleTransparentDecorator::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

    if (size() != parentWidget()->size()) {
        resize(parentWidget()->size());
        return;
    }

    if (m_fillImage.isNull()) {
        return;
    }

    QPainter painter(this);

    const auto clipRegion = QRegion(rect());
    const auto ellipseSize = m_radius * 2;
    const auto innerEllipse = QRegion(QRect(m_startPoint - QPoint(m_radius, m_radius),
                                            QSize(ellipseSize, ellipseSize)), QRegion::Ellipse);
    painter.setClipRegion(clipRegion.xored(innerEllipse));

    painter.drawPixmap(0, 0, m_fillImage);
}

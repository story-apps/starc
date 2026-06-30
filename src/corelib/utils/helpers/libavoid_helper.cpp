#include "libavoid_helper.h"

#include <libavoid/connector.h>
#include <libavoid/geomtypes.h>

#include <QPainterPath>
#include <QRectF>


Avoid::Point LibavoidHelper::convertPoint(const QPointF& _point)
{
    return Avoid::Point(_point.x(), _point.y());
}

Avoid::Rectangle LibavoidHelper::convertRectangle(const QRectF& _rect)
{
    QPointF topLeft = _rect.topLeft();
    QPointF bottomRight = _rect.bottomRight();

    return Avoid::Rectangle(Avoid::Point(topLeft.x(), topLeft.y()),
                            Avoid::Point(bottomRight.x(), bottomRight.y()));
}

Avoid::Polygon LibavoidHelper::convertPolygon(const QPolygonF& _polygon)
{
    Avoid::Polygon newPolygon;
    newPolygon.ps.clear();

    for (int idx = 0; idx < _polygon.size(); ++idx) {
        newPolygon.ps.push_back(Avoid::Point(_polygon[idx].x(), _polygon[idx].y()));
    }

    return newPolygon;
}

QPainterPath LibavoidHelper::makePainterPath(Avoid::ConnRef* _connection)
{
    const Avoid::PolyLine displayRoute = _connection->displayRoute().curvedPolyline(4.0);
    if (displayRoute.empty()) {
        return {};
    }

    Avoid::Point p = displayRoute.at(0);
    QPainterPath path(QPointF(p.x, p.y));

    for (size_t i = 0; i < displayRoute.size(); ++i) {
        const auto& pointInfo = displayRoute.ts[i];
        if (pointInfo == 'C') {
            const auto& c1 = displayRoute.ps[i];
            const auto& c2 = displayRoute.ps[i + 1];
            const auto& c3 = displayRoute.ps[i + 2];
            path.cubicTo(c1.x, c1.y, c2.x, c2.y, c3.x, c3.y);
            i += 2;
        } else {
            const auto& point = displayRoute.ps[i];
            path.lineTo(QPointF(point.x, point.y));
        }
    }

    return path;
}

#include "libavoid_helper.h"

#include <libavoid/connector.h>
#include <libavoid/geomtypes.h>

#include <QPainterPath>
#include <QRectF>


Avoid::Rectangle LibavoidHelper::convertRectangle(const QRectF& rect)
{
    QPointF topLeft = rect.topLeft();
    QPointF bottomRight = rect.bottomRight();

    return Avoid::Rectangle(Avoid::Point(topLeft.x(), topLeft.y()),
                            Avoid::Point(bottomRight.x(), bottomRight.y()));
}

Avoid::Polygon LibavoidHelper::convertPolygon(const QPolygonF& polygon)
{
    Avoid::Polygon newPolygon;
    newPolygon.ps.clear();

    for (int idx = 0; idx < polygon.size(); ++idx) {
        newPolygon.ps.push_back(Avoid::Point(polygon[idx].x(), polygon[idx].y()));
    }

    return newPolygon;
}

QPainterPath LibavoidHelper::makePainterPath(Avoid::ConnRef* connection)
{
    const Avoid::PolyLine displayRoute = connection->displayRoute().curvedPolyline(4.0);
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

#pragma once

#include <corelib_global.h>

class QPainterPath;
class QPointF;
class QPolygonF;
class QRectF;

namespace Avoid {
class ConnRef;
class Point;
class Polygon;
class Rectangle;
} // namespace Avoid


/**
 * @brief Вспомогательные методы для работы с libavoid
 */
class CORE_LIBRARY_EXPORT LibavoidHelper
{
public:
    static Avoid::Point convertPoint(const QPointF& _point);
    static Avoid::Rectangle convertRectangle(const QRectF& _rect);
    static Avoid::Polygon convertPolygon(const QPolygonF& _polygon);

    static QPainterPath makePainterPath(Avoid::ConnRef* _connection);
};

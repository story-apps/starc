#pragma once

#include <corelib_global.h>

class QPainterPath;
class QPolygonF;
class QRectF;

namespace Avoid {
class ConnRef;
class Polygon;
class Rectangle;
} // namespace Avoid


/**
 * @brief Вспомогательные методы для работы с libavoid
 */
class CORE_LIBRARY_EXPORT LibavoidHelper
{
public:
    static Avoid::Rectangle convertRectangle(const QRectF& rect);
    static Avoid::Polygon convertPolygon(const QPolygonF& polygon);

    static QPainterPath makePainterPath(Avoid::ConnRef* connection);
};

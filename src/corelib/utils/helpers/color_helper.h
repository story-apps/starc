#pragma once

#include <corelib_global.h>

#include <QtGlobal>

class QColor;


class CORE_LIBRARY_EXPORT ColorHelper
{
public:
    /**
     * @brief Получить полупрозрачный цвет
     */
    static QColor transparent(const QColor& _color, qreal _alphaF);

    /**
     * @brief Получить промежуточный цвет между заданными
     */
    static QColor colorBetween(const QColor& _lhs, const QColor& _rhs);

    /**
     * @brief Определить, является ли цвет светлым
     */
    static bool isColorLight(const QColor& _color);

    /**
     * @brief Получить контрастный цвет к заданному
     */
    static QColor contrasted(const QColor& _color);

    /**
     * @brief Получить инвертированный цвет
     */
    static QColor inverted(const QColor& _color);

    /**
     * @brief Получить цвет рядом с заданым
     */
    static QColor nearby(const QColor& _color);
};

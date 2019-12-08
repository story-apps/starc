#pragma once

class QColor;


class ColorHelper
{
public:
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
    static QColor contrast(const QColor& _color);
};

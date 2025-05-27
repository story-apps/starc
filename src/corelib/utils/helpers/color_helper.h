#pragma once

#include <QtGlobal>

#include <corelib_global.h>

class QColor;


class CORE_LIBRARY_EXPORT ColorHelper
{
public:
    /**
     * @brief Цвет фона удалённого текста
     */
    static QColor removedTextBackgroundColor();

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
    static QColor nearby(const QColor& _color, int _f = 110);

    /**
     * @brief Сформировать строку из цвета
     */
    static QString toString(const QColor& _color);

    /**
     * @brief Сформировать цвет из строки
     */
    static QColor fromString(const QString& _colorName);

    /**
     * @brief Получить цветовой образ заданного текста
     */
    static QColor forText(const QString& _text);

    /**
     * @brief Получить цвет для заданного числа
     */
    static QColor forNumber(int _number);

    /**
     * @brief Цвет ревизии по уровню
     */
    static QColor revisionColor(int _level);

    /**
     * @brief Уровень ревизии по цвету
     */
    static int revisionLevel(const QColor& _color);
};

#pragma once

#include <QtGlobal>

class QFont;
class QSizeF;
class QString;
class QTextOption;


/**
 * @brief Вспомогательные функции для работы с текстом
 */
class TextHelper
{
public:
    /**
     * @brief Определить оптимальную ширину текста
     */
    static int fineTextWidth(const QString& _text, const QFont& _font);

    /**
     * Возвращает высоту текста
     * @param text Текст
     * @param font Шрифт, которым рисуется текст
     * @param width Ширина фигуры (она останется неизменной)
     * @param option Параметры отображения
     * @return Размер прямоугольника.
     */
    static qreal heightForWidth(const QString& _text, const QFont& _font, int _width);
};

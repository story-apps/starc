#pragma once

#include <corelib_global.h>

#include <QtGlobal>

class QChar;
class QFont;
class QFontMetricsF;
class QRectF;
class QString;


/**
 * @brief Вспомогательные функции для работы с текстом
 */
class CORE_LIBRARY_EXPORT TextHelper
{
public:
    /**
     * @brief Определить оптимальную ширину текста
     */
    static qreal fineTextWidth(const QString& _text, const QFont& _font);
    static qreal fineTextWidth(const QString& _text, const QFontMetricsF& _metrics);

    /**
     * @brief Определить правильную высоту строки для заданного шрифта
     */
    static qreal fineLineSpacing(const QFont& _font);
    static qreal fineLineSpacing(const QFontMetricsF& _metrics);

    /**
     * Возвращает высоту текста
     * @param text Текст
     * @param font Шрифт, которым рисуется текст
     * @param width Ширина фигуры (она останется неизменной)
     * @param option Параметры отображения
     * @return Размер прямоугольника.
     */
    static qreal heightForWidth(const QString& _text, const QFont& _font, qreal _width);

    /**
     * @brief Сформировать замноготоченный текст из исходного в рамках заданной области
     */
    static QString elidedText(const QString& _text, const QFont& _font, const QRectF& _rect);

    /**
     * @brief Преобразовать специфичные символы к html-виду
     */
    static QString toHtmlEscaped(const QString& _text);

    /**
     * @brief Преобразовать html-специфичные символы к обычному виду
     */
    static QString fromHtmlEscaped(const QString& _escaped);

    /**
     * @brief Продвинутый toUpper с учётом некоторых специфичных юникод-символов
     */
    static QString smartToUpper(const QString& _text);
    static QChar smartToUpper(const QChar& _char);

    /**
     * @brief Продвинутый toLower с учётом некоторых специфичных юникод-символов
     */
    static QString smartToLower(const QString& _text);
    static QChar smartToLower(const QChar& _char);
};

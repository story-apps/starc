#pragma once

#include <QtGlobal>

#include <corelib_global.h>
#include <functional>

class QChar;
class QFont;
class QFontMetricsF;
class QRectF;
class QString;
class QTextBlock;
class QTextCharFormat;
class QTextCursor;


/**
 * @brief Вспомогательные функции для работы с текстом
 */
class CORE_LIBRARY_EXPORT TextHelper
{
public:
    /**
     * @brief Определить оптимальную ширину текста
     */
    static qreal fineTextWidthF(const QString& _text, const QFont& _font);
    static qreal fineTextWidthF(const QString& _text, const QFontMetricsF& _metrics);
    static int fineTextWidth(const QString& _text, const QFont& _font);

    /**
     * @brief Определить правильную высоту строки для заданного шрифта
     */
    static qreal fineLineSpacing(const QFont& _font);

    /**
     * @brief Обновить хинтинг шрифта в зависимости от гарнитуры
     */
    static void updateFontHinting(QFont& _font);

    /**
     * @brief Быстро оценить высоту текста в прямоугольнике заданной ширины.
     *
     * Метод использует QFontMetricsF::boundingRect и подходит для утилитарных UI-сценариев,
     * где важнее получить недорогую приблизительную высоту: подписи, карточки, делегаты списков,
     * подсказки и другие элементы интерфейса, в которых результат не должен в точности повторять
     * построчную раскладку редактора. Такой расчёт может отличаться от QTextLayout/QTextDocument
     * на текстах с явными переводами строк, длинными словами без пробелов, специальными символами
     * переноса или при необходимости использовать ровно заданную высоту строки.
     *
     * Если высота влияет на пагинацию, хронометраж, экспорт или другой пользовательски видимый
     * расчёт расположения текста, используйте layoutHeightForWidth: он медленнее, зато компонует
     * текст построчно и ближе к поведению текстового редактора.
     *
     * @param _text Текст.
     * @param _font Шрифт, которым рисуется текст.
     * @param _width Ширина области текста.
     * @param _lineHeight Высота строки для масштабирования результата; если -1, используется
     *                    стандартная высота QFontMetricsF::boundingRect.
     */
    static qreal heightForWidth(const QString& _text, const QFont& _font, qreal _width);

    /**
     * @brief Точно скомпоновать текст и вернуть высоту построчной раскладки в заданной ширине.
     *
     * Метод использует QTextLayout с QTextOption::WrapAtWordBoundaryOrAnywhere, предварительно
     * заменяя '\n' на QChar::LineSeparator, поэтому одинаково учитывает ручные переводы строк и
     * переносит длинные слова, которые не помещаются в строку. Его следует применять там, где
     * расхождение даже на одну строку приводит к ошибкам в логике: расчёт страниц и «восьмушек»,
     * хронометраж, предпросмотр/экспорт, проверка попадания текста в область.
     *
     * Не используйте этот метод как универсальную замену heightForWidth в массовой отрисовке
     * простых UI-элементов: из-за реальной компоновки он дороже. Для приблизительных высот без
     * строгого соответствия редактору оставляйте heightForWidth.
     *
     * @param _text Текст.
     * @param _font Шрифт, которым компонуется текст.
     * @param _width Ширина области текста.
     * @param _lineHeight Высота строки; если -1, используется fineLineSpacing(_font).
     */
    static qreal layoutHeightForWidth(const QString& _text, const QFont& _font, qreal _width,
                                      qreal _lineHeight);


    /**
     * @brief Определить последнюю строку для текста в блоке заданной ширины
     */
    static QString lastLineText(const QString& _text, const QFont& _font, qreal _width);

    /**
     * @brief Сформировать замноготоченный текст из исходного в рамках заданной области
     */
    static QString elidedText(const QString& _text, const QFont& _font, const QRectF& _rect);
    static QString elidedText(const QString& _text, const QFont& _font, qreal _width);

    /**
     * @brief Преобразовать специфичные символы к html-виду
     */
    static QString toHtmlEscaped(const QString& _text);

    /**
     * @brief Экранировать специфичные символы, чтобы их можно было использовать в регулярках
     */
    static QString toRxEscaped(const QString& _text);

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

    /**
     * @brief Очистить текст от управляющих символов
     */
    static QString removeControlCharacters(const QString& _text);

    /**
     * @brief Убрать все "пустые" символы строки
     */
    static QString simplified(const QString& _text, bool _keepLineBreaks = false);

    /**
     * @brief Оформить текст как предложение (первая заглавная, остальные строчные)
     */
    static QString toSentenceCase(const QString& _text, bool _capitalizeEveryWord = false,
                                  bool _capitalizeEverySentence = false);

    /**
     * @brief Определить количество слов в тексте
     */
    static int wordsCount(const QString& _text);

    /**
     * @brief Применить заданный функтор форматирования для выделенного текста в курсоре
     */
    static void updateSelectionFormatting(
        QTextCursor& _cursor, std::function<QTextCharFormat(const QTextCharFormat&)> _updateFormat);

    /**
     * @brief Применить заданный формат к абзацу, в котором установлен курсор, сохраняя при этом
     *        нюансы исходного форматирования текста
     */
    static void applyTextFormattingForBlock(QTextCursor& _cursor,
                                            const QTextCharFormat& _newFormat);

    /**
     * @brief Получить правильный формат блока
     */
    static QTextCharFormat fineBlockCharFormat(const QTextBlock& _block);

    /**
     * @brief Находится ли текст в верхнем регистре
     */
    static bool isUppercase(const QString& _text);
};

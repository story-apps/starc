#pragma once

#include <QChar>
#include <QRegularExpression>
#include <QString>
#include <QTextBlock>

class QTextDocument;


namespace Ui {
namespace Vim {

/**
 * @brief Диапазон текста, над которым выполняется команда
 * @note Конец диапазона не входит в него. Для построчных диапазонов начало указывает на начало
 *       первой строки, а конец - на конец текста последней строки, без учёта разделителя абзацев
 */
struct TextRange {
    int start = -1;
    int end = -1;
    bool linewise = false;

    bool isValid() const;
};

/**
 * @brief Результат применения текстового объекта
 * @note Позиции заданы так же, как и в vim: начало и курсор, а признак включения говорит о том,
 *       входит ли символ под курсором в диапазон
 */
struct ObjectRange {
    int start = -1;
    int cursor = -1;
    bool inclusive = false;
    bool linewise = false;

    bool isValid() const;
};

/**
 * @brief Может ли курсор находиться в заданном блоке
 * @note Курсор не заходит в скрытые блоки, а так же в декорации и служебные блоки таблиц
 */
bool isNavigableBlock(const QTextBlock& _block);

/**
 * @brief Блоки, в которых может находиться курсор
 */
QTextBlock nextNavigableBlock(const QTextBlock& _block);
QTextBlock previousNavigableBlock(const QTextBlock& _block);
QTextBlock firstNavigableBlock(const QTextDocument* _document);
QTextBlock lastNavigableBlock(const QTextDocument* _document);

/**
 * @brief Блок, в котором должен оказаться курсор, если он будет установлен в заданную позицию
 */
QTextBlock navigableBlock(const QTextDocument* _document, int _position);

/**
 * @brief Количество блоков, в которых может находиться курсор
 */
int navigableBlocksCount(const QTextDocument* _document);

/**
 * @brief Блок с заданным порядковым номером среди тех, в которых может находиться курсор
 * @note Номера начинаются с единицы, номера за пределами документа ограничиваются его границами
 */
QTextBlock navigableBlockByNumber(const QTextDocument* _document, int _number);

/**
 * @brief Порядковый номер блока среди тех, в которых может находиться курсор
 */
int navigableBlockNumber(const QTextBlock& _block);

/**
 * @brief Позиция конца текста абзаца, в котором находится заданный блок
 */
int lineEndPosition(const QTextBlock& _block);

/**
 * @brief Позиция первого значимого символа строки
 */
int firstNonBlankPosition(const QTextBlock& _block);

/**
 * @brief Курсор, перемещающийся по документу так, как это делает vim
 * @note Позиция в конце строки соответствует символу перевода строки, из конца строки курсор
 *       переходит в начало следующей строки, в которой он может находиться
 */
class Cursor
{
public:
    Cursor(const QTextDocument* _document, int _position);

    /**
     * @brief Позиция в документе
     */
    int position() const;
    void setPosition(int _position);

    /**
     * @brief Строка и позиция в ней
     */
    QTextBlock block() const;
    int column() const;
    void setColumn(int _column);
    int lineLength() const;
    bool isLineEmpty() const;
    bool atLineEnd() const;

    /**
     * @brief Символ под курсором, в конце строки - пустой символ
     */
    QChar character() const;

    /**
     * @brief Класс символа под курсором: 0 - пробел, 1 - знак препинания, 2 - символ слова
     */
    int characterClass(bool _bigWord) const;

    /**
     * @brief Сдвинуться на символ вперёд
     * @return 0 - сдвинулись в пределах строки, 1 - перешли на следующую строку, 2 - встали на
     *         конец строки, -1 - дальше двигаться некуда
     */
    int inc();

    /**
     * @brief Сдвинуться на символ назад
     * @return 0 - сдвинулись в пределах строки, 1 - перешли на предыдущую строку, -1 - дальше
     *         двигаться некуда
     */
    int dec();

    /**
     * @brief Сдвинуться вперёд или назад, перешагивая через конец строки
     */
    int incl();
    int decl();

    /**
     * @brief Находится ли курсор в отступе строки
     */
    bool inIndent(int _extra) const;

private:
    void setBlock(const QTextBlock& _block);

    const QTextDocument* m_document = nullptr;
    QTextBlock m_block;
    QString m_text;
    int m_column = 0;
};

/**
 * @brief Перемещения по словам, повторяющие соответствующие алгоритмы vim
 * @note Большим словом считается любая последовательность непробельных символов
 */
bool forwardWord(Cursor& _cursor, int _count, bool _bigWord, bool _stopAtLineEnd);
bool backwardWord(Cursor& _cursor, int _count, bool _bigWord, bool _stop);
bool endOfWord(Cursor& _cursor, int _count, bool _bigWord, bool _stop, bool _stopAtEmptyLine);
bool backwardEndOfWord(Cursor& _cursor, int _count, bool _bigWord, bool _stopAtLineEnd);

/**
 * @brief Позиция парной скобки для ближайшей скобки в строке
 * @return -1, если скобка или пара для неё не найдены
 */
int matchingBracketPosition(const QTextDocument* _document, int _position);

/**
 * @brief Перемещение по абзацам, разделённым пустыми строками
 * @return -1, если перемещение невозможно
 */
int paragraphPosition(const QTextDocument* _document, int _position, bool _forward, int _count,
                      bool* _inclusive);

/**
 * @brief Перемещение по предложениям
 * @return Удалось ли переместиться
 */
bool findSentence(Cursor& _cursor, bool _forward, int _count);

/**
 * @brief Текстовые объекты
 * @note В визуальном режиме объекты расширяют уже имеющееся выделение
 */
ObjectRange wordObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                       int _count, bool _around, bool _bigWord);
ObjectRange bracketObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                          int _count, bool _around, QChar _openBracket, QChar _closeBracket);
ObjectRange quoteObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                        int _count, bool _around, QChar _quote);
ObjectRange sentenceObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                           int _count, bool _around);
ObjectRange paragraphObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                            bool _visualLine, int _count, bool _around);

/**
 * @brief Слово под курсором, либо ближайшее за ним в строке - для поиска по звёздочке
 * @param _isKeyword - состоит ли найденное слово из символов слова, или это просто набор
 *        непробельных символов
 * @param _start - позиция начала слова в документе
 */
QString wordUnderCursor(const QTextDocument* _document, int _position, bool* _isKeyword,
                        int* _start);

/**
 * @brief Регулярное выражение из шаблона в синтаксисе vim
 */
QRegularExpression regularExpression(const QString& _pattern);

/**
 * @brief Текст для замены найденного совпадения, заданный в синтаксисе vim
 */
QString substitution(const QRegularExpressionMatch& _match, const QString& _replacement);

} // namespace Vim
} // namespace Ui

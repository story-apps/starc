#pragma once

#include <QChar>
#include <QString>

class QTextDocument;


namespace Ui {
namespace Vim {

/**
 * @brief Диапазон текста, над которым выполняется команда
 * @note Для построчных диапазонов конец указывает на конец текста последней строки, без учёта
 *       разделителя абзацев
 */
struct TextRange {
    int start = -1;
    int end = -1;
    bool linewise = false;

    bool isValid() const;
};

/**
 * @brief Ограничить позицию границами документа
 */
int boundedPosition(const QTextDocument* _document, int _position);

/**
 * @brief Позиция последнего символа документа
 */
int lastPosition(const QTextDocument* _document);

/**
 * @brief Позиции начала и конца строки, в которой находится заданная позиция
 */
int startOfLinePosition(const QTextDocument* _document, int _position);
int endOfLinePosition(const QTextDocument* _document, int _position);

/**
 * @brief Позиция первого значимого символа строки
 */
int firstNonBlankPosition(const QTextDocument* _document, int _position);

/**
 * @brief Позиция первого значимого символа строки с заданным номером
 */
int linePosition(const QTextDocument* _document, int _lineNumber);

/**
 * @brief Перемещения по словам
 * @note Большим словом считается любая последовательность непробельных символов
 */
int nextWordPosition(const QTextDocument* _document, int _position, bool _bigWord);
int previousWordPosition(const QTextDocument* _document, int _position, bool _bigWord);
int endOfWordPosition(const QTextDocument* _document, int _position, bool _bigWord);

/**
 * @brief Позиция заданного символа в строке
 * @return -1, если символ не найден
 */
int findCharacterPosition(const QTextDocument* _document, int _position, QChar _character,
                          bool _forward, bool _till, int _count);

/**
 * @brief Позиция парной скобки для ближайшей скобки в строке
 * @return -1, если скобка или пара для неё не найдены
 */
int matchingBracketPosition(const QTextDocument* _document, int _position);

/**
 * @brief Позиция начала ближайшей пустой строки
 */
int paragraphPosition(const QTextDocument* _document, int _position, bool _forward, int _count);

/**
 * @brief Текстовые объекты
 */
TextRange wordRange(const QTextDocument* _document, int _position, bool _bigWord, bool _around);
TextRange blockRange(const QTextDocument* _document, int _position, QChar _openBracket,
                     QChar _closeBracket, bool _around);
TextRange quoteRange(const QTextDocument* _document, int _position, QChar _quote, bool _around);
TextRange paragraphRange(const QTextDocument* _document, int _position, bool _around);

/**
 * @brief Слово под курсором - для поиска по звёздочке
 */
QString wordUnderCursor(const QTextDocument* _document, int _position);

} // namespace Vim
} // namespace Ui

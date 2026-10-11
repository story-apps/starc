#include "vim_motions.h"

#include <business_layer/templates/text_template.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>

#include <QTextDocument>
#include <QVector>

#include <algorithm>

namespace Ui {
namespace Vim {

namespace {

/**
 * @brief Класс символа, по которому определяются границы слов
 */
int characterClass(QChar _character, bool _bigWord)
{
    if (_character.isNull() || _character.isSpace()) {
        return 0;
    }

    if (_bigWord) {
        return 1;
    }

    if (_character.isLetterOrNumber() || _character.isMark() || _character == '_') {
        return 2;
    }

    return 1;
}

/**
 * @brief Количество символов в строке без учёта разделителя абзацев
 */
int lineLength(const QTextBlock& _block)
{
    return qMax(0, _block.length() - 1);
}

/**
 * @brief Состоит ли строка только из пробельных символов
 */
bool isLineWhite(const QTextBlock& _block)
{
    const auto text = _block.text();
    for (const auto& character : text) {
        if (!character.isSpace()) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Сдвинуть курсор на символ влево в пределах строки
 */
bool oneLeft(Cursor& _cursor)
{
    if (_cursor.column() == 0) {
        return false;
    }

    _cursor.setColumn(_cursor.column() - 1);
    return true;
}

/**
 * @brief Пропустить символы заданного класса
 * @return Упёрлись ли в границу документа
 */
bool skipCharacters(Cursor& _cursor, int _class, bool _bigWord, bool _forward)
{
    while (_cursor.characterClass(_bigWord) == _class) {
        if ((_forward ? _cursor.inc() : _cursor.dec()) == -1) {
            return true;
        }
    }
    return false;
}

/**
 * @brief Перейти к началу слова или пробелов, на которых стоит курсор, в пределах строки
 */
void backInLine(Cursor& _cursor, bool _bigWord)
{
    const int startClass = _cursor.characterClass(_bigWord);
    while (_cursor.column() > 0) {
        _cursor.dec();
        if (_cursor.characterClass(_bigWord) != startClass) {
            _cursor.inc();
            break;
        }
    }
}

/**
 * @brief Найти незакрытую скобку перед курсором, или незакрытую скобку после него
 * @note Символ под курсором не учитывается
 */
int findUnmatchedBracket(const QTextDocument* _document, int _position, QChar _openBracket,
                         QChar _closeBracket, bool _forward)
{
    Cursor cursor(_document, _position);
    const auto targetBracket = _forward ? _closeBracket : _openBracket;
    const auto nestedBracket = _forward ? _openBracket : _closeBracket;
    int depth = 0;
    while ((_forward ? cursor.inc() : cursor.dec()) != -1) {
        const auto character = cursor.character();
        if (character == nestedBracket) {
            ++depth;
        } else if (character == targetBracket) {
            if (depth == 0) {
                return cursor.position();
            }
            --depth;
        }
    }
    return -1;
}

/**
 * @brief Найти следующую открывающую скобку, если курсор не находится внутри скобок
 */
int findNextOpenBracket(const QTextDocument* _document, int _position, QChar _openBracket,
                        QChar _closeBracket)
{
    Cursor cursor(_document, _position);
    int depth = 0;
    while (cursor.inc() != -1) {
        const auto character = cursor.character();
        if (character == _closeBracket) {
            ++depth;
        } else if (character == _openBracket) {
            if (depth == 0) {
                return cursor.position();
            }
            --depth;
        }
    }
    return -1;
}

/**
 * @brief Поиск кавычек в строке с учётом экранирования
 */
int findNextQuote(const QString& _line, int _column, QChar _quote, bool _escape)
{
    int column = _column;
    while (column >= 0 && column < _line.size()) {
        const auto character = _line.at(column);
        if (_escape && character == '\\') {
            ++column;
            if (column >= _line.size()) {
                return -1;
            }
        } else if (character == _quote) {
            return column;
        }
        ++column;
    }
    return -1;
}

int findPreviousQuote(const QString& _line, int _column, QChar _quote, bool _escape)
{
    int column = _column;
    while (column > 0) {
        --column;
        int escapes = 0;
        if (_escape) {
            while (column - escapes > 0 && _line.at(column - escapes - 1) == '\\') {
                ++escapes;
            }
        }
        if (escapes & 1) {
            column -= escapes;
        } else if (_line.at(column) == _quote) {
            break;
        }
    }
    return column;
}

/**
 * @brief Символ строки, либо пустой символ за её пределами
 */
QChar characterAt(const QString& _line, int _column)
{
    return _column >= 0 && _column < _line.size() ? _line.at(_column) : QChar();
}

bool isWhite(QChar _character)
{
    return _character == ' ' || _character == '\t';
}

/**
 * @brief Перейти назад к началу слова или пробелов
 */
void findFirstBlank(Cursor& _cursor)
{
    while (_cursor.decl() != -1) {
        if (!isWhite(_cursor.character())) {
            _cursor.incl();
            break;
        }
    }
}

/**
 * @brief Пропустить заданное количество предложений и пробелов между ними, как findsent_forward
 */
void findSentenceForward(Cursor& _cursor, int _count, bool _atStartOfSentence)
{
    bool atStartOfSentence = _atStartOfSentence;
    int count = _count;
    while (count-- > 0) {
        findSentence(_cursor, true, 1);
        if (atStartOfSentence) {
            findFirstBlank(_cursor);
        }
        if (count == 0 || atStartOfSentence) {
            _cursor.decl();
        }
        atStartOfSentence = !atStartOfSentence;
    }
}

} // namespace


bool TextRange::isValid() const
{
    return start >= 0 && end >= start;
}

bool ObjectRange::isValid() const
{
    return start >= 0 && cursor >= 0;
}

bool isNavigableBlock(const QTextBlock& _block)
{
    if (!_block.isValid() || !_block.isVisible()) {
        return false;
    }

    using BusinessLayer::TextBlockStyle;
    const auto format = _block.blockFormat();
    return !format.boolProperty(TextBlockStyle::PropertyIsCorrection)
        && !format.boolProperty(PageTextEdit::PropertyDontShowCursor)
        && TextBlockStyle::forBlock(_block) != BusinessLayer::TextParagraphType::PageSplitter;
}

QTextBlock nextNavigableBlock(const QTextBlock& _block)
{
    auto block = _block.next();
    while (block.isValid() && !isNavigableBlock(block)) {
        block = block.next();
    }
    return block;
}

QTextBlock previousNavigableBlock(const QTextBlock& _block)
{
    auto block = _block.previous();
    while (block.isValid() && !isNavigableBlock(block)) {
        block = block.previous();
    }
    return block;
}

QTextBlock firstNavigableBlock(const QTextDocument* _document)
{
    const auto block = _document->firstBlock();
    return isNavigableBlock(block) ? block : nextNavigableBlock(block);
}

QTextBlock lastNavigableBlock(const QTextDocument* _document)
{
    const auto block = _document->lastBlock();
    return isNavigableBlock(block) ? block : previousNavigableBlock(block);
}

QTextBlock navigableBlock(const QTextDocument* _document, int _position)
{
    const auto block
        = _document->findBlock(qBound(0, _position, qMax(0, _document->characterCount() - 1)));
    if (isNavigableBlock(block)) {
        return block;
    }

    auto candidate = nextNavigableBlock(block);
    if (!candidate.isValid()) {
        candidate = previousNavigableBlock(block);
    }
    return candidate.isValid() ? candidate : _document->firstBlock();
}

int navigableBlocksCount(const QTextDocument* _document)
{
    int count = 0;
    for (auto block = _document->firstBlock(); block.isValid(); block = block.next()) {
        if (isNavigableBlock(block)) {
            ++count;
        }
    }
    return count;
}

QTextBlock navigableBlockByNumber(const QTextDocument* _document, int _number)
{
    auto block = firstNavigableBlock(_document);
    for (int number = 1; number < _number; ++number) {
        const auto next = nextNavigableBlock(block);
        if (!next.isValid()) {
            break;
        }
        block = next;
    }
    return block.isValid() ? block : _document->firstBlock();
}

int navigableBlockNumber(const QTextBlock& _block)
{
    int number = 1;
    for (auto block = previousNavigableBlock(_block); block.isValid();
         block = previousNavigableBlock(block)) {
        ++number;
    }
    return number;
}

int lineEndPosition(const QTextBlock& _block)
{
    return _block.position() + lineLength(_block);
}

int firstNonBlankPosition(const QTextBlock& _block)
{
    const auto text = _block.text();
    int column = 0;
    while (column < text.size() && text.at(column).isSpace()) {
        ++column;
    }
    return _block.position() + column;
}


// ****


Cursor::Cursor(const QTextDocument* _document, int _position)
    : m_document(_document)
{
    setPosition(_position);
}

int Cursor::position() const
{
    return m_block.position() + m_column;
}

void Cursor::setPosition(int _position)
{
    setBlock(navigableBlock(m_document, _position));
    m_column = qBound(0, _position - m_block.position(), m_text.size());
}

QTextBlock Cursor::block() const
{
    return m_block;
}

int Cursor::column() const
{
    return m_column;
}

void Cursor::setColumn(int _column)
{
    m_column = qBound(0, _column, m_text.size());
    //
    // ... не встаём посреди суррогатной пары
    //
    if (m_column > 0 && m_column < m_text.size() && m_text.at(m_column).isLowSurrogate()
        && m_text.at(m_column - 1).isHighSurrogate()) {
        --m_column;
    }
}

int Cursor::lineLength() const
{
    return m_text.size();
}

bool Cursor::isLineEmpty() const
{
    return m_text.isEmpty();
}

bool Cursor::atLineEnd() const
{
    return m_column >= m_text.size();
}

QChar Cursor::character() const
{
    return atLineEnd() ? QChar() : m_text.at(m_column);
}

int Cursor::characterClass(bool _bigWord) const
{
    return Vim::characterClass(character(), _bigWord);
}

int Cursor::inc()
{
    if (m_column < m_text.size()) {
        ++m_column;
        if (m_column < m_text.size() && m_text.at(m_column).isLowSurrogate()
            && m_text.at(m_column - 1).isHighSurrogate()) {
            ++m_column;
        }
        return m_column < m_text.size() ? 0 : 2;
    }

    const auto next = nextNavigableBlock(m_block);
    if (!next.isValid()) {
        return -1;
    }

    setBlock(next);
    m_column = 0;
    return 1;
}

int Cursor::dec()
{
    if (m_column > 0) {
        setColumn(m_column - 1);
        return 0;
    }

    const auto previous = previousNavigableBlock(m_block);
    if (!previous.isValid()) {
        return -1;
    }

    setBlock(previous);
    m_column = m_text.size();
    return 1;
}

int Cursor::incl()
{
    int result = inc();
    if (result >= 1 && m_column > 0) {
        result = inc();
    }
    return result;
}

int Cursor::decl()
{
    int result = dec();
    if (result == 1 && m_column > 0) {
        result = dec();
    }
    return result;
}

bool Cursor::inIndent(int _extra) const
{
    int indent = 0;
    while (indent < m_text.size() && isWhite(m_text.at(indent))) {
        ++indent;
    }
    return indent >= m_column + _extra;
}

void Cursor::setBlock(const QTextBlock& _block)
{
    m_block = _block;
    m_text = _block.text();
}


// ****


bool forwardWord(Cursor& _cursor, int _count, bool _bigWord, bool _stopAtLineEnd)
{
    for (int count = _count - 1; count >= 0; --count) {
        const int startClass = _cursor.characterClass(_bigWord);
        const bool lastLine = !nextNavigableBlock(_cursor.block()).isValid();
        int moved = _cursor.inc();
        //
        // ... стояли на последнем символе документа
        //
        if (moved == -1 || (moved >= 1 && lastLine)) {
            return false;
        }
        //
        // ... стояли на последнем символе строки
        //
        if (moved >= 1 && _stopAtLineEnd && count == 0) {
            return true;
        }

        //
        // ... переходим за конец текущего слова
        //
        if (startClass != 0) {
            while (_cursor.characterClass(_bigWord) == startClass) {
                moved = _cursor.inc();
                if (moved == -1 || (moved >= 1 && _stopAtLineEnd && count == 0)) {
                    return true;
                }
            }
        }

        //
        // ... и к следующему непробельному символу, останавливаясь на пустой строке
        //
        while (_cursor.characterClass(_bigWord) == 0) {
            if (_cursor.column() == 0 && _cursor.isLineEmpty()) {
                break;
            }
            moved = _cursor.inc();
            if (moved == -1 || (moved >= 1 && _stopAtLineEnd && count == 0)) {
                return true;
            }
        }
    }
    return true;
}

bool backwardWord(Cursor& _cursor, int _count, bool _bigWord, bool _stop)
{
    bool stop = _stop;
    for (int count = _count; count > 0; --count) {
        const int startClass = _cursor.characterClass(_bigWord);
        if (_cursor.dec() == -1) {
            return false;
        }

        bool finished = false;
        if (!stop || startClass == _cursor.characterClass(_bigWord) || startClass == 0) {
            //
            // ... пропускаем пробелы перед словом, останавливаясь на пустой строке
            //
            while (_cursor.characterClass(_bigWord) == 0) {
                if (_cursor.column() == 0 && _cursor.isLineEmpty()) {
                    finished = true;
                    break;
                }
                if (_cursor.dec() == -1) {
                    return true;
                }
            }

            //
            // ... и переходим к началу слова
            //
            if (!finished
                && skipCharacters(_cursor, _cursor.characterClass(_bigWord), _bigWord, false)) {
                return true;
            }
        }

        if (!finished) {
            _cursor.inc();
        }
        stop = false;
    }
    return true;
}

bool endOfWord(Cursor& _cursor, int _count, bool _bigWord, bool _stop, bool _stopAtEmptyLine)
{
    bool stop = _stop;
    for (int count = _count; count > 0; --count) {
        const int startClass = _cursor.characterClass(_bigWord);
        if (_cursor.inc() == -1) {
            return false;
        }

        bool finished = false;
        //
        // ... если курсор посреди слова, то просто идём к его концу
        //
        if (_cursor.characterClass(_bigWord) == startClass && startClass != 0) {
            if (skipCharacters(_cursor, startClass, _bigWord, true)) {
                return false;
            }
        }
        //
        // ... а если в конце, то к концу следующего слова
        //
        else if (!stop || startClass == 0) {
            while (_cursor.characterClass(_bigWord) == 0) {
                if (_cursor.column() == 0 && _cursor.isLineEmpty() && _stopAtEmptyLine) {
                    finished = true;
                    break;
                }
                if (_cursor.inc() == -1) {
                    return false;
                }
            }

            if (!finished
                && skipCharacters(_cursor, _cursor.characterClass(_bigWord), _bigWord, true)) {
                return false;
            }
        }

        if (!finished) {
            _cursor.dec();
        }
        stop = false;
    }
    return true;
}

bool backwardEndOfWord(Cursor& _cursor, int _count, bool _bigWord, bool _stopAtLineEnd)
{
    for (int count = _count; count > 0; --count) {
        const int startClass = _cursor.characterClass(_bigWord);
        int moved = _cursor.dec();
        if (moved == -1) {
            return false;
        }
        if (_stopAtLineEnd && moved == 1) {
            return true;
        }

        //
        // ... переходим к началу текущего слова
        //
        if (startClass != 0) {
            while (_cursor.characterClass(_bigWord) == startClass) {
                moved = _cursor.dec();
                if (moved == -1 || (_stopAtLineEnd && moved == 1)) {
                    return true;
                }
            }
        }

        //
        // ... и к концу предыдущего
        //
        while (_cursor.characterClass(_bigWord) == 0) {
            if (_cursor.column() == 0 && _cursor.isLineEmpty()) {
                break;
            }
            moved = _cursor.dec();
            if (moved == -1 || (_stopAtLineEnd && moved == 1)) {
                return true;
            }
        }
    }
    return true;
}

int matchingBracketPosition(const QTextDocument* _document, int _position)
{
    const QString openBrackets = QStringLiteral("([{");
    const QString closeBrackets = QStringLiteral(")]}");

    //
    // Ищем ближайшую скобку в строке, начиная с позиции курсора
    //
    Cursor cursor(_document, _position);
    while (!cursor.atLineEnd() && !openBrackets.contains(cursor.character())
           && !closeBrackets.contains(cursor.character())) {
        cursor.inc();
    }
    if (cursor.atLineEnd()) {
        return -1;
    }

    //
    // ... и идём к парной ей
    //
    const auto bracket = cursor.character();
    const int openIndex = openBrackets.indexOf(bracket);
    const bool forward = openIndex >= 0;
    const auto openBracket = forward ? bracket : openBrackets.at(closeBrackets.indexOf(bracket));
    const auto closeBracket = forward ? closeBrackets.at(openIndex) : bracket;
    return findUnmatchedBracket(_document, cursor.position(), openBracket, closeBracket, forward);
}

int paragraphPosition(const QTextDocument* _document, int _position, bool _forward, int _count,
                      bool* _inclusive)
{
    if (_inclusive != nullptr) {
        *_inclusive = false;
    }

    auto line = navigableBlock(_document, _position);
    for (int count = _count - 1; count >= 0; --count) {
        bool didSkip = false;
        for (bool first = true;; first = false) {
            if (lineLength(line) > 0) {
                didSkip = true;
            }
            if (!first && didSkip && lineLength(line) == 0) {
                break;
            }

            const auto next = _forward ? nextNavigableBlock(line) : previousNavigableBlock(line);
            if (!next.isValid()) {
                if (count > 0) {
                    return -1;
                }
                break;
            }
            line = next;
        }
    }

    //
    // ... в конце документа курсор встаёт на последний символ, который попадает в диапазон
    //
    if (_forward && !nextNavigableBlock(line).isValid() && lineLength(line) > 0) {
        if (_inclusive != nullptr) {
            *_inclusive = true;
        }
        return lineEndPosition(line) - 1;
    }

    return line.position();
}

ObjectRange wordObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                       int _count, bool _around, bool _bigWord)
{
    Cursor cursor(_document, _cursor);
    int visualStart = _start;
    int startPosition = -1;
    bool inclusive = true;
    bool includeWhite = false;
    int count = qMax(1, _count);

    //
    // Если выделения нет, то выделяем слово, или пробелы под курсором
    //
    if (!_visual || _start == _cursor) {
        backInLine(cursor, _bigWord);
        startPosition = cursor.position();
        if ((cursor.characterClass(_bigWord) == 0) == _around) {
            if (!endOfWord(cursor, 1, _bigWord, true, true)) {
                return {};
            }
        } else {
            forwardWord(cursor, 1, _bigWord, true);
            if (cursor.column() == 0) {
                cursor.decl();
            } else {
                oneLeft(cursor);
            }
            if (_around) {
                includeWhite = true;
            }
        }

        if (_visual) {
            visualStart = startPosition;
        }
        --count;
    }

    //
    // ... и расширяем выделение на заданное количество объектов
    //
    while (count > 0) {
        inclusive = true;
        if (_visual && cursor.position() < visualStart) {
            if (cursor.decl() == -1) {
                return {};
            }
            if (_around != (cursor.characterClass(_bigWord) != 0)) {
                if (!backwardWord(cursor, 1, _bigWord, true)) {
                    return {};
                }
            } else {
                if (!backwardEndOfWord(cursor, 1, _bigWord, true)) {
                    return {};
                }
                cursor.incl();
            }
        } else {
            if (cursor.incl() == -1) {
                return {};
            }
            if (_around != (cursor.characterClass(_bigWord) == 0)) {
                if (!forwardWord(cursor, 1, _bigWord, true) && count > 1) {
                    return {};
                }
                if (!oneLeft(cursor)) {
                    inclusive = false;
                }
            } else {
                if (!endOfWord(cursor, 1, _bigWord, true, true)) {
                    return {};
                }
            }
        }
        --count;
    }

    //
    // Если пробелы за словом не попали в объект, то захватываем пробелы перед ним, но не отступ
    //
    if (includeWhite
        && (cursor.characterClass(_bigWord) != 0 || (cursor.column() == 0 && !inclusive))) {
        Cursor startCursor(_document, startPosition);
        if (oneLeft(startCursor)) {
            backInLine(startCursor, _bigWord);
            if (startCursor.characterClass(_bigWord) == 0 && startCursor.column() > 0) {
                if (_visual) {
                    visualStart = startCursor.position();
                } else {
                    startPosition = startCursor.position();
                }
            }
        }
    }

    return { _visual ? visualStart : startPosition, cursor.position(), inclusive, false };
}

ObjectRange bracketObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                          int _count, bool _around, QChar _openBracket, QChar _closeBracket)
{
    int oldStart = _cursor;
    int oldEnd = _cursor;
    Cursor cursor(_document, _cursor);
    if (!_visual || _start == _cursor) {
        //
        // ... отступ перед фигурной скобкой не учитываем
        //
        if (_openBracket == '{') {
            while (cursor.inIndent(1)) {
                if (cursor.inc() != 0) {
                    break;
                }
            }
        }
        //
        // ... если курсор стоит на открывающей скобке, то ищем от следующего за ней символа
        //
        if (cursor.character() == _openBracket) {
            cursor.setColumn(cursor.column() + 1);
        }
    } else if (_start < _cursor) {
        oldStart = _start;
        cursor.setPosition(_start);
    } else {
        oldEnd = _start;
    }

    //
    // Ищем незакрытую открывающую скобку перед курсором, а если её нет, то следующую в строке
    //
    int startPosition = -1;
    int position
        = findUnmatchedBracket(_document, cursor.position(), _openBracket, _closeBracket, false);
    const bool searchForward = position < 0;
    for (int count = qMax(1, _count); count > 0; --count) {
        position = searchForward
            ? findNextOpenBracket(_document, cursor.position(), _openBracket, _closeBracket)
            : findUnmatchedBracket(_document, cursor.position(), _openBracket, _closeBracket,
                                   false);
        if (position < 0) {
            break;
        }
        cursor.setPosition(position);
        startPosition = position;
    }
    if (position < 0) {
        return {};
    }

    //
    // ... и парную ей закрывающую
    //
    const int endPosition
        = findUnmatchedBracket(_document, cursor.position(), _openBracket, _closeBracket, true);
    if (endPosition < 0) {
        return {};
    }
    cursor.setPosition(endPosition);

    //
    // Исключаем сами скобки, а если закрывающей скобке предшествует только отступ, то и его
    //
    bool startOfLine = false;
    Cursor startCursor(_document, startPosition);
    while (!_around) {
        startCursor.incl();
        startOfLine = cursor.column() == 0;
        cursor.decl();
        while (cursor.inIndent(1)) {
            startOfLine = true;
            if (cursor.decl() != 0) {
                break;
            }
        }

        //
        // ... в визуальном режиме пустые скобки не выделяются
        //
        if (_visual && startCursor.position() == endPosition) {
            return {};
        }

        //
        // ... в визуальном режиме, если выделение не увеличилось, то переходим к внешним скобкам
        //
        if (_visual && startCursor.position() >= oldStart && oldEnd >= cursor.position()
            && startCursor.position() != cursor.position()) {
            cursor.setPosition(oldStart);
            cursor.decl();
            const int outerStart = findUnmatchedBracket(_document, cursor.position(), _openBracket,
                                                        _closeBracket, false);
            if (outerStart < 0) {
                return {};
            }
            startCursor.setPosition(outerStart);
            cursor.setPosition(outerStart);
            const int outerEnd = findUnmatchedBracket(_document, cursor.position(), _openBracket,
                                                      _closeBracket, true);
            if (outerEnd < 0) {
                return {};
            }
            cursor.setPosition(outerEnd);
        } else {
            break;
        }
    }

    if (_visual) {
        if (startOfLine && !cursor.atLineEnd()) {
            cursor.inc();
        }
        return { startCursor.position(), cursor.position(), true, false };
    }

    ObjectRange range{ startCursor.position(), cursor.position(), false, false };
    if (startOfLine) {
        cursor.incl();
        range.cursor = cursor.position();
    } else if (startCursor.position() <= cursor.position()) {
        range.inclusive = true;
    } else {
        range.cursor = startCursor.position();
    }
    return range;
}

ObjectRange quoteObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                        int _count, bool _around, QChar _quote)
{
    const Cursor cursor(_document, _cursor);
    const auto line = cursor.block().text();
    const int lineStart = cursor.block().position();
    int columnStart = cursor.column();
    int columnEnd = -1;
    bool visualEmpty = true;
    bool visualBeforeCursor = false;
    bool insideQuotes = false;
    bool selectedQuote = false;
    int visualColumn = columnStart;

    if (_visual) {
        //
        // ... объект работает только в пределах одной строки
        //
        const Cursor startCursor(_document, _start);
        if (startCursor.block() != cursor.block()) {
            return {};
        }
        visualColumn = startCursor.column();
        visualBeforeCursor = _start < _cursor;
        visualEmpty = _start == _cursor;
        if (!visualEmpty) {
            int column = 0;
            if (visualBeforeCursor) {
                insideQuotes = visualColumn > 0 && characterAt(line, visualColumn - 1) == _quote
                    && !characterAt(line, cursor.column()).isNull()
                    && characterAt(line, cursor.column() + 1) == _quote;
                column = visualColumn;
                columnEnd = cursor.column();
            } else {
                insideQuotes = cursor.column() > 0
                    && characterAt(line, cursor.column() - 1) == _quote
                    && !characterAt(line, visualColumn).isNull()
                    && characterAt(line, visualColumn + 1) == _quote;
                column = cursor.column();
                columnEnd = visualColumn;
            }
            while (column <= columnEnd && column < line.size()) {
                if (line.at(column++) == _quote) {
                    selectedQuote = true;
                    break;
                }
            }
        }
    }

    if (!visualEmpty && characterAt(line, columnStart) == _quote) {
        //
        // ... уже что-то выделено и курсор на кавычке - ищем следующую строку в кавычках
        //
        if (visualBeforeCursor) {
            columnStart = findNextQuote(line, columnStart + 1, _quote, false);
            if (columnStart < 0) {
                return {};
            }
            columnEnd = findNextQuote(line, columnStart + 1, _quote, true);
            if (columnEnd < 0) {
                columnEnd = columnStart;
                columnStart = cursor.column();
            }
        } else {
            columnEnd = findPreviousQuote(line, columnStart, _quote, false);
            if (characterAt(line, columnEnd) != _quote) {
                return {};
            }
            columnStart = findPreviousQuote(line, columnEnd, _quote, true);
            if (characterAt(line, columnStart) != _quote) {
                columnStart = columnEnd;
                columnEnd = cursor.column();
            }
        }
    } else if (characterAt(line, columnStart) == _quote || !visualEmpty) {
        //
        // ... курсор на кавычке, но непонятно, открывающая она, или закрывающая, поэтому
        //     ищем от начала строки
        //
        int firstColumn = columnStart;
        if (!visualEmpty) {
            firstColumn = visualBeforeCursor ? findNextQuote(line, columnStart, _quote, false)
                                             : findPreviousQuote(line, columnStart, _quote, false);
        }
        columnStart = 0;
        forever
        {
            columnStart = findNextQuote(line, columnStart, _quote, false);
            if (columnStart < 0 || columnStart > firstColumn) {
                return {};
            }
            columnEnd = findNextQuote(line, columnStart + 1, _quote, true);
            if (columnEnd < 0) {
                return {};
            }
            if (columnStart <= firstColumn && firstColumn <= columnEnd) {
                break;
            }
            columnStart = columnEnd + 1;
        }
    } else {
        //
        // ... ищем открывающую кавычку перед курсором, а если её нет, то после него
        //
        columnStart = findPreviousQuote(line, columnStart, _quote, true);
        if (characterAt(line, columnStart) != _quote) {
            columnStart = findNextQuote(line, columnStart, _quote, false);
            if (columnStart < 0) {
                return {};
            }
        }
        columnEnd = findNextQuote(line, columnStart + 1, _quote, true);
        if (columnEnd < 0) {
            return {};
        }
    }

    //
    // Объект с кавычками захватывает пробелы за ними, а если их нет, то перед ними
    //
    if (_around) {
        if (isWhite(characterAt(line, columnEnd + 1))) {
            while (isWhite(characterAt(line, columnEnd + 1))) {
                ++columnEnd;
            }
        } else {
            while (columnStart > 0 && isWhite(line.at(columnStart - 1))) {
                --columnStart;
            }
        }
    }

    //
    // ... после vi" повторное i" захватывает и сами кавычки
    //
    if (!_around && _count < 2 && (visualEmpty || !insideQuotes)) {
        ++columnStart;
    }
    int visualStart = _start;
    if (_visual
        && (visualEmpty
            || (visualBeforeCursor && !selectedQuote
                && (insideQuotes
                    || (characterAt(line, visualColumn) != _quote
                        && (visualColumn == 0
                            || characterAt(line, visualColumn - 1) != _quote)))))) {
        visualStart = lineStart + columnStart;
    }

    //
    // ... и конец объекта
    //
    int cursorColumn = columnEnd;
    if (_around || _count > 1 || (!visualEmpty && insideQuotes)) {
        cursorColumn = qMin(cursorColumn + 1, static_cast<int>(line.size()));
    }

    if (_visual) {
        if (visualEmpty || visualBeforeCursor) {
            return { visualStart, lineStart + qMax(0, cursorColumn - 1), true, false };
        }

        //
        // ... курсор в начале выделения
        //
        if (insideQuotes
            || (!selectedQuote && characterAt(line, visualColumn) != _quote
                && (characterAt(line, visualColumn).isNull()
                    || characterAt(line, visualColumn + 1) != _quote))) {
            visualStart = lineStart + qMax(0, cursorColumn - 1);
        }
        return { visualStart, lineStart + columnStart, true, false };
    }

    return { lineStart + columnStart, lineStart + cursorColumn, false, false };
}

bool findSentence(Cursor& _cursor, bool _forward, int _count)
{
    const QString sentenceEnds = QStringLiteral(".!?");
    const QString closingCharacters = QStringLiteral(")]\"'");
    auto isLastLine
        = [](const QTextBlock& _block) { return !nextNavigableBlock(_block).isValid(); };

    Cursor position = _cursor;
    int count = _count;
    while (count-- > 0) {
        const int previousPosition = position.position();
        bool noSkip = false;
        bool found = false;

        //
        // ... с пустой строки идём к первой непустой
        //
        if (position.character().isNull()) {
            do {
                if ((_forward ? position.incl() : position.decl()) == -1) {
                    break;
                }
            } while (position.character().isNull());
            if (_forward) {
                found = true;
            }
        }
        //
        // ... из начала абзаца вперёд - к следующей строке
        //
        else if (_forward && position.column() == 0 && position.isLineEmpty()) {
            if (isLastLine(position.block())) {
                return false;
            }
            position.setPosition(nextNavigableBlock(position.block()).position());
            found = true;
        } else if (!_forward) {
            position.decl();
        }

        if (!found) {
            //
            // ... возвращаемся к последнему символу, не являющемуся пробелом или знаком конца
            //     предложения
            //
            bool foundDot = false;
            forever
            {
                const auto character = position.character();
                if (!isWhite(character) && !sentenceEnds.contains(character)
                    && !closingCharacters.contains(character)) {
                    break;
                }
                Cursor previous = position;
                if (previous.decl() == -1 || (previous.isLineEmpty() && _forward)) {
                    break;
                }
                if (foundDot) {
                    break;
                }
                if (sentenceEnds.contains(character)) {
                    foundDot = true;
                }
                if (closingCharacters.contains(character)
                    && !sentenceEnds.contains(previous.character())
                    && !closingCharacters.contains(previous.character())) {
                    break;
                }
                position.decl();
            }

            //
            // ... и ищем конец предложения
            //
            const auto startLine = position.block();
            forever
            {
                const auto character = position.character();
                if (character.isNull() || (position.column() == 0 && position.isLineEmpty())) {
                    if (!_forward && position.block() != startLine) {
                        position.setPosition(nextNavigableBlock(position.block()).position());
                    }
                    break;
                }
                if (sentenceEnds.contains(character)) {
                    Cursor next = position;
                    int moved = 0;
                    do {
                        moved = next.inc();
                        if (moved == -1) {
                            break;
                        }
                    } while (closingCharacters.contains(next.character()));
                    if (moved == -1 || isWhite(next.character()) || next.character().isNull()) {
                        position = next;
                        if (position.character().isNull()) {
                            position.inc();
                        }
                        break;
                    }
                }
                if ((_forward ? position.incl() : position.decl()) == -1) {
                    if (count > 0) {
                        return false;
                    }
                    noSkip = true;
                    break;
                }
            }
        }

        //
        // ... пропускаем пробелы
        //
        while (!noSkip && isWhite(position.character())) {
            if (position.incl() == -1) {
                break;
            }
        }

        //
        // ... если так и не сдвинулись, то шагаем на символ и пробуем ещё раз
        //
        if (position.position() == previousPosition) {
            if ((_forward ? position.incl() : position.decl()) == -1) {
                if (count > 0) {
                    return false;
                }
                break;
            }
            ++count;
        }
    }

    _cursor = position;
    return true;
}

ObjectRange sentenceObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                           int _count, bool _around)
{
    Cursor startPosition(_document, _cursor);
    Cursor position = startPosition;
    Cursor cursor = startPosition;
    const int count = qMax(1, _count);
    findSentence(cursor, true, 1);

    //
    // Расширение уже имеющегося выделения
    //
    auto extend = [&]() -> ObjectRange {
        if (startPosition.position() < _start) {
            //
            // ... курсор в начале выделения: определяем, где мы - в пробелах перед предложением,
            //     в предложении, или в самом его начале
            //
            bool atStartOfSentence = true;
            position.decl();
            while (position.position() < cursor.position()) {
                if (!isWhite(position.character())) {
                    atStartOfSentence = false;
                    break;
                }
                position.incl();
            }
            if (!atStartOfSentence) {
                findSentence(cursor, false, 1);
                if (cursor.position() == startPosition.position()) {
                    atStartOfSentence = true;
                } else {
                    findSentence(cursor, true, 1);
                }
            }
            int steps = _around ? count * 2 : count;
            while (steps-- > 0) {
                if (atStartOfSentence) {
                    findFirstBlank(cursor);
                }
                if (!atStartOfSentence || (!_around && !isWhite(cursor.character()))) {
                    findSentence(cursor, false, 1);
                }
                atStartOfSentence = !atStartOfSentence;
            }
        } else {
            //
            // ... курсор в конце выделения: определяем, где мы - прямо перед предложением, в
            //     пробелах перед ним, или в самом предложении
            //
            position.incl();
            bool atStartOfSentence = true;
            if (position.position() != cursor.position()) {
                atStartOfSentence = false;
                while (position.position() < cursor.position()) {
                    if (!isWhite(position.character())) {
                        atStartOfSentence = true;
                        break;
                    }
                    position.incl();
                }
                if (atStartOfSentence) {
                    findSentence(cursor, false, 1);
                } else {
                    cursor = startPosition;
                }
            }
            findSentenceForward(cursor, _around ? count * 2 : count, atStartOfSentence);
        }
        return { _start, cursor.position(), true, false };
    };

    if (_visual && _start != _cursor) {
        return extend();
    }

    //
    // Если курсор на пробелах, то проверяем, не стоят ли они перед следующим предложением
    //
    while (isWhite(position.character())) {
        position.incl();
    }
    bool startBlank = false;
    if (position.position() == cursor.position()) {
        startBlank = true;
        findFirstBlank(startPosition);
    } else {
        findSentence(cursor, false, 1);
        startPosition = cursor;
    }

    const int steps = _around ? count * 2 : (startBlank ? count - 1 : count);
    if (steps > 0) {
        findSentenceForward(cursor, steps, true);
    } else {
        cursor.decl();
    }

    //
    // Если пробелы перед предложением попали в объект, то пробелы после него исключаем, а если
    // пробелов после него нет, то захватываем пробелы перед ним
    //
    if (_around) {
        if (startBlank) {
            findFirstBlank(cursor);
            if (isWhite(cursor.character())) {
                cursor.decl();
            }
        } else if (!isWhite(cursor.character())) {
            findFirstBlank(startPosition);
        }
    }

    if (_visual) {
        //
        // ... чтобы не застрять на единственном пробеле перед предложением, расширяем выделение
        //
        if (startPosition.position() == cursor.position()) {
            return extend();
        }
        return { startPosition.position(), cursor.position(), true, false };
    }

    //
    // ... перевод строки после предложения тоже попадает в объект
    //
    ObjectRange range{ startPosition.position(), cursor.position(), false, false };
    if (cursor.incl() == -1) {
        range.inclusive = true;
    } else {
        range.cursor = cursor.position();
    }
    return range;
}

ObjectRange paragraphObject(const QTextDocument* _document, int _start, int _cursor, bool _visual,
                            bool _visualLine, int _count, bool _around)
{
    //
    // Работаем с номерами строк, как это делает vim, учитывая только строки, по которым может
    // перемещаться курсор
    //
    QVector<QTextBlock> lines;
    for (auto block = firstNavigableBlock(_document); block.isValid();
         block = nextNavigableBlock(block)) {
        lines.append(block);
    }
    if (lines.isEmpty()) {
        return {};
    }
    const int lineCount = lines.size();
    auto lineNumber = [&lines](const QTextBlock& _block) {
        const auto iter = std::lower_bound(lines.cbegin(), lines.cend(), _block,
                                           [](const QTextBlock& _lhs, const QTextBlock& _rhs) {
                                               return _lhs.blockNumber() < _rhs.blockNumber();
                                           });
        return static_cast<int>(std::distance(lines.cbegin(), iter)) + 1;
    };
    auto isWhiteLine = [&lines](int _line) { return isLineWhite(lines.at(_line - 1)); };
    auto isParagraphStart = [&lines](int _line) { return lineLength(lines.at(_line - 1)) == 0; };

    const int count = qMax(1, _count);
    const int cursorLine = lineNumber(navigableBlock(_document, _cursor));
    const int visualLine = lineNumber(navigableBlock(_document, _start));
    int startLine = cursorLine;

    //
    // Расширение выделения на заданное количество абзацев
    //
    auto extend = [&]() -> ObjectRange {
        const int direction = startLine < visualLine ? -1 : 1;
        const int edgeLine = direction < 0 ? 1 : lineCount;
        bool success = true;
        for (int index = count; --index >= 0;) {
            if (startLine == edgeLine) {
                success = false;
                break;
            }

            int previousStartIsWhite = -1;
            for (int round = 0; round < 2; ++round) {
                startLine += direction;
                const int startIsWhite = isWhiteLine(startLine) ? 1 : 0;
                if (previousStartIsWhite == startIsWhite) {
                    startLine -= direction;
                    break;
                }
                forever
                {
                    if (startLine == edgeLine) {
                        break;
                    }
                    if (startIsWhite != (isWhiteLine(startLine + direction) ? 1 : 0)
                        || (!startIsWhite
                            && isParagraphStart(startLine + (direction > 0 ? 1 : 0)))) {
                        break;
                    }
                    startLine += direction;
                }
                if (!_around || startLine == edgeLine) {
                    break;
                }
                previousStartIsWhite = startIsWhite;
            }
        }
        if (!success && startLine == cursorLine) {
            return {};
        }
        return { _start, lines.at(startLine - 1).position(), true, true };
    };

    if (_visual && startLine != visualLine) {
        return extend();
    }

    //
    // Переходим к началу абзаца, или пустых строк
    //
    const bool whiteInFront = isWhiteLine(startLine);
    while (startLine > 1) {
        if (whiteInFront) {
            if (!isWhiteLine(startLine - 1)) {
                break;
            }
        } else if (isWhiteLine(startLine - 1) || isParagraphStart(startLine)) {
            break;
        }
        --startLine;
    }

    //
    // ... пропускаем пустые строки
    //
    int endLine = startLine;
    while (endLine <= lineCount && isWhiteLine(endLine)) {
        ++endLine;
    }
    --endLine;

    //
    // ... и идём к концу заданного количества абзацев
    //
    int index = count;
    if (!_around && whiteInFront) {
        --index;
    }
    while (index-- > 0) {
        if (endLine == lineCount) {
            return {};
        }

        bool doWhite = false;
        if (!_around) {
            doWhite = isWhiteLine(endLine + 1);
        }

        if (_around || !doWhite) {
            ++endLine;
            while (endLine < lineCount && !isWhiteLine(endLine + 1)
                   && !isParagraphStart(endLine + 1)) {
                ++endLine;
            }
        }

        if (index == 0 && whiteInFront && _around) {
            break;
        }

        if (_around || doWhite) {
            while (endLine < lineCount && isWhiteLine(endLine + 1)) {
                ++endLine;
            }
        }
    }

    //
    // Если за абзацем нет пустых строк, то захватываем пустые строки перед ним
    //
    if (!whiteInFront && !isWhiteLine(endLine) && _around) {
        while (startLine > 1 && isWhiteLine(startLine - 1)) {
            --startLine;
        }
    }

    if (_visual) {
        //
        // ... чтобы повторное применение объекта к пустой строке не давало тот же результат,
        //     расширяем выделение
        //
        if (_visualLine && startLine == cursorLine) {
            return extend();
        }
        const int start = visualLine != startLine ? lines.at(startLine - 1).position() : _start;
        return { start, lines.at(endLine - 1).position(), true, true };
    }

    return { lines.at(startLine - 1).position(), lines.at(endLine - 1).position(), true, true };
}

QString wordUnderCursor(const QTextDocument* _document, int _position, bool* _isKeyword,
                        int* _start)
{
    const Cursor cursor(_document, _position);
    const auto line = cursor.block().text();

    //
    // Сперва ищем слово, а если его нет, то любую последовательность непробельных символов
    //
    int column = 0;
    int pass = 0;
    int currentClass = 0;
    for (; pass < 2; ++pass) {
        column = cursor.column();
        while (column < line.size()) {
            currentClass = characterClass(line.at(column), false);
            if (currentClass != 0 && (pass == 1 || currentClass != 1)) {
                break;
            }
            ++column;
        }

        currentClass = characterClass(characterAt(line, column), false);
        while (column > 0 && currentClass != 0) {
            const int previousClass = characterClass(line.at(column - 1), false);
            if (currentClass != previousClass) {
                break;
            }
            --column;
        }

        if (currentClass == 2) {
            break;
        }
    }

    if (column >= line.size() || currentClass == 0) {
        return {};
    }

    int end = column;
    while (end < line.size()
           && (pass == 0 ? characterClass(line.at(end), false) == currentClass
                         : characterClass(line.at(end), false) != 0)) {
        ++end;
    }

    if (_isKeyword != nullptr) {
        *_isKeyword = currentClass == 2;
    }
    if (_start != nullptr) {
        *_start = cursor.block().position() + column;
    }
    return line.mid(column, end - column);
}

QRegularExpression regularExpression(const QString& _pattern)
{
    //
    // Режимы vim: very magic, magic, nomagic и very nomagic
    //
    enum class Magic {
        VeryMagic,
        Magic,
        NoMagic,
        VeryNoMagic,
    };
    Magic magic = Magic::Magic;
    bool ignoreCase = false;

    QString result;
    QString lookaheadSuffix;
    bool atBranchStart = true;
    for (int index = 0; index < _pattern.size(); ++index) {
        const auto character = _pattern.at(index);
        const bool escaped = character == '\\' && index + 1 < _pattern.size();
        const auto next = escaped ? _pattern.at(index + 1) : QChar();
        if (escaped) {
            ++index;
        }

        //
        // ... переключатели режимов и регистра
        //
        if (escaped) {
            if (next == 'v') {
                magic = Magic::VeryMagic;
                continue;
            } else if (next == 'm') {
                magic = Magic::Magic;
                continue;
            } else if (next == 'M') {
                magic = Magic::NoMagic;
                continue;
            } else if (next == 'V') {
                magic = Magic::VeryNoMagic;
                continue;
            } else if (next == 'c') {
                ignoreCase = true;
                continue;
            } else if (next == 'C') {
                continue;
            }
        }

        //
        // ... определяем, является ли символ специальным в текущем режиме
        //
        const auto symbol = escaped ? next : character;
        auto isSpecial
            = [&](const QString& _veryMagic, const QString& _magic, const QString& _noMagic) {
                  switch (magic) {
                  case Magic::VeryMagic: {
                      return escaped ? false : _veryMagic.contains(symbol);
                  }
                  case Magic::Magic: {
                      return escaped ? !_magic.contains(symbol) : _magic.contains(symbol);
                  }
                  case Magic::NoMagic: {
                      return escaped ? !_noMagic.contains(symbol) : _noMagic.contains(symbol);
                  }
                  case Magic::VeryNoMagic:
                  default: {
                      return escaped;
                  }
                  }
              };

        const bool wasBranchStart = atBranchStart;
        atBranchStart = false;

        //
        // ... группы и альтернативы
        //
        if ((symbol == '(' || symbol == ')' || symbol == '|')
            && isSpecial(QStringLiteral("()|"), QString(), QString())) {
            if (symbol == '(' && index + 0 < _pattern.size()) {
                result += '(';
            } else {
                result += symbol;
            }
            atBranchStart = symbol != ')';
            continue;
        }
        if (symbol == '%' && isSpecial(QStringLiteral("%"), QString(), QString())
            && index + 1 < _pattern.size()) {
            //
            // ... \%( - группа без запоминания
            //
            const bool veryMagicGroup = magic == Magic::VeryMagic && _pattern.at(index + 1) == '(';
            const bool magicGroup = magic != Magic::VeryMagic && index + 2 < _pattern.size()
                && _pattern.at(index + 1) == '\\' && _pattern.at(index + 2) == '(';
            if (veryMagicGroup || magicGroup) {
                index += veryMagicGroup ? 1 : 2;
                result += QStringLiteral("(?:");
                atBranchStart = true;
                continue;
            }
        }

        //
        // ... квантификаторы
        //
        if ((symbol == '+' || symbol == '?' || symbol == '=')
            && isSpecial(QStringLiteral("+?="), QString(), QString())) {
            result += symbol == '+' ? QStringLiteral("+") : QStringLiteral("?");
            continue;
        }
        if (symbol == '*' && isSpecial(QStringLiteral("*"), QStringLiteral("*"), QString())) {
            result += wasBranchStart ? QStringLiteral("\\*") : QStringLiteral("*");
            continue;
        }
        if (symbol == '{' && isSpecial(QStringLiteral("{"), QString(), QString())) {
            int close = _pattern.indexOf('}', index + 1);
            if (close < 0) {
                result += QStringLiteral("\\{");
                continue;
            }
            auto body = _pattern.mid(index + 1, close - index - 1);
            if (body.endsWith('\\')) {
                body.chop(1);
            }
            index = close;
            bool lazy = false;
            if (body.startsWith('-')) {
                lazy = true;
                body.remove(0, 1);
            }
            if (body.isEmpty()) {
                result += lazy ? QStringLiteral("*?") : QStringLiteral("*");
            } else {
                if (body.startsWith(',')) {
                    body.prepend('0');
                }
                result
                    += QStringLiteral("{%1}%2").arg(body, lazy ? QStringLiteral("?") : QString());
            }
            continue;
        }

        //
        // ... границы слов
        //
        if ((symbol == '<' || symbol == '>')
            && isSpecial(QStringLiteral("<>"), QString(), QString())) {
            result += symbol == '<' ? QStringLiteral("\\b(?=\\w)") : QStringLiteral("\\b(?<=\\w)");
            continue;
        }

        //
        // ... любой символ и наборы символов
        //
        if (symbol == '.' && isSpecial(QStringLiteral("."), QStringLiteral("."), QString())) {
            result += '.';
            continue;
        }
        if (symbol == '[' && isSpecial(QStringLiteral("["), QStringLiteral("["), QString())) {
            int close = index + 1;
            if (close < _pattern.size() && _pattern.at(close) == '^') {
                ++close;
            }
            if (close < _pattern.size() && _pattern.at(close) == ']') {
                ++close;
            }
            while (close < _pattern.size() && _pattern.at(close) != ']') {
                if (_pattern.at(close) == '\\') {
                    ++close;
                } else if (_pattern.at(close) == '[' && close + 1 < _pattern.size()
                           && _pattern.at(close + 1) == ':') {
                    const int classEnd = _pattern.indexOf(QStringLiteral(":]"), close + 2);
                    if (classEnd > 0) {
                        close = classEnd + 1;
                    }
                }
                ++close;
            }
            if (close >= _pattern.size()) {
                result += QStringLiteral("\\[");
                continue;
            }
            auto set = _pattern.mid(index + 1, close - index - 1);
            index = close;
            result += '[';
            for (int setIndex = 0; setIndex < set.size(); ++setIndex) {
                if (set.at(setIndex) == '['
                    && !set.mid(setIndex).startsWith(QStringLiteral("[:"))) {
                    result += QStringLiteral("\\[");
                } else {
                    result += set.at(setIndex);
                }
            }
            result += ']';
            continue;
        }

        //
        // ... якоря начала и конца строки
        //
        if (symbol == '^' && !escaped && magic != Magic::VeryNoMagic) {
            result += wasBranchStart ? QStringLiteral("^") : QStringLiteral("\\^");
            if (wasBranchStart) {
                atBranchStart = true;
            }
            continue;
        }
        if (symbol == '$' && !escaped && magic != Magic::VeryNoMagic) {
            const auto rest = _pattern.mid(index + 1);
            const bool atBranchEnd = rest.isEmpty() || rest.startsWith(QStringLiteral("\\|"))
                || rest.startsWith(QStringLiteral("\\)"))
                || (magic == Magic::VeryMagic && (rest.startsWith('|') || rest.startsWith(')')));
            result += atBranchEnd ? QStringLiteral("$") : QStringLiteral("\\$");
            continue;
        }

        //
        // ... экранированные классы символов и обратные ссылки
        //
        if (escaped) {
            switch (next.unicode()) {
            case 's':
            case 'S':
            case 'd':
            case 'D':
            case 'w':
            case 'W': {
                result += '\\';
                result += next;
                continue;
            }
            case 'a': {
                result += QStringLiteral("[A-Za-z]");
                continue;
            }
            case 'A': {
                result += QStringLiteral("[^A-Za-z]");
                continue;
            }
            case 'l': {
                result += QStringLiteral("[a-z]");
                continue;
            }
            case 'L': {
                result += QStringLiteral("[^a-z]");
                continue;
            }
            case 'u': {
                result += QStringLiteral("[A-Z]");
                continue;
            }
            case 'U': {
                result += QStringLiteral("[^A-Z]");
                continue;
            }
            case 'x': {
                result += QStringLiteral("[0-9A-Fa-f]");
                continue;
            }
            case 'X': {
                result += QStringLiteral("[^0-9A-Fa-f]");
                continue;
            }
            case 'o': {
                result += QStringLiteral("[0-7]");
                continue;
            }
            case 'O': {
                result += QStringLiteral("[^0-7]");
                continue;
            }
            case 'h': {
                result += QStringLiteral("[A-Za-z_]");
                continue;
            }
            case 'H': {
                result += QStringLiteral("[^A-Za-z_]");
                continue;
            }
            case 'k':
            case 'i':
            case 'f':
            case 'p': {
                result += QStringLiteral("\\w");
                continue;
            }
            case 't': {
                result += QStringLiteral("\\t");
                continue;
            }
            case 'e': {
                result += QStringLiteral("\\x1b");
                continue;
            }
            case 'n':
            case 'r': {
                result += QStringLiteral("\\n");
                continue;
            }
            case 'z': {
                if (index + 1 < _pattern.size()) {
                    const auto zType = _pattern.at(index + 1);
                    if (zType == 's') {
                        ++index;
                        result += QStringLiteral("\\K");
                        continue;
                    }
                    if (zType == 'e') {
                        ++index;
                        result += QStringLiteral("(?=");
                        lookaheadSuffix += ')';
                        continue;
                    }
                }
                break;
            }
            default: {
                if (next.isDigit() && next != '0') {
                    result += '\\';
                    result += next;
                    continue;
                }
                break;
            }
            }
        }

        result += QRegularExpression::escape(QString(symbol));
    }
    result += lookaheadSuffix;

    QRegularExpression expression(result,
                                  ignoreCase ? QRegularExpression::CaseInsensitiveOption
                                             : QRegularExpression::NoPatternOption);
    if (!expression.isValid()) {
        expression = QRegularExpression(QRegularExpression::escape(_pattern),
                                        ignoreCase ? QRegularExpression::CaseInsensitiveOption
                                                   : QRegularExpression::NoPatternOption);
    }
    return expression;
}

QString substitution(const QRegularExpressionMatch& _match, const QString& _replacement)
{
    enum class CaseChange {
        None,
        Upper,
        Lower,
    };
    CaseChange oneCharacter = CaseChange::None;
    CaseChange allCharacters = CaseChange::None;

    QString result;
    auto append = [&](const QString& _text) {
        for (auto character : _text) {
            if (oneCharacter != CaseChange::None) {
                character
                    = oneCharacter == CaseChange::Upper ? character.toUpper() : character.toLower();
                oneCharacter = CaseChange::None;
            } else if (allCharacters != CaseChange::None) {
                character = allCharacters == CaseChange::Upper ? character.toUpper()
                                                               : character.toLower();
            }
            result += character;
        }
    };

    for (int index = 0; index < _replacement.size(); ++index) {
        const auto character = _replacement.at(index);
        if (character == '&') {
            append(_match.captured(0));
            continue;
        }
        if (character != '\\' || index + 1 >= _replacement.size()) {
            append(QString(character));
            continue;
        }

        const auto next = _replacement.at(++index);
        if (next.isDigit()) {
            append(_match.captured(next.digitValue()));
        } else if (next == 'u') {
            oneCharacter = CaseChange::Upper;
        } else if (next == 'l') {
            oneCharacter = CaseChange::Lower;
        } else if (next == 'U') {
            allCharacters = CaseChange::Upper;
        } else if (next == 'L') {
            allCharacters = CaseChange::Lower;
        } else if (next == 'e' || next == 'E') {
            allCharacters = CaseChange::None;
            oneCharacter = CaseChange::None;
        } else if (next == 'n' || next == 'r') {
            result += '\n';
        } else if (next == 't') {
            result += '\t';
        } else {
            append(QString(next));
        }
    }

    return result;
}

} // namespace Vim
} // namespace Ui

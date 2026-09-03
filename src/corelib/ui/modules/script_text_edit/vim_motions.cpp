#include "vim_motions.h"

#include <QTextBlock>
#include <QTextDocument>
#include <QVector>

namespace Ui {
namespace Vim {

namespace {

/**
 * @brief Класс символа, по которому определяются границы слов
 */
enum class CharacterClass {
    Blank,
    Word,
    Punctuation,
};

CharacterClass characterClass(QChar _character, bool _bigWord)
{
    if (_character.isNull() || _character.isSpace() || _character == QChar::ParagraphSeparator
        || _character == QChar::LineSeparator) {
        return CharacterClass::Blank;
    }

    if (_bigWord || _character.isLetterOrNumber() || _character == '_') {
        return CharacterClass::Word;
    }

    return CharacterClass::Punctuation;
}

/**
 * @brief Находится ли позиция в начале пустой строки
 * @note В vim пустая строка считается отдельным словом
 */
bool isEmptyLinePosition(const QTextDocument* _document, int _position)
{
    const auto block = _document->findBlock(_position);
    return block.isValid() && block.length() == 1 && block.position() == _position;
}

/**
 * @brief Позиция курсора внутри строки, приведённая к границам её текста
 */
int positionInBlock(const QTextBlock& _block, int _position)
{
    return qBound(0, _position - _block.position(), qMax(0, _block.text().size() - 1));
}

} // namespace


bool TextRange::isValid() const
{
    return start >= 0 && end >= start;
}

int lastPosition(const QTextDocument* _document)
{
    return qMax(0, _document->characterCount() - 1);
}

int boundedPosition(const QTextDocument* _document, int _position)
{
    return qBound(0, _position, lastPosition(_document));
}

int startOfLinePosition(const QTextDocument* _document, int _position)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    return block.isValid() ? block.position() : 0;
}

int endOfLinePosition(const QTextDocument* _document, int _position)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid()) {
        return lastPosition(_document);
    }

    return block.position() + block.length() - 1;
}

int firstNonBlankPosition(const QTextDocument* _document, int _position)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid()) {
        return boundedPosition(_document, _position);
    }

    const auto text = block.text();
    int offset = 0;
    while (offset < text.size() && text.at(offset).isSpace()) {
        ++offset;
    }
    return block.position() + offset;
}

int linePosition(const QTextDocument* _document, int _lineNumber)
{
    const auto block
        = _document->findBlockByNumber(qBound(0, _lineNumber - 1, _document->blockCount() - 1));
    return block.isValid() ? firstNonBlankPosition(_document, block.position()) : 0;
}

int nextWordPosition(const QTextDocument* _document, int _position, bool _bigWord)
{
    const int endPosition = lastPosition(_document);
    const int startIndex = boundedPosition(_document, _position);
    int index = startIndex;
    if (index >= endPosition) {
        return endPosition;
    }

    //
    // ... пропускаем текущее слово
    //
    const auto startClass = characterClass(_document->characterAt(index), _bigWord);
    if (startClass != CharacterClass::Blank) {
        while (index < endPosition
               && characterClass(_document->characterAt(index), _bigWord) == startClass) {
            ++index;
        }
    }

    //
    // ... и пробельные символы за ним, останавливаясь на пустых строках
    //
    while (index < endPosition
           && characterClass(_document->characterAt(index), _bigWord) == CharacterClass::Blank) {
        if (index != startIndex && isEmptyLinePosition(_document, index)) {
            break;
        }
        ++index;
    }

    return index;
}

int previousWordPosition(const QTextDocument* _document, int _position, bool _bigWord)
{
    int index = boundedPosition(_document, _position);
    if (index <= 0) {
        return 0;
    }

    --index;

    //
    // ... пропускаем пробельные символы назад, останавливаясь на пустых строках
    //
    while (index > 0
           && characterClass(_document->characterAt(index), _bigWord) == CharacterClass::Blank) {
        if (isEmptyLinePosition(_document, index)) {
            return index;
        }
        --index;
    }

    //
    // ... и переходим в начало слова
    //
    const auto wordClass = characterClass(_document->characterAt(index), _bigWord);
    if (wordClass != CharacterClass::Blank) {
        while (index > 0
               && characterClass(_document->characterAt(index - 1), _bigWord) == wordClass) {
            --index;
        }
    }

    return index;
}

int endOfWordPosition(const QTextDocument* _document, int _position, bool _bigWord)
{
    const int endPosition = lastPosition(_document);
    int index = boundedPosition(_document, _position);
    if (index >= endPosition) {
        return endPosition;
    }

    //
    // ... курсор в vim стоит на последнем символе слова, поэтому шагаем вперёд и пропускаем пробелы
    //
    ++index;
    while (index < endPosition
           && characterClass(_document->characterAt(index), _bigWord) == CharacterClass::Blank) {
        ++index;
    }

    const auto wordClass = characterClass(_document->characterAt(index), _bigWord);
    if (wordClass != CharacterClass::Blank) {
        while (index + 1 < endPosition
               && characterClass(_document->characterAt(index + 1), _bigWord) == wordClass) {
            ++index;
        }
    }

    return index;
}

int findCharacterPosition(const QTextDocument* _document, int _position, QChar _character,
                          bool _forward, bool _till, int _count)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid()) {
        return -1;
    }

    const auto text = block.text();
    if (text.isEmpty()) {
        return -1;
    }

    int index = qBound(-1, boundedPosition(_document, _position) - block.position(), text.size());
    for (int step = 0; step < qMax(1, _count); ++step) {
        const int found = _forward ? text.indexOf(_character, index + 1)
                                   : text.lastIndexOf(_character, index - 1);
        if (found < 0) {
            return -1;
        }
        index = found;
    }

    if (_till) {
        index += _forward ? -1 : 1;
    }
    if (index < 0 || index >= text.size()) {
        return -1;
    }

    return block.position() + index;
}

int matchingBracketPosition(const QTextDocument* _document, int _position)
{
    const QString openBrackets = QStringLiteral("([{");
    const QString closeBrackets = QStringLiteral(")]}");

    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid()) {
        return -1;
    }

    //
    // ... ищем ближайшую скобку в строке, начиная с позиции курсора
    //
    const int blockEndPosition = block.position() + block.length() - 1;
    int bracketPosition = -1;
    for (int index = boundedPosition(_document, _position); index < blockEndPosition; ++index) {
        const auto character = _document->characterAt(index);
        if (openBrackets.contains(character) || closeBrackets.contains(character)) {
            bracketPosition = index;
            break;
        }
    }
    if (bracketPosition < 0) {
        return -1;
    }

    //
    // ... и идём к парной ей, учитывая вложенность
    //
    const auto bracket = _document->characterAt(bracketPosition);
    const int openBracketIndex = openBrackets.indexOf(bracket);
    const bool forward = openBracketIndex >= 0;
    const auto openBracket = forward ? bracket : openBrackets.at(closeBrackets.indexOf(bracket));
    const auto closeBracket = forward ? closeBrackets.at(openBracketIndex) : bracket;

    const int endPosition = lastPosition(_document);
    int depth = 0;
    for (int index = bracketPosition; index >= 0 && index < endPosition;
         index += forward ? 1 : -1) {
        const auto character = _document->characterAt(index);
        if (character == openBracket) {
            depth += forward ? 1 : -1;
        } else if (character == closeBracket) {
            depth += forward ? -1 : 1;
        } else {
            continue;
        }

        if (depth == 0) {
            return index;
        }
    }

    return -1;
}

int paragraphPosition(const QTextDocument* _document, int _position, bool _forward, int _count)
{
    auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid()) {
        return boundedPosition(_document, _position);
    }

    for (int step = 0; step < qMax(1, _count); ++step) {
        auto candidate = _forward ? block.next() : block.previous();
        while (candidate.isValid() && candidate.length() != 1) {
            candidate = _forward ? candidate.next() : candidate.previous();
        }

        if (!candidate.isValid()) {
            return _forward ? lastPosition(_document) : 0;
        }
        block = candidate;
    }

    return block.position();
}

TextRange wordRange(const QTextDocument* _document, int _position, bool _bigWord, bool _around)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid() || block.length() <= 1) {
        return {};
    }

    const auto text = block.text();
    const int index = positionInBlock(block, boundedPosition(_document, _position));
    const auto targetClass = characterClass(text.at(index), _bigWord);

    int start = index;
    while (start > 0 && characterClass(text.at(start - 1), _bigWord) == targetClass) {
        --start;
    }
    int end = index;
    while (end + 1 < text.size() && characterClass(text.at(end + 1), _bigWord) == targetClass) {
        ++end;
    }

    if (_around) {
        //
        // ... объект со словом захватывает пробелы за ним, а если их нет, то перед ним
        //
        int aroundEnd = end;
        while (aroundEnd + 1 < text.size()
               && characterClass(text.at(aroundEnd + 1), _bigWord) == CharacterClass::Blank) {
            ++aroundEnd;
        }
        if (aroundEnd == end && targetClass != CharacterClass::Blank) {
            while (start > 0
                   && characterClass(text.at(start - 1), _bigWord) == CharacterClass::Blank) {
                --start;
            }
        }
        end = aroundEnd;
    }

    return { block.position() + start, block.position() + end + 1, false };
}

TextRange blockRange(const QTextDocument* _document, int _position, QChar _openBracket,
                     QChar _closeBracket, bool _around)
{
    //
    // Ограничиваем область поиска скобок, чтобы не просматривать документ целиком
    //
    const int kMaxScanLength = 50000;

    const int position = boundedPosition(_document, _position);
    const int endPosition = qMin(lastPosition(_document), position + kMaxScanLength);
    const int startPosition = qMax(0, position - kMaxScanLength);

    //
    // ... ищем открывающую скобку слева от курсора
    //
    int openPosition = -1;
    int depth = 0;
    for (int index = position; index >= startPosition; --index) {
        const auto character = _document->characterAt(index);
        if (character == _closeBracket && index != position) {
            ++depth;
        } else if (character == _openBracket) {
            if (depth == 0) {
                openPosition = index;
                break;
            }
            --depth;
        }
    }
    if (openPosition < 0) {
        return {};
    }

    //
    // ... и закрывающую справа от неё
    //
    int closePosition = -1;
    depth = 0;
    for (int index = openPosition + 1; index < endPosition; ++index) {
        const auto character = _document->characterAt(index);
        if (character == _openBracket) {
            ++depth;
        } else if (character == _closeBracket) {
            if (depth == 0) {
                closePosition = index;
                break;
            }
            --depth;
        }
    }
    if (closePosition < 0) {
        return {};
    }

    return _around ? TextRange{ openPosition, closePosition + 1, false }
                   : TextRange{ openPosition + 1, closePosition, false };
}

TextRange quoteRange(const QTextDocument* _document, int _position, QChar _quote, bool _around)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid() || block.length() <= 1) {
        return {};
    }

    const auto text = block.text();
    const int index = positionInBlock(block, boundedPosition(_document, _position));

    //
    // ... собираем позиции кавычек в строке, пропуская экранированные
    //
    QVector<int> quotes;
    for (int scan = 0; scan < text.size(); ++scan) {
        if (text.at(scan) == _quote && (scan == 0 || text.at(scan - 1) != '\\')) {
            quotes.append(scan);
        }
    }

    //
    // ... и берём первую пару, которая заканчивается не раньше курсора
    //
    for (int pair = 0; pair + 1 < quotes.size(); pair += 2) {
        const int openIndex = quotes.at(pair);
        const int closeIndex = quotes.at(pair + 1);
        if (index > closeIndex) {
            continue;
        }

        if (!_around) {
            return { block.position() + openIndex + 1, block.position() + closeIndex, false };
        }

        int end = closeIndex + 1;
        while (end < text.size() && text.at(end).isSpace()) {
            ++end;
        }
        return { block.position() + openIndex, block.position() + end, false };
    }

    return {};
}

TextRange paragraphRange(const QTextDocument* _document, int _position, bool _around)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid()) {
        return {};
    }

    const bool emptyParagraph = block.length() == 1;
    auto firstBlock = block;
    while (firstBlock.previous().isValid()
           && (firstBlock.previous().length() == 1) == emptyParagraph) {
        firstBlock = firstBlock.previous();
    }
    auto lastBlock = block;
    while (lastBlock.next().isValid() && (lastBlock.next().length() == 1) == emptyParagraph) {
        lastBlock = lastBlock.next();
    }

    if (_around) {
        //
        // ... объект с абзацем захватывает ещё и отделяющие его пустые строки
        //
        while (lastBlock.next().isValid() && (lastBlock.next().length() == 1) != emptyParagraph) {
            lastBlock = lastBlock.next();
        }
    }

    return { firstBlock.position(), lastBlock.position() + lastBlock.length() - 1, true };
}

QString wordUnderCursor(const QTextDocument* _document, int _position)
{
    const auto block = _document->findBlock(boundedPosition(_document, _position));
    if (!block.isValid() || block.length() <= 1) {
        return {};
    }

    const auto text = block.text();
    int index = positionInBlock(block, boundedPosition(_document, _position));

    //
    // ... если курсор стоит не на слове, то берём ближайшее справа
    //
    while (index < text.size() && characterClass(text.at(index), false) != CharacterClass::Word) {
        ++index;
    }
    if (index >= text.size()) {
        return {};
    }

    int start = index;
    while (start > 0 && characterClass(text.at(start - 1), false) == CharacterClass::Word) {
        --start;
    }
    int end = index;
    while (end + 1 < text.size()
           && characterClass(text.at(end + 1), false) == CharacterClass::Word) {
        ++end;
    }

    return text.mid(start, end - start + 1);
}

} // namespace Vim
} // namespace Ui

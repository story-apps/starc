#include "spell_check_highlighter.h"

#include "spell_checker.h"

#include <QTextDocument>


class SpellCheckHighlighter::Implementation
{
public:
    explicit Implementation(const SpellChecker& _checker);
    /**
     * @brief Проверяющий орфографию
     */
    const SpellChecker& spellChecker;

    /**
     * @brief Формат текста не прошедшего проверку орфографии
     */
    QTextCharFormat misspeledCharFormat;

    /**
     * @brief Использовать проверяющего
     */
    bool useSpellChecker = false;

    /**
     * @brief Позиция курсора в блоке
     */
    int cursorPosition = -1;
};

SpellCheckHighlighter::Implementation::Implementation(const SpellChecker& _checker)
    : spellChecker(_checker)
{
    //
    // Настроим стиль выделения текста не прошедшего проверку
    //
    misspeledCharFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);
    misspeledCharFormat.setUnderlineColor(Qt::red);
}


// ****


SpellCheckHighlighter::SpellCheckHighlighter(QTextDocument* _parent, const SpellChecker& _checker) :
    SyntaxHighlighter(_parent),
    d(new Implementation(_checker))
{
}

SpellCheckHighlighter::~SpellCheckHighlighter() = default;

void SpellCheckHighlighter::setUseSpellChecker(bool _use)
{
    if (d->useSpellChecker == _use) {
        return;
    }

    d->useSpellChecker = _use;

    //
    // Если документ создан и не пуст, перепроверить его
    //
    if (document() != nullptr && !document()->isEmpty()) {
        rehighlight();
    }
}

bool SpellCheckHighlighter::useSpellChecker() const
{
    return d->useSpellChecker;
}

void SpellCheckHighlighter::setCursorPosition(int _position)
{
    d->cursorPosition = _position;
}

void SpellCheckHighlighter::highlightBlock(const QString& _text)
{
    if (!d->useSpellChecker) {
        return;
    }

    if (_text.simplified().isEmpty()) {
        return;
    }

    //
    // Убираем пустоты из проверяемого текста
    //
    QRegExp notWord("[^\\w'-]+");
    notWord.indexIn(_text);
    //
    // Проверяем каждое слово
    //
    int wordPos = 0;
    int notWordPos = notWord.pos(0);
    for (wordPos = 0; wordPos < _text.length(); wordPos = notWordPos + 1) {
        //
        // Получим окончание слова
        //
        notWord.indexIn(_text, wordPos);
        notWordPos = notWord.pos(0);
        if (notWordPos == -1) {
            notWordPos = _text.length();
        }

        //
        // Получим само слово
        //
        const QString wordToCheck = _text.mid(wordPos, notWordPos - wordPos);

        //
        // Проверяем слова длинной более одного символа
        //
        if (wordToCheck.length() > 1) {
            int positionInText = wordPos;
            //
            // Не проверяем слово, которое сейчас пишется
            //
            if (isChanged()
                && positionInText <= d->cursorPosition
                && positionInText + wordToCheck.length() > d->cursorPosition) {
                continue;
            }

            //
            // Если слово не прошло проверку
            //
            if (!d->spellChecker.spellCheckWord(wordToCheck)) {
                const int wordLength = wordToCheck.length();
                setFormat(positionInText, wordLength, d->misspeledCharFormat);
            }
        }
    }
}

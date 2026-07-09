#include "spell_check_highlighter.h"

#include "spell_checker.h"

#include <QPointer>
#include <QRegularExpression>
#include <QTextBlock>
#include <QTextDocument>
#include <QTimer>


namespace {
const int kInvalidCursorPosition = -1;
}


class SpellCheckHighlighter::Implementation
{
public:
    explicit Implementation(SpellCheckHighlighter* _q, const SpellChecker& _checker);

    /**
     * @brief Применить заданный формат к заданному тексту
     */
    void setFormat(int _start, int _count, const QTextCharFormat& _format);

    /**
     * @brief Применить изменения формата
     */
    void applyFormatChanges();

    /**
     * @brief Перепроверить проверку орфографии в заданном блоке
     */
    void reformatBlock(const QTextBlock& _block);

    /**
     * @brief Перепроверить проверку орфографии в заданных блоках
     */
    void reformatBlocks(int _from, int _charsRemoved, int _charsAdded);

    /**
     * @brief Нужно ли выполнять подсветку заданного абзаца
     */
    bool shouldReformatBlock(const QTextBlock& _block) const;

    /**
     * @brief Подсветить текст не прошедший проверку орфографии
     */
    void highlightBlock(const QString& _text);

    /**
     * @brief Очистить установленные форматы из всего документа
     */
    void clearFormats();


    SpellCheckHighlighter* q = nullptr;

    /**
     * @brief Документ, в котором осуществляется проверка орфографии
     */
    QPointer<QTextDocument> doc;

    /**
     * @brief Подготовленные изменения формата текста
     */
    QVector<QTextCharFormat> formatChanges;

    /**
     * @brief Текущий текстовый блок, с которым работаем
     */
    QTextBlock currentBlock;

    /**
     * @brief Использовать проверяющего
     */
    bool useSpellChecker = false;

    /**
     * @brief Проверяющий орфографию
     */
    const SpellChecker& spellChecker;

    /**
     * @brief Диапазон абзацев, для которых выполняется проверка орфографии
     */
    struct {
        int first = 0;
        int last = 0;
    } activeBlockRange;

    /**
     * @brief Формат текста не прошедшего проверку орфографии
     */
    QTextCharFormat misspeledCharFormat;

    /**
     * @brief Позиция курсора в блоке
     */
    struct {
        int inDocument = kInvalidCursorPosition;
        int inBlock = kInvalidCursorPosition;
    } cursorPosition;

    /**
     * @brief Таймер перепроверки текущего абзаца после изменения положения курсора
     */
    QTimer recheckTimer;
};

SpellCheckHighlighter::Implementation::Implementation(SpellCheckHighlighter* _q,
                                                      const SpellChecker& _checker)
    : q(_q)
    , spellChecker(_checker)
{
    //
    // Настроим стиль выделения текста не прошедшего проверку
    //
    misspeledCharFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    misspeledCharFormat.setUnderlineColor(Qt::red);

    recheckTimer.setInterval(1600);
    recheckTimer.setSingleShot(true);
}

void SpellCheckHighlighter::Implementation::setFormat(int _start, int _count,
                                                      const QTextCharFormat& _format)
{
    if (_start < 0 || _start >= formatChanges.count()) {
        return;
    }

    const int end = qMin(_start + _count, formatChanges.count());
    for (int i = _start; i < end; ++i) {
        formatChanges[i] = _format;
    }
}

void SpellCheckHighlighter::Implementation::applyFormatChanges()
{
    bool formatsChanged = false;

    QTextLayout* layout = currentBlock.layout();
    QVector<QTextLayout::FormatRange> ranges = layout->formats();

    const int preeditAreaStart = layout->preeditAreaPosition();
    const int preeditAreaLength = layout->preeditAreaText().length();

    if (preeditAreaLength != 0) {
        auto it = ranges.begin();
        while (it != ranges.end()) {
            if (it->start >= preeditAreaStart
                && it->start + it->length <= preeditAreaStart + preeditAreaLength) {
                ++it;
            } else {
                it = ranges.erase(it);
                formatsChanged = true;
            }
        }
    } else if (!ranges.isEmpty()) {
        ranges.clear();
        formatsChanged = true;
    }

    int i = 0;
    while (i < formatChanges.count()) {
        QTextLayout::FormatRange range;

        while (i < formatChanges.count() && formatChanges.at(i) == range.format) {
            ++i;
        }
        if (i == formatChanges.count()) {
            break;
        }

        range.start = i;
        range.format = formatChanges.at(i);

        while (i < formatChanges.count() && formatChanges.at(i) == range.format) {
            ++i;
        }

        range.length = i - range.start;

        if (preeditAreaLength != 0) {
            if (range.start >= preeditAreaStart) {
                range.start += preeditAreaLength;
            } else if (range.start + range.length >= preeditAreaStart) {
                range.length += preeditAreaLength;
            }
        }

        ranges << range;
        formatsChanged = true;
    }

    if (formatsChanged) {
        layout->setFormats(ranges);
        doc->markContentsDirty(currentBlock.position(), currentBlock.length());
    }
}

void SpellCheckHighlighter::Implementation::reformatBlock(const QTextBlock& _block)
{
    Q_ASSERT_X(!currentBlock.isValid(), "SpellCheckHighlighter::reformatBlock()",
               "reformatBlock() called recursively");

    currentBlock = _block;

    formatChanges.fill(QTextCharFormat(), _block.length() - 1);
    //
    // Если блок в верхнем регистре, то передадим в этом же регистре спеллчекеру
    //
    if (_block.charFormat().fontCapitalization() == QFont::AllUppercase) {
        highlightBlock(_block.text().toUpper());
    } else {
        highlightBlock(_block.text());
    }
    applyFormatChanges();

    currentBlock = QTextBlock();
}

void SpellCheckHighlighter::Implementation::reformatBlocks(int _from, int _charsRemoved,
                                                           int _charsAdded)
{
    QTextBlock block = doc->findBlock(_from);
    if (!block.isValid()) {
        return;
    }

    int endPosition = 0;
    const QTextBlock lastBlock = doc->findBlock(_from + _charsAdded + (_charsRemoved > 0 ? 1 : 0));
    if (lastBlock.isValid()) {
        endPosition = lastBlock.position() + lastBlock.length();
    } else {
        endPosition = doc->characterCount();
    }

    bool forceHighlightOfNextBlock = false;
    while (block.isValid() && (block.position() < endPosition || forceHighlightOfNextBlock)) {
        if (!shouldReformatBlock(block)) {
            forceHighlightOfNextBlock = false;
            block = block.next();
            continue;
        }

        const int stateBeforeHighlight = block.userState();
        reformatBlock(block);
        forceHighlightOfNextBlock = (block.userState() != stateBeforeHighlight);
        block = block.next();
    }

    formatChanges.clear();
}

bool SpellCheckHighlighter::Implementation::shouldReformatBlock(const QTextBlock& _block) const
{
    if (!useSpellChecker) {
        return true;
    }

    const auto blockNumber = _block.blockNumber();
    return blockNumber >= activeBlockRange.first && blockNumber <= activeBlockRange.last;
}

void SpellCheckHighlighter::Implementation::highlightBlock(const QString& _text)
{
    if (!useSpellChecker) {
        return;
    }

    if (_text.simplified().isEmpty()) {
        return;
    }

    //
    // Убираем пустоты из проверяемого текста
    //
    const static QRegularExpression notWord("([^\\w'’-]|·)+",
                                            QRegularExpression::UseUnicodePropertiesOption);
    //
    // Проверяем каждое слово
    //
    int wordPos = 0;
    int notWordLength = 1;
    int notWordPos = 0;
    for (wordPos = 0; wordPos < _text.length(); wordPos = notWordPos + notWordLength) {
        //
        // Получим окончание слова
        //
        const auto match = notWord.match(_text, wordPos);
        if (match.hasMatch()) {
            notWordPos = match.capturedStart();
            notWordLength = std::max(1, static_cast<int>(match.capturedLength()));
        } else {
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
            if (positionInText <= cursorPosition.inBlock
                && positionInText + wordToCheck.length() > cursorPosition.inBlock) {
                continue;
            }

            //
            // Если слово не прошло проверку
            //
            if (!spellChecker.spellCheckWord(wordToCheck)) {
                const int wordLength = wordToCheck.length();
                setFormat(positionInText, wordLength, misspeledCharFormat);
            }
        }
    }
}

void SpellCheckHighlighter::Implementation::clearFormats()
{
    if (!doc || doc->isEmpty()) {
        return;
    }

    QTextCursor cursor(doc);
    cursor.beginEditBlock();
    for (QTextBlock block = doc->begin(); block.isValid(); block = block.next()) {
        block.layout()->clearFormats();
    }
    cursor.endEditBlock();
}


// ****


SpellCheckHighlighter::SpellCheckHighlighter(QTextDocument* _parent, const SpellChecker& _checker)
    : QObject(_parent)
    , d(new Implementation(this, _checker))
{
    setDocument(_parent);

    connect(&d->recheckTimer, &QTimer::timeout, this, [this] {
        if (d->cursorPosition.inDocument == kInvalidCursorPosition) {
            return;
        }

        const auto blockToRecheck = document()->findBlock(d->cursorPosition.inDocument);
        d->cursorPosition = {};
        rehighlightBlocks(blockToRecheck, blockToRecheck);
    });
}

SpellCheckHighlighter::~SpellCheckHighlighter()
{
    setDocument(nullptr);
}

void SpellCheckHighlighter::setDocument(QTextDocument* _document)
{
    if (d->doc) {
        d->doc->disconnect(this);
        d->clearFormats();
    }

    d->doc = _document;
    if (d->doc) {
        connect(d->doc, &QTextDocument::contentsChange, this,
                [this](int _from, int _charsRemoved, int _charsAdded) {
                    d->reformatBlocks(_from, _charsRemoved, _charsAdded);
                });
    }
}

QTextDocument* SpellCheckHighlighter::document() const
{
    return d->doc;
}

void SpellCheckHighlighter::rehighlightBlock(const QTextBlock& _block)
{
    rehighlightBlocks(_block, _block);
}

void SpellCheckHighlighter::rehighlightBlocks(const QTextBlock& _firstBlock,
                                              const QTextBlock& _lastBlock)
{
    if (!d->doc || !_firstBlock.isValid() || !_lastBlock.isValid()
        || _firstBlock.document() != d->doc || _lastBlock.document() != d->doc) {
        return;
    }

    d->activeBlockRange.first = std::min(_firstBlock.blockNumber(), _lastBlock.blockNumber());
    d->activeBlockRange.last = std::max(_firstBlock.blockNumber(), _lastBlock.blockNumber());

    auto block = d->doc->findBlockByNumber(d->activeBlockRange.first);
    QTextCursor cursor(block);
    cursor.beginEditBlock();
    while (block.isValid() && block.blockNumber() <= d->activeBlockRange.last) {
        d->reformatBlock(block);
        block = block.next();
    }
    cursor.endEditBlock();

    d->formatChanges.clear();
}

void SpellCheckHighlighter::setUseSpellChecker(bool _use)
{
    if (d->useSpellChecker == _use || !d->spellChecker.isAvailable()) {
        return;
    }

    d->useSpellChecker = _use;

    //
    // Если проверка отключается, нужно сбросить существующую подсветку во всём документе.
    // При включении вызывающий код сам задаёт актуальный видимый диапазон и перепроверяет
    // только его, чтобы не сканировать весь документ.
    //
    if (!d->useSpellChecker) {
        d->clearFormats();
    }
}

bool SpellCheckHighlighter::useSpellChecker() const
{
    return d->useSpellChecker;
}

void SpellCheckHighlighter::setCursorPosition(int _position)
{
    d->cursorPosition.inDocument = _position;
    d->cursorPosition.inBlock = _position - document()->findBlock(_position).position();
    d->recheckTimer.start();
}

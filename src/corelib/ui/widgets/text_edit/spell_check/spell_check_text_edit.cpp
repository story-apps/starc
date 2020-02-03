#include "spell_check_text_edit.h"

#include "spell_checker.h"
#include "spell_check_highlighter.h"

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QMenu>
#include <QStandardPaths>
#include <QTimer>

#include <QtGui/private/qtextdocument_p.h>


namespace {
    /**
     * @brief Максимальное кол-во подсказок для проверки орфографии
     */
    const int kSuggestionsActionsMaxCount = 5;
}

class SpellCheckTextEdit::Implementation
{
public:
    explicit Implementation(QTextDocument* _document);


    /**
     * @brief Проверяющий орфографию
     */
    SpellChecker& m_spellChecker;

    /**
     * @brief Подсвечивающий орфографические ошибки
     */
    SpellCheckHighlighter m_spellCheckHighlighter;

    /**
     * @brief Действия для слова не прошедшего проверку орфографии
     */
    /** @{ */
    QAction* m_ignoreWordAction;
    QAction* m_addWordToUserDictionaryAction;
    QList<QAction*> m_suggestionsActions;
    /** @} */

    /**
     * @brief Последняя позиция курсора, при открытии контекстного меню
     */
    QPoint m_lastCursorPosition;

    /**
     * @brief Предыдущий блок, на котором был курсор
     */
    QTextBlock m_prevBlock;
};

SpellCheckTextEdit::Implementation::Implementation(QTextDocument* _document)
    : m_spellChecker(SpellChecker::instance()),
      m_spellCheckHighlighter(_document, m_spellChecker)
{
}


// ****


SpellCheckTextEdit::SpellCheckTextEdit(QWidget *_parent)
    : PageTextEdit(_parent),
      d(new Implementation(document()))
{
    connect(this, &SpellCheckTextEdit::cursorPositionChanged,
            this, &SpellCheckTextEdit::rehighlighWithNewCursor);
}

SpellCheckTextEdit::~SpellCheckTextEdit() = default;

void SpellCheckTextEdit::setUseSpellChecker(bool _use)
{
    d->m_spellCheckHighlighter.setUseSpellChecker(_use);
}

bool SpellCheckTextEdit::useSpellChecker() const
{
    return d->m_spellCheckHighlighter.useSpellChecker();
}

void SpellCheckTextEdit::setSpellCheckLanguage(SpellCheckerLanguage _language)
{
    //
    // Установим язык проверяющего
    //
    d->m_spellChecker.setSpellingLanguage(_language);

    if (!useSpellChecker()) {
        return;
    }

    //
    // Заново выделим слова не проходящие проверку орфографии вновь заданного языка
    //
    d->m_spellCheckHighlighter.rehighlight();
}

void SpellCheckTextEdit::prepareToClear()
{
    d->m_prevBlock = QTextBlock();
}

void SpellCheckTextEdit::setHighlighterDocument(QTextDocument* _document)
{
    d->m_prevBlock = QTextBlock();
    d->m_spellCheckHighlighter.setDocument(_document);
}

void SpellCheckTextEdit::ignoreWord() const
{
    //
    // Определим слово под курсором
    //
    const QString wordUnderCursor = wordOnPosition(d->m_lastCursorPosition);

    //
    // Уберем пунктуацию
    //
    const QString wordUnderCursorWithoutPunct = removePunctutaion(wordUnderCursor);

    //
    // Скорректируем регистр слова
    //
    const QString wordUnderCursorWithoutPunctInCorrectRegister = wordUnderCursorWithoutPunct.toLower();

    //
    // Объявляем проверяющему о том, что это слово нужно игнорировать
    //
    d->m_spellChecker.ignoreWord(wordUnderCursorWithoutPunctInCorrectRegister);

    //
    // Уберём выделение с игнорируемых слов
    //
    d->m_spellCheckHighlighter.rehighlight();
}

void SpellCheckTextEdit::addWordToUserDictionary() const
{
    //
    // Определим слово под курсором
    //
    const QString wordUnderCursor = wordOnPosition(d->m_lastCursorPosition);

    //
    // Уберем пунктуацию в слове
    //
    const QString wordUnderCursorWithoutPunct = removePunctutaion(wordUnderCursor);

    //
    // Приведем к нижнему регистру
    //
    const QString wordUnderCursorWithoutPunctInCorrectRegister = wordUnderCursorWithoutPunct.toLower();

    //
    // Объявляем проверяющему о том, что это слово нужно добавить в пользовательский словарь
    //
    d->m_spellChecker.addWordToDictionary(wordUnderCursorWithoutPunctInCorrectRegister);

    //
    // Уберём выделение со слов добавленных в словарь
    //
    d->m_spellCheckHighlighter.rehighlight();
}

void SpellCheckTextEdit::replaceWordOnSuggestion()
{
    if (QAction* suggestAction = qobject_cast<QAction*>(sender())) {
        QTextCursor cursor = cursorForPosition(d->m_lastCursorPosition);
        cursor = moveCursorToStartWord(cursor);
        QTextCursor endCursor = moveCursorToEndWord(cursor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, endCursor.positionInBlock() - cursor.positionInBlock());
        cursor.beginEditBlock();
        cursor.removeSelectedText();
        cursor.insertText(suggestAction->text());
        setTextCursor(cursor);
        cursor.endEditBlock();

        //
        // Немного магии. Мы хотим перепроверить блок, в котором изменили слово.
        // Беда в том, что SpellCheckHighlighter не будет проверять слово в блоке,
        // если оно сейчас редактировалось. Причем, эта проверка идет по позиции.
        // А значит при проверке другого блока, слово на этой позиции не проверится.
        // Поэтому, мы ему говорим, что слово не редактировалось, проверяй весь абзац
        // а затем восстанавливаем в прежнее состояние
        //
        bool isEdited = d->m_spellCheckHighlighter.isChanged();
        d->m_spellCheckHighlighter.rehighlightBlock(cursor.block());
        d->m_spellCheckHighlighter.setChanged(isEdited);
    }
}

QTextCursor SpellCheckTextEdit::moveCursorToStartWord(QTextCursor cursor) const
{
    cursor.movePosition(QTextCursor::StartOfWord);
    QString text = cursor.block().text();

    //
    // Цикл ниже необходим, потому что movePosition(StartOfWord)
    // считает - и ' другими словами
    // Примеры "кто-" - еще не закончив печатать слово, получим
    // его подсветку
    //
    while (cursor.positionInBlock() > 0 &&
           (text[cursor.positionInBlock()] == '\''
            || text[cursor.positionInBlock()] == '-'
            || text[cursor.positionInBlock() - 1] == '\''
            || text[cursor.positionInBlock() - 1] == '-')) {
            cursor.movePosition(QTextCursor::PreviousCharacter);
            cursor.movePosition(QTextCursor::StartOfWord);
    }
    return cursor;
}

QTextCursor SpellCheckTextEdit::moveCursorToEndWord(QTextCursor cursor) const
{
    QRegExp splitWord("[^\\w'-]");
    splitWord.indexIn(cursor.block().text(), cursor.positionInBlock());
    int pos = splitWord.pos();
    if (pos == -1) {
        pos= cursor.block().text().length();
    }
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, pos - cursor.positionInBlock());
    return cursor;
}

void SpellCheckTextEdit::rehighlighWithNewCursor()
{
    if (!d->m_spellCheckHighlighter.useSpellChecker()) {
        return;
    }

    //
    // Если редактирование документа не закончено, но позиция курсора сменилась, откладываем проверку орфографии
    //
    if (document()->docHandle()->isInEditBlock()) {
        QTimer::singleShot(0, this, &SpellCheckTextEdit::rehighlighWithNewCursor);
        return;
    }

    QTextCursor cursor = textCursor();
    cursor = moveCursorToStartWord(cursor);
    d->m_spellCheckHighlighter.setCursorPosition(cursor.positionInBlock());
    if (d->m_prevBlock.isValid()) {
        d->m_spellCheckHighlighter.rehighlightBlock(d->m_prevBlock);
    }
    d->m_prevBlock = textCursor().block();
}

QString SpellCheckTextEdit::wordOnPosition(const QPoint& _position) const
{
    const QTextCursor cursorWordStart = moveCursorToStartWord(cursorForPosition(_position));
    const QTextCursor cursorWordEnd = moveCursorToEndWord(cursorWordStart);
    const QString text = cursorWordStart.block().text();
    return text.mid(cursorWordStart.positionInBlock(), cursorWordEnd.positionInBlock() - cursorWordStart.positionInBlock());
}

QString SpellCheckTextEdit::removePunctutaion(const QString &_word) const
{
    //
    // Убираем знаки препинания окружающие слово
    //
    QString wordWithoutPunct = _word.trimmed();
    while (!wordWithoutPunct.isEmpty()
           && (wordWithoutPunct.at(0).isPunct()
               || wordWithoutPunct.at(wordWithoutPunct.length()-1).isPunct())) {
        if (wordWithoutPunct.at(0).isPunct()) {
            wordWithoutPunct = wordWithoutPunct.mid(1);
        } else {
            wordWithoutPunct = wordWithoutPunct.left(wordWithoutPunct.length()-1);
        }
    }
    return wordWithoutPunct;
}

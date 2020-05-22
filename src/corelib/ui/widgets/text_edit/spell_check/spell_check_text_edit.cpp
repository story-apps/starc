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
    Implementation();

    /**
     * @brief Получить подсвечивающий ошибки объект
     * @note Объект пересоздаётся, если прошлый был удалён
     */
    SpellCheckHighlighter* spellCheckHighlighter(QTextDocument* _document);

    /**
     * @brief Проверяющий орфографию
     */
    SpellChecker& spellChecker;

    /**
     * @brief Действия для слова не прошедшего проверку орфографии
     */
    /** @{ */
    QAction* ignoreWordAction;
    QAction* addWordToUserDictionaryAction;
    QList<QAction*> suggestionsActions;
    /** @} */

    /**
     * @brief Последняя позиция курсора, при открытии контекстного меню
     */
    QPoint lastCursorPosition;

    /**
     * @brief Предыдущий блок, на котором был курсор
     */
    QTextBlock previousBlockUnderCursor;

private:
    /**
     * @brief Подсвечивающий орфографические ошибки
     */
    QPointer<SpellCheckHighlighter> m_spellCheckHighlighter;
};

SpellCheckTextEdit::Implementation::Implementation()
    : spellChecker(SpellChecker::instance())
{
}

SpellCheckHighlighter* SpellCheckTextEdit::Implementation::spellCheckHighlighter(QTextDocument* _document)
{
    if (m_spellCheckHighlighter.isNull()) {
        m_spellCheckHighlighter = new SpellCheckHighlighter(_document, spellChecker);
    }
    return m_spellCheckHighlighter;
}


// ****


SpellCheckTextEdit::SpellCheckTextEdit(QWidget *_parent)
    : PageTextEdit(_parent),
      d(new Implementation)
{
    connect(this, &SpellCheckTextEdit::cursorPositionChanged, this, &SpellCheckTextEdit::rehighlighWithNewCursor);
}

SpellCheckTextEdit::~SpellCheckTextEdit() = default;

void SpellCheckTextEdit::setUseSpellChecker(bool _use)
{
    d->spellCheckHighlighter(document())->setUseSpellChecker(_use);
}

bool SpellCheckTextEdit::useSpellChecker() const
{
    return d->spellCheckHighlighter(document())->useSpellChecker();
}

void SpellCheckTextEdit::setSpellCheckLanguage(const QString& _languageCode)
{
    //
    // Установим язык проверяющего
    //
    d->spellChecker.setSpellingLanguage(_languageCode);

    if (!useSpellChecker()) {
        return;
    }

    //
    // Заново выделим слова не проходящие проверку орфографии вновь заданного языка
    //
    d->spellCheckHighlighter(document())->rehighlight();
}

void SpellCheckTextEdit::prepareToClear()
{
    d->previousBlockUnderCursor = QTextBlock();
}

void SpellCheckTextEdit::setHighlighterDocument(QTextDocument* _document)
{
    d->previousBlockUnderCursor = QTextBlock();
    d->spellCheckHighlighter(document())->setDocument(_document);
}

void SpellCheckTextEdit::ignoreWord() const
{
    //
    // Определим слово под курсором
    //
    const QString wordUnderCursor = wordOnPosition(d->lastCursorPosition);

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
    d->spellChecker.ignoreWord(wordUnderCursorWithoutPunctInCorrectRegister);

    //
    // Уберём выделение с игнорируемых слов
    //
    d->spellCheckHighlighter(document())->rehighlight();
}

void SpellCheckTextEdit::addWordToUserDictionary() const
{
    //
    // Определим слово под курсором
    //
    const QString wordUnderCursor = wordOnPosition(d->lastCursorPosition);

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
    d->spellChecker.addWordToDictionary(wordUnderCursorWithoutPunctInCorrectRegister);

    //
    // Уберём выделение со слов добавленных в словарь
    //
    d->spellCheckHighlighter(document())->rehighlight();
}

void SpellCheckTextEdit::replaceWordOnSuggestion()
{
    if (QAction* suggestAction = qobject_cast<QAction*>(sender())) {
        QTextCursor cursor = cursorForPosition(d->lastCursorPosition);
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
        bool isEdited = d->spellCheckHighlighter(document())->isChanged();
        d->spellCheckHighlighter(document())->rehighlightBlock(cursor.block());
        d->spellCheckHighlighter(document())->setChanged(isEdited);
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
    if (!d->spellCheckHighlighter(document())->useSpellChecker()) {
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
    d->spellCheckHighlighter(document())->setCursorPosition(cursor.positionInBlock());
    if (d->previousBlockUnderCursor.isValid()) {
        d->spellCheckHighlighter(document())->rehighlightBlock(d->previousBlockUnderCursor);
    }
    d->previousBlockUnderCursor = textCursor().block();
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

#include "spell_check_text_edit.h"

#include "spell_check_highlighter.h"
#include "spell_checker.h"

#include <include/custom_events.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QMenu>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QTimer>
#include <QtGui/private/qtextdocument_p.h>


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
     * @brief Политика обновления состояния проверки орфографии
     */
    SpellCheckPolicy policy = SpellCheckPolicy::Auto;

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

SpellCheckHighlighter* SpellCheckTextEdit::Implementation::spellCheckHighlighter(
    QTextDocument* _document)
{
    if (m_spellCheckHighlighter.isNull()) {
        m_spellCheckHighlighter = new SpellCheckHighlighter(_document, spellChecker);
    }
    return m_spellCheckHighlighter;
}


// ****


SpellCheckTextEdit::SpellCheckTextEdit(QWidget* _parent)
    : PageTextEdit(_parent)
    , d(new Implementation)
{
    connect(this, &SpellCheckTextEdit::cursorPositionChanged, this,
            &SpellCheckTextEdit::rehighlighWithNewCursor);
}

SpellCheckTextEdit::~SpellCheckTextEdit() = default;

void SpellCheckTextEdit::setSpellCheckPolicy(SpellCheckPolicy _policy)
{
    d->policy = _policy;

    //
    // Если используется ручное управление, то отключим проверку, т.к. она могла быть включена до
    // установки флага, а дальше клиента сам определит необходимость включения проверки
    //
    if (d->policy == SpellCheckPolicy::Manual) {
        setUseSpellChecker(false);
    }
}

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
    if (d->spellChecker.spellingLanguage() == _languageCode) {
        return;
    }

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

bool SpellCheckTextEdit::isMispelledWordUnderCursor(const QPoint& _position) const
{
    if (!useSpellChecker()) {
        return false;
    }

    const QTextCursor cursorWordStart = moveCursorToStartWord(cursorForPosition(_position));
    const QTextCursor cursorWordEnd = moveCursorToEndWord(cursorWordStart);
    const QString text = cursorWordStart.block().text();
    const QString wordUnderCursor
        = text.mid(cursorWordStart.positionInBlock(),
                   cursorWordEnd.positionInBlock() - cursorWordStart.positionInBlock());

    QString wordInCorrectRegister = wordUnderCursor;
    if (cursorForPosition(_position).charFormat().fontCapitalization() == QFont::AllUppercase) {
        //
        // Приведем к верхнему регистру
        //
        wordInCorrectRegister = wordUnderCursor.toUpper();
    }

    //
    // Если слово не проходит проверку орфографии добавим дополнительные действия в контекстное меню
    //
    return !d->spellChecker.spellCheckWord(wordInCorrectRegister);
}

ContextMenu* SpellCheckTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    //
    // Если в режиме только чтения, не используется проверка орфографии, есть выделенный текст
    // или нет ошибок под курсором
    //
    if (isReadOnly() || !useSpellChecker() || textCursor().hasSelection()
        || !isMispelledWordUnderCursor(_position)) {
        //
        // ... возвращаем стандартное меню
        //
        return PageTextEdit::createContextMenu(_position, _parent);
    }

    //
    // А если ошибка есть, то генерим собственное меню
    //
    // Определим слово под курсором
    //
    d->lastCursorPosition = _position;
    const QTextCursor cursorWordStart = moveCursorToStartWord(cursorForPosition(_position));
    const QTextCursor cursorWordEnd = moveCursorToEndWord(cursorWordStart);
    const QString text = cursorWordStart.block().text();
    const QString wordUnderCursor
        = text.mid(cursorWordStart.positionInBlock(),
                   cursorWordEnd.positionInBlock() - cursorWordStart.positionInBlock());

    QString wordInCorrectRegister = wordUnderCursor;
    if (cursorForPosition(_position).charFormat().fontCapitalization() == QFont::AllUppercase) {
        //
        // Приведем к верхнему регистру
        //
        wordInCorrectRegister = wordUnderCursor.toUpper();
    }

    //
    // Формируем контекстное меню
    //
    QVector<QAction*> actions;
    const auto suggestions = d->spellChecker.suggestionsForWord(wordUnderCursor);
    const auto maxSuggestions = std::min(7, static_cast<int>(suggestions.size()));
    for (int index = 0; index < maxSuggestions; ++index) {
        auto suggestionAction = new QAction;
        suggestionAction->setText(suggestions.at(index));
        if (index == 0) {
            suggestionAction->setIconText(u8"\U000F04C6");
        } else {
            suggestionAction->setIconText(u8"\U000F68C0");
        }
        connect(suggestionAction, &QAction::triggered, this,
                &SpellCheckTextEdit::replaceWordOnSuggestion);
        actions.append(suggestionAction);
    }
    if (actions.isEmpty()) {
        auto suggestionAction = new QAction;
        suggestionAction->setText("No suggestions");
        suggestionAction->setIconText(u8"\U000F04C6");
        suggestionAction->setEnabled(false);
        actions.append(suggestionAction);
    }
    //
    auto addWordAction = new QAction;
    addWordAction->setSeparator(true);
    addWordAction->setText(tr("Add to dictionary"));
    addWordAction->setIconText(u8"\U000F68C0");
    connect(addWordAction, &QAction::triggered, this, &SpellCheckTextEdit::addWordToUserDictionary);
    actions.append(addWordAction);
    //
    auto ignoreWordAction = new QAction;
    ignoreWordAction->setText(tr("Ignore word"));
    ignoreWordAction->setIconText(u8"\U000F68C0");
    connect(ignoreWordAction, &QAction::triggered, this, &SpellCheckTextEdit::ignoreWord);
    actions.append(ignoreWordAction);

    auto menu = new ContextMenu(_parent == nullptr ? this : _parent);
    menu->setActions(actions);
    menu->setBackgroundColor(Ui::DesignSystem::color().background());
    menu->setTextColor(Ui::DesignSystem::color().onBackground());
    return menu;
}

void SpellCheckTextEdit::setHighlighterDocument(QTextDocument* _document)
{
    d->previousBlockUnderCursor = QTextBlock();
    d->spellCheckHighlighter(document())->setDocument(_document);
}

bool SpellCheckTextEdit::event(QEvent* _event)
{
    switch (static_cast<int>(_event->type())) {
    case static_cast<int>(EventType::SpellingChangeEvent): {
        if (d->policy == SpellCheckPolicy::Auto) {
            const auto event = static_cast<SpellingChangeEvent*>(_event);
            //
            // Включение проверки орфографии
            //
            if (event->enabled && !useSpellChecker()) {
                setSpellCheckLanguage(event->languageCode);
                setUseSpellChecker(true);
            }
            //
            // Смена языка проверки орфографии
            //
            else if (event->enabled && useSpellChecker()) {
                setSpellCheckLanguage(event->languageCode);
            }
            //
            // Отключение
            //
            else if (!event->enabled) {
                setUseSpellChecker(false);
            }
        }
        return false;
    }

    default: {
        return PageTextEdit::event(_event);
    }
    }
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
    const QString wordUnderCursorWithoutPunctInCorrectRegister
        = wordUnderCursorWithoutPunct.toLower();

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
    const QString wordUnderCursorWithoutPunctInCorrectRegister
        = wordUnderCursorWithoutPunct.toLower();

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
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                            endCursor.positionInBlock() - cursor.positionInBlock());
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
    while (cursor.positionInBlock() > 0 && text.length() > cursor.positionInBlock()
           && (text[cursor.positionInBlock()] == '\''
               || text[cursor.positionInBlock()] == QLatin1String("’")
               || text[cursor.positionInBlock()] == '-'
               || text[cursor.positionInBlock() - 1] == '\''
               || text[cursor.positionInBlock() - 1] == '-'
               || text[cursor.positionInBlock() - 1] == QLatin1String("·"))) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
        cursor.movePosition(QTextCursor::StartOfWord);
    }
    return cursor;
}

QTextCursor SpellCheckTextEdit::moveCursorToEndWord(QTextCursor cursor) const
{
    QRegularExpression splitWord("([^\\w'’-]|·)", QRegularExpression::UseUnicodePropertiesOption);
    const auto match = splitWord.match(cursor.block().text(), cursor.positionInBlock());
    const int pos = match.hasMatch() ? match.capturedStart() : cursor.block().text().length();
    cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                        pos - cursor.positionInBlock());
    return cursor;
}

void SpellCheckTextEdit::rehighlighWithNewCursor()
{
    if (!useSpellChecker()) {
        return;
    }

    //
    // Если редактирование документа не закончено, но позиция курсора сменилась,
    // откладываем проверку орфографии
    //
#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
    if (QTextDocumentPrivate::get(document())->isInEditBlock()) {
#else
    if (document()->docHandle()->isInEditBlock()) {
#endif
        QMetaObject::invokeMethod(this, &SpellCheckTextEdit::rehighlighWithNewCursor,
                                  Qt::QueuedConnection);
        return;
    }

    //
    // Определим позицию курсора в блоке, чтобы игнорировать проверку текущего слова
    //
    QTextCursor cursor = textCursor();
    cursor = moveCursorToStartWord(cursor);
    //
    // Если сменился параграф, перепроверим орфографию в том блоке, где курсор был прежде
    //
    if (d->previousBlockUnderCursor.isValid() && d->previousBlockUnderCursor != cursor.block()) {
        //
        // ... сбросим значение курсора в блоке, чтобы блок проверился полностью
        //
        d->spellCheckHighlighter(document())->setCursorPosition(-1);
        d->spellCheckHighlighter(document())->rehighlightBlock(d->previousBlockUnderCursor);
    }
    //
    // А затем проверим орфографию в текущем абзаце, чтобы перепроверить слово под курсором
    //
    d->spellCheckHighlighter(document())->setCursorPosition(cursor.position());
    d->spellCheckHighlighter(document())->rehighlightBlock(textCursor().block());
    //
    // И на последок запомним, абзац, в которым был курсор
    //
    d->previousBlockUnderCursor = textCursor().block();
}

QString SpellCheckTextEdit::wordOnPosition(const QPoint& _position) const
{
    const QTextCursor cursorWordStart = moveCursorToStartWord(cursorForPosition(_position));
    const QTextCursor cursorWordEnd = moveCursorToEndWord(cursorWordStart);
    const QString text = cursorWordStart.block().text();
    return text.mid(cursorWordStart.positionInBlock(),
                    cursorWordEnd.positionInBlock() - cursorWordStart.positionInBlock());
}

QString SpellCheckTextEdit::removePunctutaion(const QString& _word) const
{
    //
    // Убираем знаки препинания окружающие слово
    //
    QString wordWithoutPunct = _word.trimmed();
    while (!wordWithoutPunct.isEmpty()
           && (wordWithoutPunct.at(0).isPunct()
               || wordWithoutPunct.at(wordWithoutPunct.length() - 1).isPunct())) {
        if (wordWithoutPunct.at(0).isPunct()) {
            wordWithoutPunct = wordWithoutPunct.mid(1);
        } else {
            wordWithoutPunct = wordWithoutPunct.left(wordWithoutPunct.length() - 1);
        }
    }
    return wordWithoutPunct;
}

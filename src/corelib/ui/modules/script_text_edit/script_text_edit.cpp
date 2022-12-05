#include "script_text_edit.h"

#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>

#include <QRegularExpression>
#include <QScrollBar>
#include <QTextTable>

using BusinessLayer::TextBlockStyle;


namespace Ui {

class ScriptTextEdit::Implementation
{
public:
    /**
     * @brief Показывать автодополения в пустых блоках
     */
    bool showSuggestionsInEmptyBlocks = true;
};


// ****


ScriptTextEdit::ScriptTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent)
    , d(new Implementation)
{
}

ScriptTextEdit::~ScriptTextEdit() = default;

bool ScriptTextEdit::showSuggestionsInEmptyBlocks() const
{
    return d->showSuggestionsInEmptyBlocks;
}

void ScriptTextEdit::setShowSuggestionsInEmptyBlocks(bool _show)
{
    d->showSuggestionsInEmptyBlocks = _show;
}

void ScriptTextEdit::setTextCursorAndKeepScrollBars(const QTextCursor& _cursor)
{
    //
    // При позиционировании курсора в невидимый блок, Qt откидывает прокрутку в самый верх, поэтому
    // перед редактированием документа запомним прокрутку, а после завершения редактирования
    // восстановим значения полос прокрутки
    //
    const auto verticalScrollValue = verticalScrollBar()->value();
    const auto horizontalScrollValue = horizontalScrollBar()->value();

    //
    // Задаём курсор
    //
    setTextCursor(_cursor);

    //
    // Восстанавливаем значения полос прокрутки
    //
    verticalScrollBar()->setValue(verticalScrollValue);
    horizontalScrollBar()->setValue(horizontalScrollValue);
}

bool ScriptTextEdit::updateEnteredText(const QString& _eventText)
{
    if (_eventText.isEmpty()) {
        return false;
    }

    //
    // Получим значения
    //
    // ... курсора
    QTextCursor cursor = textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());
    // ... стиль шрифта блока
    QTextCharFormat currentCharFormat = currentBlock.charFormat();

    //
    // Определяем необходимость установки верхнего регистра для первого символа блока
    //
    if (currentCharFormat.boolProperty(TextBlockStyle::PropertyIsFirstUppercase)
        && cursorBackwardText != " " && cursorBackwardText == _eventText
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сформируем правильное представление строки
        //
        QString correctedText = _eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        cursor.beginEditBlock();
        for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursorAndKeepScrollBars(cursor);
        cursor.endEditBlock();

        return true;
    }

    //
    // Если перед нами конец предложения
    // и не сокращение
    // и после курсора нет текста (для ремарки допустима скобка)
    //
    static QRegularExpression endOfSentenceRx;
    endOfSentenceRx.setPattern(QString("([.]|[?]|[!]|[…]) %1$").arg(_eventText));
    if (cursorBackwardText.contains(endOfSentenceRx) && cursorForwardText.isEmpty()
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сделаем первую букву заглавной
        //
        QString correctedText = _eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        cursor.beginEditBlock();
        for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursorAndKeepScrollBars(cursor);
        cursor.endEditBlock();

        return true;
    }

    return BaseTextEdit::updateEnteredText(_eventText);
}

} // namespace Ui

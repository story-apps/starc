#include "standard_key_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer {

namespace {
/**
 * @brief Получить тип перехода/смены в зависимости от заданных параметров
 */
static TextParagraphType actionFor(bool _tab, bool _jump, TextParagraphType _blockType)
{
    const QString settingsKey
        = QString("%1/styles-%2/from-%3-by-%4")
              .arg(DataStorageLayer::kComponentsScreenplayEditorKey,
                   (_jump ? "jumping" : "changing"), BusinessLayer::toString(_blockType),
                   (_tab ? "tab" : "enter"));
    const auto typeString = settingsValue(settingsKey).toString();
    return BusinessLayer::textParagraphTypeFromString(typeString);
}

/**
 * @brief Вспомогательные константы для использования с функцией actionFor
 */
/** @{ */
const bool kTab = true;
const bool kEnter = false;
const bool kJump = true;
const bool kChange = false;
/** @} */
} // namespace


StandardKeyHandler::StandardKeyHandler(Ui::ScreenplayTextEdit* _editor)
    : AbstractKeyHandler(_editor)
{
}

TextParagraphType StandardKeyHandler::jumpForTab(TextParagraphType _blockType)
{
    return actionFor(kTab, kJump, _blockType);
}

TextParagraphType StandardKeyHandler::jumpForEnter(TextParagraphType _blockType)
{
    return actionFor(kEnter, kJump, _blockType);
}

TextParagraphType StandardKeyHandler::changeForTab(TextParagraphType _blockType)
{
    return actionFor(kTab, kChange, _blockType);
}

TextParagraphType StandardKeyHandler::changeForEnter(TextParagraphType _blockType)
{
    return actionFor(kEnter, kChange, _blockType);
}

void StandardKeyHandler::handleDelete(QKeyEvent* _event)
{
    if (!editor()->isReadOnly()) {
        //
        // Удаление
        //
        removeCharacters(false);

        //
        // Покажем подсказку, если это возможно
        //
        handleOther(_event);
    }
}

void StandardKeyHandler::handleBackspace(QKeyEvent* _event)
{
    if (!editor()->isReadOnly()) {
        //
        // Удаление
        //
        removeCharacters(true);

        //
        // Покажем подсказку, если это возможно
        //
        handleOther(_event);
    }
}

void StandardKeyHandler::handleEscape(QKeyEvent*)
{
    editor()->closeCompleter();
}

void StandardKeyHandler::handleUp(QKeyEvent* _event)
{
    if (editor()->isCompleterVisible()) {
        return;
    }

    using namespace BusinessLayer;

    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
    const auto cursorMoveMode = isShiftPressed ? TextCursor::KeepAnchor : TextCursor::MoveAnchor;

    TextCursor cursor = editor()->textCursor();

    auto cursorRect = [this, &cursor] { return editor()->cursorRect(cursor); };

    //
    // Исходная позиция курсора
    //
    const auto sourceCursorRect = cursorRect();
    const bool sourceCursorInTable = cursor.inTable();
    const bool sourceCursorInFirstColumn = cursor.inFirstColumn();

    //
    // Идём через один предыдущий символ до предыдущей строки и ищем в ней лучшее совпадение
    //
    while (!cursor.atStart()) {
        if (!cursor.movePosition(TextCursor::PreviousCharacter, cursorMoveMode)) {
            cursor.setPosition(cursor.position() - 1, cursorMoveMode);
        }

        //
        // Если всё в той же строке
        //      или в невидимом блоке
        //      или в разделителе
        //      или в декорации
        //      или в блоке, в котором нельза показывать курсор
        //      или в таблице, но в другой колонке
        //
        if (cursorRect().top() >= sourceCursorRect.top() || !cursor.block().isVisible()
            || TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter
            || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
            || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor)
            || (sourceCursorInTable && cursor.inTable()
                && ((sourceCursorInFirstColumn && !cursor.inFirstColumn())
                    || (!sourceCursorInFirstColumn && cursor.inFirstColumn())))) {
            continue;
        }

        //
        // Дошли до нужно строки
        //

        break;
    }

    //
    // Ищем лучшее совпадение на текущей строке
    //
    auto findXDelta = [&cursorRect, sourceCursorRect] {
        return abs(sourceCursorRect.right() - cursorRect().right());
    };
    const auto previousLineCursorRect = cursorRect();
    auto bestXDelta = findXDelta();
    while (!cursor.atStart()) {
        if (!cursor.movePosition(TextCursor::PreviousCharacter, cursorMoveMode)) {
            cursor.setPosition(cursor.position() - 1, cursorMoveMode);
        }

        //
        // Если предыдущий символ находится на другой строке, то прерываем поиск
        //
        if (cursorRect().top() != previousLineCursorRect.top()) {
            cursor.movePosition(TextCursor::NextCharacter, cursorMoveMode);
            break;
        }

        //
        // Если положение предыдущего символа дальше от цели, то прерываем поиск
        //
        const auto xDelta = findXDelta();
        if (xDelta > bestXDelta) {
            cursor.movePosition(TextCursor::NextCharacter, cursorMoveMode);
            break;
        }

        bestXDelta = xDelta;
    }

    editor()->setTextCursor(cursor);
}

void StandardKeyHandler::handleDown(QKeyEvent* _event)
{
    if (editor()->isCompleterVisible()) {
        return;
    }

    using namespace BusinessLayer;

    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
    const auto cursorMoveMode = isShiftPressed ? TextCursor::KeepAnchor : TextCursor::MoveAnchor;

    TextCursor cursor = editor()->textCursor();

    auto cursorRect = [this, &cursor] { return editor()->cursorRect(cursor); };

    //
    // Исходная позиция курсора
    //
    const auto sourceCursorRect = cursorRect();
    const bool sourceCursorInTable = cursor.inTable();
    const bool sourceCursorInFirstColumn = cursor.inFirstColumn();

    //
    // Идём через один последующий символ до следующей строки и ищем в ней лучшее совпадение
    //
    while (!cursor.atEnd()) {
        if (!cursor.movePosition(TextCursor::NextCharacter, cursorMoveMode)) {
            cursor.setPosition(cursor.position() + 1, cursorMoveMode);
        }

        //
        // Если всё в той же строке
        //      или в невидимом блоке
        //      или в разделителе
        //      или в декорации
        //      или в блоке, в котором нельза показывать курсор
        //      или в таблице, но в другой колонке
        //
        if (cursorRect().top() <= sourceCursorRect.top() || !cursor.block().isVisible()
            || TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter
            || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
            || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor)
            || (sourceCursorInTable && cursor.inTable()
                && ((sourceCursorInFirstColumn && !cursor.inFirstColumn())
                    || (!sourceCursorInFirstColumn && cursor.inFirstColumn())))) {
            continue;
        }

        //
        // Дошли до нужно строки
        //

        break;
    }

    //
    // Ищем лучшее совпадение на текущей строке
    //
    auto findXDelta = [&cursorRect, sourceCursorRect] {
        return abs(sourceCursorRect.right() - cursorRect().right());
    };
    const auto previousLineCursorRect = cursorRect();
    auto bestXDelta = findXDelta();
    while (!cursor.atEnd()) {
        if (!cursor.movePosition(TextCursor::NextCharacter, cursorMoveMode)) {
            cursor.setPosition(cursor.position() + 1, cursorMoveMode);
        }

        //
        // Если следующий символ находится на другой строке, то прерываем поиск
        //
        if (cursorRect().top() != previousLineCursorRect.top()) {
            cursor.movePosition(TextCursor::PreviousCharacter, cursorMoveMode);
            break;
        }

        //
        // Если положение следуещего символа дальше от цели, то прерываем поиск
        //
        const auto xDelta = findXDelta();
        if (xDelta > bestXDelta) {
            cursor.movePosition(TextCursor::PreviousCharacter, cursorMoveMode);
            break;
        }

        bestXDelta = xDelta;
    }

    editor()->setTextCursorForced(cursor);

    //
    // Если курсор в абзаце с таблицей, а под таблицей ничего нет, то добавим блок вниз,
    //
    if (cursor.atEnd()
        && TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        editor()->addParagraph(TextParagraphType::Action);
    }
}

void StandardKeyHandler::handlePageUp(QKeyEvent* _event)
{
    for (int line = 0; line < 20; ++line) {
        handleUp(_event);
    }
}

void StandardKeyHandler::handlePageDown(QKeyEvent* _event)
{
    for (int line = 0; line < 20; ++line) {
        handleDown(_event);
    }
}

void StandardKeyHandler::handleOther(QKeyEvent*)
{
    if (editor()->isCompleterVisible()) {
        editor()->closeCompleter();
    }
}


// **** private ****


void StandardKeyHandler::removeCharacters(bool _backward)
{
    BusinessLayer::TextCursor cursor = editor()->textCursor();

    //
    // TODO: При удалении заголовка сцены бекспейсом, нужно удалить все его биты и текст
    // присоединить к тексту предыдущей сцены
    //

    //
    // Если пользователь нажимает Backspace в начале первого блока бита и он при этом скрыт,
    // то удаляем полностью блок с заголовком предшествующего бита
    //
    if (!cursor.atStart() && !cursor.hasSelection() && cursor.positionInBlock() == 0 && _backward
        && !cursor.block().previous().isVisible()
        && TextBlockStyle::forBlock(cursor.block().previous()) == TextParagraphType::BeatHeading) {
        cursor.movePosition(QTextCursor::PreviousBlock);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
        editor()->setTextCursorForced(cursor);
    }
    //
    // Если пользователь нажал Delete в конце непустого абзаца и при этом после текущего абзаца идёт
    // блок с заголовком бита, то удаляем блок заголовка бита
    //
    else if (!cursor.atEnd() && !cursor.hasSelection() && !cursor.block().text().isEmpty()
             && cursor.positionInBlock() == cursor.block().text().length() && !_backward
             && !cursor.block().next().isVisible()
             && TextBlockStyle::forBlock(cursor.block().next()) == TextParagraphType::BeatHeading) {
        cursor.movePosition(QTextCursor::NextBlock);
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
        editor()->setTextCursorForced(cursor);
    }

    cursor.removeCharacters(_backward, editor());
}

} // namespace KeyProcessingLayer

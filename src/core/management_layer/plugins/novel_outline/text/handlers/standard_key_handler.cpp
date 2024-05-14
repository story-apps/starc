#include "standard_key_handler.h"

#include "../novel_outline_edit.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/novel_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::NovelOutlineEdit;


namespace KeyProcessingLayer {

namespace {
/**
 * @brief Получить тип перехода/смены в зависимости от заданных параметров
 */
static TextParagraphType actionFor(bool _tab, bool _jump, TextParagraphType _blockType)
{
    const QString settingsKey
        = QString("%1/styles-%2/from-%3-by-%4")
              .arg(DataStorageLayer::kComponentsNovelOutlineEditorKey,
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


StandardKeyHandler::StandardKeyHandler(Ui::NovelOutlineEdit* _editor)
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

    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
    const QTextCursor::MoveMode cursorMoveMode
        = isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    QTextCursor cursor = editor()->textCursor();

    //
    // Исходная позиция курсора
    //
    int initCursorPosition = cursor.position();

    //
    // Рассчитаем количество символов от края
    //
    int marginFromLineStart = 0;
    {
        int currentLineYCoordinate = editor()->cursorRect(cursor).y();
        while (!cursor.atStart() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                break;
            }
        }
        marginFromLineStart = initCursorPosition - cursor.position() - (cursor.atStart() ? 0 : 1);
    }

    //
    // В данный момент курсор либо в начале документа, либо поднялся к концу предыдущей строки
    //

    if (!cursor.atStart()) {
        //
        // Если мы поднялись на строку вверх, но попали в невидимый блок, перейдём к предыдущему
        // видимому
        //
        const QTextBlock firstDocumentBlock = cursor.document()->firstBlock();
        while (cursor.block() != firstDocumentBlock
               && (!cursor.block().isVisible()
                   || TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                   || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor))) {
            cursor.movePosition(QTextCursor::PreviousBlock, cursorMoveMode);
            cursor.movePosition(QTextCursor::EndOfBlock, cursorMoveMode);
        }

        //
        // Сместим курсор в предыдущей строке на то кол-во символов, на которое он был смещён прежде
        //
        {
            int currentLineEndPosition = cursor.position();
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atStart()
                   && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                    break;
                }
            }

            //
            // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую
            // строку
            //
            if (!cursor.atStart()) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
            }

            int currentLineStartPosition = cursor.position();
            if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode,
                                    marginFromLineStart);
            } else {
                cursor.setPosition(currentLineEndPosition, cursorMoveMode);
            }
        }
    }

    editor()->setTextCursor(cursor);
}

void StandardKeyHandler::handleDown(QKeyEvent* _event)
{
    if (editor()->isCompleterVisible()) {
        return;
    }

    //
    // Если подстановщик скрыт - имитируем действие обычного редактора
    //
    const bool isShiftPressed = _event->modifiers().testFlag(Qt::ShiftModifier);
    const QTextCursor::MoveMode cursorMoveMode
        = isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

    QTextCursor cursor = editor()->textCursor();

    //
    // Исходная позиция курсора
    //
    int initCursorPosition = cursor.position();

    //
    // Рассчитаем количество символов от края
    //
    int marginFromLineStart = 0;
    {
        int currentLineYCoordinate = editor()->cursorRect(cursor).y();
        while (!cursor.atStart() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                break;
            }
        }
        marginFromLineStart = initCursorPosition - cursor.position() - (cursor.atStart() ? 0 : 1);
    }

    //
    // Вернём курсор в исходное положение
    //
    cursor.setPosition(initCursorPosition, cursorMoveMode);

    //
    // Сместим курсор к следующей строке или к концу документа
    //
    {
        int currentLineYCoordinate = editor()->cursorRect(cursor).y();
        while (!cursor.atEnd() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            if (!cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode)) {
                break;
            }
        }
    }

    //
    // В данный момент курсор либо в конце документа, либо перешёл к началу следующей строки
    //

    if (!cursor.atEnd()) {
        //
        // Если мы опустились на строку вниз, но попали в невидимый блок, перейдём к следующему
        // видимому
        //
        while (!cursor.atEnd()
               && (!cursor.block().isVisible()
                   || TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                   || cursor.blockFormat().boolProperty(PageTextEdit::PropertyDontShowCursor))) {
            cursor.movePosition(QTextCursor::NextBlock, cursorMoveMode);
            cursor.movePosition(QTextCursor::EndOfBlock, cursorMoveMode);
        }

        //
        // Сместим курсор в следующей строке на то кол-во символов, на которое он был смещён прежде
        //
        {
            int currentLineStartPosition = cursor.position();
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atEnd() && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                if (!cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode)) {
                    break;
                }
            }

            //
            // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую
            // строку
            //
            if (!cursor.atEnd()) {
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
                while (
                    !cursor.atStart()
                    && (!cursor.block().isVisible()
                        || TextBlockStyle::forBlock(cursor.block())
                            == TextParagraphType::PageSplitter
                        || cursor.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                        || cursor.blockFormat().boolProperty(
                            PageTextEdit::PropertyDontShowCursor))) {
                    if (!cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode)) {
                        break;
                    }
                }
            }

            int currentLineEndPosition = cursor.position();
            if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                const int moveRepeats
                    = currentLineEndPosition - currentLineStartPosition - marginFromLineStart;
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode, moveRepeats);
            } else {
                cursor.setPosition(currentLineEndPosition, cursorMoveMode);
            }
        }
    }

    editor()->setTextCursor(cursor);
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
    // TODO: не удалять невидимые блоки внутри выделения
    //

    //
    // Если нет выделения, то обработаем крайние случаи с удалением по краям блоков
    //
    if (!cursor.hasSelection()) {
        //
        // ... если пользователь нажимает Backspace в начале блока, перед котором идёт невидимый,
        //     то идём назад до конца первого видимого блока
        //
        if (!cursor.atStart() && cursor.positionInBlock() == 0 && _backward
            && !cursor.block().previous().isVisible()) {
            do {
                cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            } while (!cursor.block().isVisible() && !cursor.atStart());
            editor()->setTextCursorForced(cursor);
        }
        //
        // ... если пользователь нажал Delete в конце абзаца и при этом после текущего абзаца
        //     идёт невидимый блок, то идём вперёд до начала первого видимого блока
        //
        else if (!cursor.atEnd() && cursor.positionInBlock() == cursor.block().text().length()
                 && !_backward && !cursor.block().next().isVisible()) {
            do {
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            } while (!cursor.block().isVisible() && !cursor.atEnd());
            editor()->setTextCursorForced(cursor);
        }
    }

    cursor.removeCharacters(_backward, editor());
}

} // namespace KeyProcessingLayer

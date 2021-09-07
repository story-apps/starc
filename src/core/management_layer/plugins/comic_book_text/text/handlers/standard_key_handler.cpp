#include "standard_key_handler.h"

#include "../comic_book_text_edit.h"

#include <business_layer/document/comic_book/text/comic_book_text_block_data.h>
#include <business_layer/document/comic_book/text/comic_book_text_cursor.h>
#include <business_layer/templates/comic_book_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::ComicBookBlockStyle;
using BusinessLayer::ComicBookParagraphType;
using Ui::ComicBookTextEdit;


namespace KeyProcessingLayer {

namespace {
/**
 * @brief Получить тип перехода/смены в зависимости от заданных параметров
 */
static ComicBookParagraphType actionFor(bool _tab, bool _jump, ComicBookParagraphType _blockType)
{
    const QString settingsKey = QString("comicbook-editor/styles-%1/from-%2-by-%3")
                                    .arg(_jump ? "jumping" : "changing")
                                    .arg(BusinessLayer::toString(_blockType))
                                    .arg(_tab ? "tab" : "enter");

    const auto typeString
        = DataStorageLayer::StorageFacade::settingsStorage()
              ->value(settingsKey, DataStorageLayer::SettingsStorage::SettingsPlace::Application)
              .toString();

    return BusinessLayer::comicBookParagraphTypeFromString(typeString);
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


StandardKeyHandler::StandardKeyHandler(Ui::ComicBookTextEdit* _editor)
    : AbstractKeyHandler(_editor)
{
}

ComicBookParagraphType StandardKeyHandler::jumpForTab(ComicBookParagraphType _blockType)
{
    return actionFor(kTab, kJump, _blockType);
}

ComicBookParagraphType StandardKeyHandler::jumpForEnter(ComicBookParagraphType _blockType)
{
    return actionFor(kEnter, kJump, _blockType);
}

ComicBookParagraphType StandardKeyHandler::changeForTab(ComicBookParagraphType _blockType)
{
    return actionFor(kTab, kChange, _blockType);
}

ComicBookParagraphType StandardKeyHandler::changeForEnter(ComicBookParagraphType _blockType)
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
            cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
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
                   || ComicBookBlockStyle::forBlock(cursor.block())
                       == ComicBookParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(ComicBookBlockStyle::PropertyIsCorrection)
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
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
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
            cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
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
            cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
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
                   || ComicBookBlockStyle::forBlock(cursor.block())
                       == ComicBookParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(ComicBookBlockStyle::PropertyIsCorrection)
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
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
            }

            //
            // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую
            // строку
            //
            if (!cursor.atEnd()) {
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
                while (!cursor.atStart()
                       && (!cursor.block().isVisible()
                           || ComicBookBlockStyle::forBlock(cursor.block())
                               == ComicBookParagraphType::PageSplitter
                           || cursor.blockFormat().boolProperty(
                               ComicBookBlockStyle::PropertyIsCorrection)
                           || cursor.blockFormat().boolProperty(
                               PageTextEdit::PropertyDontShowCursor))) {
                    cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
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
    QTextCursor cursor = editor()->textCursor();
    cursor.beginEditBlock();

    for (int line = 0; line < 20; ++line) {
        handleUp(_event);
    }

    cursor.endEditBlock();
}

void StandardKeyHandler::handlePageDown(QKeyEvent* _event)
{
    QTextCursor cursor = editor()->textCursor();
    cursor.beginEditBlock();

    for (int line = 0; line < 20; ++line) {
        handleDown(_event);
    }

    cursor.endEditBlock();
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
    BusinessLayer::ComicBookTextCursor cursor = editor()->textCursor();
    cursor.removeCharacters(_backward, editor());
}

} // namespace KeyProcessingLayer

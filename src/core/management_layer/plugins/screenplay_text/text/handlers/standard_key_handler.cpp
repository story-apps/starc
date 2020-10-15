#include "standard_key_handler.h"

#include "../screenplay_text_block_data.h"
#include "../screenplay_text_edit.h"

#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using BusinessLayer::ScreenplayTemplateFacade;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer
{

namespace {
    /**
     * @brief Получить тип перехода/смены в зависимости от заданных параметров
     */
    static ScreenplayParagraphType actionFor(bool _tab, bool _jump, ScreenplayParagraphType _blockType) {
        const QString settingsKey =
                QString("screenplay-editor/styles-%1/from-%2-by-%3")
                .arg(_jump ? "jumping" : "changing")
                .arg(BusinessLayer::toString(_blockType))
                .arg(_tab ? "tab" : "enter");

        const auto typeString =
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    settingsKey, DataStorageLayer::SettingsStorage::SettingsPlace::Application
                    ).toString();

        return BusinessLayer::screenplayParagraphTypeFromString(typeString);
    }

    /**
     * @brief Вспомогательные константы для использования с функцией actionFor
     */
    /** @{ */
    const bool TAB = true;
    const bool ENTER = false;
    const bool JUMP = true;
    const bool CHANGE = false;
    /** @} */
}


StandardKeyHandler::StandardKeyHandler(Ui::ScreenplayTextEdit* _editor) :
    AbstractKeyHandler(_editor)
{
}

ScreenplayParagraphType StandardKeyHandler::jumpForTab(ScreenplayParagraphType _blockType)
{
    return actionFor(TAB, JUMP, _blockType);
}

ScreenplayParagraphType StandardKeyHandler::jumpForEnter(ScreenplayParagraphType _blockType)
{
    return actionFor(ENTER, JUMP, _blockType);
}

ScreenplayParagraphType StandardKeyHandler::changeForTab(ScreenplayParagraphType _blockType)
{
    return actionFor(TAB, CHANGE, _blockType);
}

ScreenplayParagraphType StandardKeyHandler::changeForEnter(ScreenplayParagraphType _blockType)
{
    return actionFor(ENTER, CHANGE, _blockType);
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
    const QTextCursor::MoveMode cursorMoveMode =
            isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

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
        while (!cursor.atStart()
               && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
        }
        marginFromLineStart =
                initCursorPosition
                - cursor.position()
                - (cursor.atStart() ? 0 : 1);
    }

    //
    // В данный момент курсор либо в начале документа, либо поднялся к концу предыдущей строки
    //

    if (!cursor.atStart()) {
        //
        // Если мы поднялись на строку вверх, но попали в невидимый блок, перейдём к предыдущему видимому
        //
        const QTextBlock firstDocumentBlock = cursor.document()->firstBlock();
        while (cursor.block() != firstDocumentBlock
               && (!cursor.block().isVisible()
                   || ScreenplayBlockStyle::forBlock(cursor.block()) == ScreenplayParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection))) {
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
            // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую строку
            //
            if (!cursor.atStart()) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
            }

            int currentLineStartPosition = cursor.position();
            if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode, marginFromLineStart);
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
    const QTextCursor::MoveMode cursorMoveMode =
            isShiftPressed ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor;

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
        while (!cursor.atStart()
               && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
        }
        marginFromLineStart =
                initCursorPosition
                - cursor.position()
                - (cursor.atStart() ? 0 : 1);
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
        while (!cursor.atEnd()
               && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
            cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
        }
    }

    //
    // В данный момент курсор либо в конце документа, либо перешёл к началу следующей строки
    //

    if (!cursor.atEnd()) {
        //
        // Если мы опустились на строку вниз, но попали в невидимый блок, перейдём к следующему видимому
        //
        while (!cursor.atEnd()
               && (!cursor.block().isVisible()
                   || ScreenplayBlockStyle::forBlock(cursor.block()) == ScreenplayParagraphType::PageSplitter
                   || cursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection))) {
            cursor.movePosition(QTextCursor::NextBlock, cursorMoveMode);
            cursor.movePosition(QTextCursor::EndOfBlock, cursorMoveMode);
        }

        //
        // Сместим курсор в следующей строке на то кол-во символов, на которое он был смещён прежде
        //
        {
            int currentLineStartPosition = cursor.position();
            int currentLineYCoordinate = editor()->cursorRect(cursor).y();
            while (!cursor.atEnd()
                   && editor()->cursorRect(cursor).y() == currentLineYCoordinate) {
                cursor.movePosition(QTextCursor::NextCharacter, cursorMoveMode);
            }

            //
            // Возвратим курсор на одну позицию назад, т.к. в предыдущем цикле мы перешли на новую строку
            //
            if (!cursor.atEnd()) {
                cursor.movePosition(QTextCursor::PreviousCharacter, cursorMoveMode);
            }

            int currentLineEndPosition = cursor.position();
            if (currentLineStartPosition + marginFromLineStart < currentLineEndPosition) {
                const int moveRepeats = currentLineEndPosition - currentLineStartPosition - marginFromLineStart;
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
    QTextCursor cursor = editor()->textCursor();

    if (!cursor.hasSelection()) {
        //
        // Если в начале документа нажат backspace
        //
        if (cursor.atStart()
            && _backward == true) {
            return;
        }

        //
        // Если в конце документа нажат delete
        //
        if (cursor.atEnd()
            && _backward == false) {
            return;
        }
    }

    //
    // Определим границы выделения
    //
    int topCursorPosition = 0;
    int bottomCursorPosition = 0;
    {
        if (cursor.hasSelection()) {
            topCursorPosition = qMin(cursor.selectionStart(), cursor.selectionEnd());
            bottomCursorPosition = qMax(cursor.selectionStart(), cursor.selectionEnd());
        } else {
            topCursorPosition = cursor.position() - (_backward ? 1 : 0);
            bottomCursorPosition = topCursorPosition + 1;

            QTextCursor checkCursor = cursor;
            checkCursor.setPosition(topCursorPosition);
            //
            // ... переходим через корректирующие блоки блоки вперёд
            //
            if (!_backward
                && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    topCursorPosition = checkCursor.position();
                    bottomCursorPosition = topCursorPosition + 1;
                }
                //
                // ... в смещении блоков, удаляем все декорации
                //
                else if (checkCursor.block().next().blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                    checkCursor.movePosition(QTextCursor::NextBlock);
                    while (!checkCursor.atEnd()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::EndOfBlock);
                        checkCursor.movePosition(QTextCursor::NextCharacter);
                    }

                    bottomCursorPosition = checkCursor.position();
                }
            }
            //
            // ... назад
            //
            else if (_backward
                     && checkCursor.atBlockEnd()) {
                //
                // ... в разрыве, удаляем символы в оторванном абзаце
                //
                if (checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionStart)
                    || checkCursor.block().next().blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsBreakCorrectionEnd)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::PreviousBlock);
                    }
                    checkCursor.movePosition(QTextCursor::EndOfBlock);
                    checkCursor.movePosition(QTextCursor::PreviousCharacter);

                    topCursorPosition = checkCursor.position();
                    bottomCursorPosition = topCursorPosition + 1;
                }
                //
                // ... в смещении блоков, удаляем все декорации
                //
                else if (checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                    while (!checkCursor.atStart()
                           && checkCursor.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                        checkCursor.movePosition(QTextCursor::PreviousBlock);
                    }
                    checkCursor.movePosition(QTextCursor::EndOfBlock);

                    topCursorPosition = checkCursor.position();
                }
            }
        }
    }

    //
    // Получим стили блоков начала и конца выделения
    //
    // ... начала
    //
    auto topParagraphType = ScreenplayParagraphType::Undefined;
    QTextBlock topBlock;
    {
        QTextCursor topCursor(editor()->document());
        topCursor.setPosition(topCursorPosition);
        topParagraphType = ScreenplayBlockStyle::forBlock(topCursor.block());
        topBlock = topCursor.block();
    }
    //
    // ... и конца
    //
    auto bottomParagraphType = ScreenplayParagraphType::Undefined;
    QTextBlock bottomBlock;
    {
        QTextCursor bottomCursor(editor()->document());
        bottomCursor.setPosition(bottomCursorPosition);
        bottomParagraphType = ScreenplayBlockStyle::forBlock(bottomCursor.block());
        bottomBlock = bottomCursor.block();
    }
    const auto topStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(topParagraphType);
    const auto bottomStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(bottomParagraphType);

    //
    // Определим стиль результирующего блока
    //
    auto targetStyle = _backward ? topStyle : bottomStyle;
    {
        if (topBlock == bottomBlock) {
            targetStyle = topStyle;
        } else {
            if (topStyle.isEmbeddable() && !bottomStyle.isEmbeddable() && !bottomBlock.text().isEmpty()) {
                targetStyle = bottomStyle;
            } else if (!topBlock.text().isEmpty()) {
                targetStyle = topStyle;
            } else if (!bottomBlock.text().isEmpty()) {
                targetStyle = bottomStyle;
            }
        }
    }

    //
    // Собственно удаление
    //
    FoldersToDelete groupsToDeleteCounts;
    cursor.beginEditBlock();
    {
        //
        // Подсчитать количество группирующих элементов входящих в выделение
        //
        const bool needToDeleteGroups
                = topBlock != bottomBlock
                  && (topStyle.isEmbeddable() || bottomStyle.isEmbeddable());
        if (needToDeleteGroups) {
            groupsToDeleteCounts = findGroupCountsToDelete(topCursorPosition, bottomCursorPosition);
        }

        //
        // В результирующем блоке должны остаться данные от верхнего блока
        //
        BusinessLayer::ScreenplayTextBlockData* blockData = nullptr;
        if (topBlock.userData() != nullptr) {
            auto topBlockData = static_cast<BusinessLayer::ScreenplayTextBlockData*>(topBlock.userData());
            if (topBlockData != nullptr) {
                blockData = new BusinessLayer::ScreenplayTextBlockData(topBlockData);
            }
        }


        //
        // Удалить текст
        //
        cursor.setPosition(topCursorPosition);
        cursor.setPosition(bottomCursorPosition, QTextCursor::KeepAnchor);
        cursor.removeSelectedText();

        //
        // Положим корректные данные в блок
        //
        cursor.block().setUserData(blockData);

        //
        // Удалить вторые половинки группирующих элементов
        //
        if (needToDeleteGroups) {
            removeGroupsPairs(cursor.position(), groupsToDeleteCounts);
        }
    }

    //
    // Если и верхний и нижний блоки являются началом и концом папки,
    //
    if (topParagraphType == ScreenplayParagraphType::FolderHeader
        && bottomParagraphType == ScreenplayParagraphType::FolderFooter) {
        //
        // ... то ничего не делаем, весь мусорный текст был удалён выше
        //
    }
    //
    // Применим финальный стиль
    //
    else {
        //
        // Если стиль последнего абзаца сменился при удалении, корректируем его
        //
        if (editor()->currentParagraphType() != targetStyle.type()) {
            editor()->setCurrentParagraphType(targetStyle.type());
        }
        //
        // В противном случае, при удалении, формат текста остаётся от предыдущего блока,
        // поэтому перезапишем его корректным форматом
        //
        else {
            cursor.setBlockCharFormat(targetStyle.charFormat());
        }
    }

    //
    // Завершим операцию удаления
    //
    cursor.endEditBlock();
}

StandardKeyHandler::FoldersToDelete StandardKeyHandler::findGroupCountsToDelete(int _topCursorPosition, int _bottomCursorPosition)
{
    FoldersToDelete groupCountsToDelete;

    //
    // Начнём поиск с заданной позиции
    //
    QTextCursor searchGroupsCursor(editor()->document());
    searchGroupsCursor.setPosition(_topCursorPosition);

    //
    // Пропускаем поиск в первом блоке области удаления, т.к. он всегда остаётся с текущим стилем
    //
    searchGroupsCursor.movePosition(QTextCursor::NextBlock);

    while (searchGroupsCursor.position() <= _bottomCursorPosition) {
        //
        // Определим тип блока
        //
        ScreenplayParagraphType currentType =
                ScreenplayBlockStyle::forBlock(searchGroupsCursor.block());

        //
        // Если найден блок открывающий папку, то нужно удалить закрывающий блок
        //
        if (currentType == ScreenplayParagraphType::FolderHeader) {
            ++groupCountsToDelete.footers;
        }
        //
        // Если найден блок закрывающий папку
        // ... если все группы закрыты, нужно удалить предыдущую открытую
        // ... в противном случае закрываем открытую группу
        //
        else if (currentType == ScreenplayParagraphType::FolderFooter) {
            if (groupCountsToDelete.footers == 0) {
                ++groupCountsToDelete.headers;
            }
            else {
                --groupCountsToDelete.footers;
            }
        }

        //
        // Если дошли до конца, прерываем выполнение
        //
        if (searchGroupsCursor.atEnd()) {
            break;
        }

        //
        // Перейдём к следующему блоку или концу блока
        //
        searchGroupsCursor.movePosition(QTextCursor::EndOfBlock);
        searchGroupsCursor.movePosition(QTextCursor::NextBlock);
    }

    return groupCountsToDelete;
}

void StandardKeyHandler::removeGroupsPairs(int _cursorPosition, const FoldersToDelete& _groupCountsToDelete)
{
    //
    // Удалим пары из последующего текста
    //
    // ... папки
    //
    if (_groupCountsToDelete.footers > 0) {
        QTextCursor cursor(editor()->document());
        cursor.setPosition(_cursorPosition);

        // ... открытые группы на пути поиска необходимого для удаления блока
        int openedGroups = 0;
        int groupsToDeleteCount = _groupCountsToDelete.footers;
        do {
            const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == ScreenplayParagraphType::FolderFooter) {
                if (openedGroups == 0) {
                    cursor.select(QTextCursor::BlockUnderCursor);
                    cursor.deleteChar();

                    //
                    // Если жто был самый последний блок
                    //
                    if (cursor.atEnd()) {
                        cursor.deletePreviousChar();
                    }

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScreenplayParagraphType::FolderHeader) {
                // ... встретилась новая группа, которую не нужно удалять
                ++openedGroups;
            }

            if (cursor.atEnd()) {
                Q_ASSERT(false);
                break;
            }

            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);
        } while (groupsToDeleteCount > 0);
    }

    //
    // Удалим пары из предшествующего текста
    //
    // ... папки
    //
    if (_groupCountsToDelete.headers > 0) {
        QTextCursor cursor = editor()->textCursor();
        cursor.setPosition(_cursorPosition);

        // ... открытые группы на пути поиска необходимого для удаления блока
        int openedGroups = 0;
        int groupsToDeleteCount = _groupCountsToDelete.headers;
        do {
            const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == ScreenplayParagraphType::FolderHeader) {
                if (openedGroups == 0) {
                    cursor.select(QTextCursor::BlockUnderCursor);
                    cursor.deleteChar();

                    //
                    // Если это был самый первый блок
                    //
                    if (cursor.atStart()) {
                        cursor.deleteChar();
                    }

                    --groupsToDeleteCount;

                    continue;
                } else {
                    --openedGroups;
                }
            } else if (currentType == ScreenplayParagraphType::FolderFooter) {
                // ... встретилась новая группа, которую не нужно удалять
                ++openedGroups;
            }

            if (cursor.atStart()) {
                break;
            }

            cursor.movePosition(QTextCursor::PreviousBlock);
        } while (groupsToDeleteCount > 0);
    }
}

} // namespace KeyProcessingLayer

#include "dialog_handler.h"

#include "../stageplay_text_edit.h"

#include <business_layer/templates/stageplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::StageplayTextEdit;


namespace KeyProcessingLayer {

DialogHandler::DialogHandler(StageplayTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void DialogHandler::handleEnter(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text().trimmed();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Ни чего не делаем
        //
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Ни чего не делаем
            //
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль блока на описание действия
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::Dialogue));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Перейдём к блоку персонажа
                    //
                    editor()->addParagraph(jumpForEnter(TextParagraphType::Dialogue));
                } else {
                    //! Внутри блока

                    //
                    // Разделим блок на две реплики
                    //
                    editor()->addParagraph(TextParagraphType::Dialogue);
                }
            }
        }
    }
}

void DialogHandler::handleTab(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());


    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Ни чего не делаем
        //
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Ни чего не делаем
            //
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль на ремарку
                //
                editor()->addParagraph(changeForTab(TextParagraphType::Dialogue));
            } else {
                //! Текст не пуст

                if (!cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставляем блок ремарки
                    //
                    editor()->addParagraph(jumpForTab(TextParagraphType::Dialogue));
                } else {
                    //! В начале блока
                    //! Внутри блока

                    //
                    // Ни чего не делаем
                    //
                }
            }
        }
    }
}

void DialogHandler::handleBackspace(QKeyEvent* _event)
{
    //
    // Если диалоги располагаются в таблице, то при нажатии бэкспейса в начале блока, просто
    // переходим в конец предыдущего абзаца (предыдущей колонки)
    //
    auto cursor = editor()->textCursor();
    if (editor()->stageplayTemplate().placeDialoguesInTable() && cursor.atBlockStart()
        && !cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
        editor()->setTextCursor(cursor);
        return;
    }

    StandardKeyHandler::handleBackspace(_event);
}

void DialogHandler::handleOther(QKeyEvent* _event)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    const QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    const QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());


    //
    // Обработка
    //
    if (editor()->stageplayTemplate().paragraphStyle(TextParagraphType::Parenthetical).isActive()
        && cursorBackwardText.endsWith("(") && _event != 0 && _event->text() == "(") {
        //! Если нажата открывающая скобка

        //
        // Удалим лишнюю скобку
        //
        editor()->textCursor().deletePreviousChar();

        if (cursorForwardText.isEmpty() && cursorBackwardText == "(") {
            //! Если текст пуст

            //
            // Cменить стиль на ремарку
            //
            editor()->setCurrentParagraphType(TextParagraphType::Parenthetical);
        } else {
            //! Если текст не пуст

            //
            // Разрываем диалог ремаркой
            //

            //
            // ... оставляем пустой блок реплики
            //
            // если скобка нажата в начале строки, то делаем лишь один перевод строки
            //
            if (cursorBackwardText != "(") {
                editor()->addParagraph(TextParagraphType::Dialogue);
            }
            //
            // ... если после скобки нет текста, не добавляем новый параграф
            //
            if (!cursorForwardText.isEmpty()) {
                editor()->addParagraph(TextParagraphType::Dialogue);

                //
                // ... возвращаем курсор к пустому блоку
                //
                cursor = editor()->textCursor();
                cursor.movePosition(QTextCursor::PreviousBlock);
                editor()->setTextCursorAndKeepScrollBars(cursor);
            }

            //
            // ... делаем блок под курсором ремаркой
            //
            editor()->setCurrentParagraphType(TextParagraphType::Parenthetical);
        }
    } else {
        //! В противном случае, обрабатываем в базовом классе

        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer

#include "inline_note_handler.h"

#include "../simple_text_edit.h"

#include <business_layer/templates/text_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::SimpleTextEdit;


namespace KeyProcessingLayer {

InlineNoteHandler::InlineNoteHandler(Ui::SimpleTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void InlineNoteHandler::handleEnter(QKeyEvent*)
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
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(editor()->currentParagraphType());
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Ни чего не делаем
                //
                editor()->setCurrentParagraphType(changeForEnter(editor()->currentParagraphType()));
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
                    // Вставляем блок и применяем ему стиль описания действия
                    //
                    editor()->addParagraph(jumpForEnter(editor()->currentParagraphType()));
                } else {
                    //! Внутри блока

                    //
                    // Вставляем блок и применяем ему стиль описания действия
                    //
                    editor()->addParagraph(TextParagraphType::Text);
                }
            }
        }
    }
}

void InlineNoteHandler::handleTab(QKeyEvent*)
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
                // Ни чего не делаем
                //
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
                    // Действуем как нажатие клавиши ENTER
                    //
                    handleEnter();
                } else {
                    //! Внутри блока

                    //
                    // Ни чего не делаем
                    //
                }
            }
        }
    }
}

} // namespace KeyProcessingLayer

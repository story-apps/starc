#include "dialog_handler.h"

#include "../audioplay_text_edit.h"

#include <business_layer/templates/audioplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::AudioplayTextEdit;


namespace KeyProcessingLayer {

DialogHandler::DialogHandler(AudioplayTextEdit* _editor)
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
    auto cursor = editor()->textCursor();
    if (cursor.atBlockStart() && !cursor.hasSelection()) {
        cursor.movePosition(QTextCursor::PreviousCharacter);
        editor()->setTextCursor(cursor);
        return;
    }

    StandardKeyHandler::handleBackspace(_event);
}

} // namespace KeyProcessingLayer

#include "act_footer_handler.h"

#include "../screenplay_treatment_edit.h"

#include <business_layer/templates/screenplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTreatmentEdit;

namespace KeyProcessingLayer {

ActFooterHandler::ActFooterHandler(ScreenplayTreatmentEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void ActFooterHandler::handleEnter(QKeyEvent*)
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
                // Вставить блок
                //
                editor()->addParagraph(TextParagraphType::ActHeading);
                editor()->moveCursor(QTextCursor::PreviousCharacter);
                editor()->ensureCursorVisible(editor()->textCursor());
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    editor()->addParagraph(TextParagraphType::SceneHeading);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    editor()->addParagraph(TextParagraphType::ActHeading);
                    editor()->moveCursor(QTextCursor::PreviousCharacter);
                    editor()->ensureCursorVisible(editor()->textCursor());
                } else {
                    //! Внутри блока

                    editor()->addParagraph(TextParagraphType::ActHeading);
                    editor()->ensureCursorVisible(editor()->textCursor());
                }
            }
        }
    }
}

void ActFooterHandler::handleTab(QKeyEvent*)
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
                    // Как ENTER
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

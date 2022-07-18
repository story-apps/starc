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
                // Вставить блок время и место
                //
                editor()->addParagraph(TextParagraphType::SceneHeading);
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставить блок время и место перед папкой
                    //
                    cursor.insertBlock();
                    cursor.movePosition(QTextCursor::PreviousCharacter);
                    cursor.setBlockFormat(QTextBlockFormat());
                    editor()->setTextCursor(cursor);
                    editor()->setCurrentParagraphType(TextParagraphType::SceneHeading);
                    editor()->moveCursor(QTextCursor::NextCharacter);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставить блок время и место
                    //
                    editor()->addParagraph(TextParagraphType::SceneHeading);
                } else {
                    //! Внутри блока

                    //
                    // Вставить блок время и место
                    //
                    editor()->addParagraph(TextParagraphType::SceneHeading);
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

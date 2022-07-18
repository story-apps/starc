#include "act_heading_handler.h"

#include "../screenplay_treatment_edit.h"

#include <business_layer/templates/screenplay_template.h>

#include <QCoreApplication>
#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTreatmentEdit;


namespace KeyProcessingLayer {

ActHeadingHandler::ActHeadingHandler(ScreenplayTreatmentEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void ActHeadingHandler::handleEnter(QKeyEvent*)
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
                // Меняем стиль в соответствии с настройками
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::ActHeading));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставка блока заголовка перед собой
                    //
                    editor()->addParagraph(TextParagraphType::SceneHeading);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставить блок время и место
                    //
                    editor()->addParagraph(jumpForEnter(TextParagraphType::ActHeading));
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

void ActHeadingHandler::handleTab(QKeyEvent*)
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
                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::ActHeading));
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
                    editor()->addParagraph(jumpForTab(TextParagraphType::ActHeading));
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

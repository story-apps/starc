#include "beat_heading_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer {

BeatHeadingHandler::BeatHeadingHandler(Ui::ScreenplayTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void BeatHeadingHandler::handleEnter(QKeyEvent*)
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
            editor()->addParagraph(TextParagraphType::BeatHeading);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::BeatHeading));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    editor()->addParagraph(jumpForEnter(TextParagraphType::BeatHeading));
                } else {
                    //! Внутри блока

                    editor()->addParagraph(TextParagraphType::BeatHeading);
                }
            }
        }
    }
}

void BeatHeadingHandler::handleTab(QKeyEvent*)
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

                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::BeatHeading));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    editor()->addParagraph(jumpForTab(TextParagraphType::BeatHeading));
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

void BeatHeadingHandler::handleOther(QKeyEvent* _event)
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


    //
    // Обработка
    //
    if (cursorBackwardText.endsWith(".") && _event != 0 && _event->text() == ".") {
        //! Если нажата точка

        //
        // Если было введено какое-либо значение из списка мест (ИНТ./НАТ. и т.п.)
        // то необходимо преобразовать блок во время и место
        //
        const QString maybeSceneIntro = TextHelper::smartToUpper(cursorBackwardText);
        if (editor()->dictionaries()->sceneIntros().contains(maybeSceneIntro)) {
            editor()->setCurrentParagraphType(TextParagraphType::SceneHeading);
        }
    } else if (cursorBackwardText.endsWith(":") && _event != 0 && _event->text() == ":") {
        //! Если нажата двоеточие

        //
        // Если было введено какое-либо значение из списка переходов
        // то необходимо преобразовать блок в переход
        //
        const QString maybeTransition = TextHelper::smartToUpper(cursorBackwardText);
        if (editor()->dictionaries()->transitions().contains(maybeTransition)) {
            editor()->setCurrentParagraphType(TextParagraphType::Transition);
        }
    } else {
        //! В противном случае, обрабатываем в базовом классе

        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer

#include "action_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::ScreenplayParagraphType;
using Ui::ScreenplayTextEdit;

namespace KeyProcessingLayer {

ActionHandler::ActionHandler(ScreenplayTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void ActionHandler::handleEnter(QKeyEvent*)
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
            editor()->addParagraph(ScreenplayParagraphType::Action);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль на место и время
                //
                editor()->setCurrentParagraphType(changeForEnter(ScreenplayParagraphType::Action));
            } else {
                //! Текст не пуст

                //
                // Если введён персонаж, меняем стиль блока и переходим к реплике
                //
                if (cursorForwardText.isEmpty()
                    && editor()->characters()->exists(cursorBackwardText)) {
                    editor()->setCurrentParagraphType(ScreenplayParagraphType::Character);
                    editor()->addParagraph(ScreenplayParagraphType::Dialogue);
                }
                //
                // Вставляем блок и применяем ему стиль описания действия
                //
                else {
                    editor()->addParagraph(jumpForEnter(ScreenplayParagraphType::Action));
                }
            }
        }
    }
}

void ActionHandler::handleTab(QKeyEvent*)
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
                // Если строка пуста, то сменить стиль на имя героя
                //
                editor()->setCurrentParagraphType(changeForTab(ScreenplayParagraphType::Action));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Меняем на блок персонажа
                    //
                    editor()->setCurrentParagraphType(ScreenplayParagraphType::Character);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставляем блок персонажа
                    //
                    editor()->addParagraph(jumpForTab(ScreenplayParagraphType::Action));
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

void ActionHandler::handleOther(QKeyEvent* _event)
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
            editor()->setCurrentParagraphType(ScreenplayParagraphType::SceneHeading);
        }
    } else if (cursorBackwardText.endsWith(":") && _event != 0 && _event->text() == ":") {
        //! Если нажата двоеточие

        //
        // Если было введено какое-либо значение из списка переходов
        // то необходимо преобразовать блок в переход
        //
        const QString maybeTransition = TextHelper::smartToUpper(cursorBackwardText);
        if (editor()->dictionaries()->transitions().contains(maybeTransition)) {
            editor()->setCurrentParagraphType(ScreenplayParagraphType::Transition);
        }
    } else {
        //! В противном случае, обрабатываем в базовом классе

        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer

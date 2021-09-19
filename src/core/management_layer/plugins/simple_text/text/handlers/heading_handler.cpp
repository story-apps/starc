#include "heading_handler.h"

#include "../simple_text_edit.h"

#include <business_layer/templates/simple_text_template.h>

#include <QKeyEvent>
#include <QStringListModel>
#include <QTextBlock>
#include <QTimer>

using BusinessLayer::TextParagraphType;
using Ui::SimpleTextEdit;


namespace KeyProcessingLayer {

HeadingHandler::HeadingHandler(Ui::SimpleTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void HeadingHandler::handleEnter(QKeyEvent* _event)
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
            // Меняем в соответствии с настройками
            //
            editor()->setCurrentParagraphType(changeForEnter(editor()->currentParagraphType()));
        } else {
            //! Текст не пуст

            if (cursorBackwardText.isEmpty()) {
                //! В начале блока

                //
                // Вставка блока заголовка перед собой
                //
                editor()->addParagraph(editor()->currentParagraphType());
            } else if (cursorForwardText.isEmpty()) {
                //! В конце блока

                //
                // Вставка блока описания действия
                //
                editor()->addParagraph(jumpForEnter(editor()->currentParagraphType()));
            } else {
                //! Внутри блока

                //
                // Вставка блока описания действия
                //
                editor()->addParagraph(TextParagraphType::Text);
            }
        }
    }
}

void HeadingHandler::handleTab(QKeyEvent*)
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
        // Работаем аналогично нажатию ENTER
        //
        handleEnter();
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
                // Если строка пуста, то сменить стиль на описание действия
                //
                editor()->setCurrentParagraphType(changeForTab(editor()->currentParagraphType()));
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
                    // Вставим блок
                    //
                    editor()->addParagraph(jumpForTab(editor()->currentParagraphType()));
                }
            }
        }
    }
}

} // namespace KeyProcessingLayer

#include "panel_handler.h"

#include "../comic_book_text_edit.h"

#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::ComicBookParagraphType;
using Ui::ComicBookTextEdit;


namespace KeyProcessingLayer {

PanelHandler::PanelHandler(ComicBookTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void PanelHandler::handleEnter(QKeyEvent*)
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
            editor()->addParagraph(ComicBookParagraphType::Panel);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Ни чего не делаем
                //
                editor()->setCurrentParagraphType(changeForEnter(ComicBookParagraphType::Panel));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока
                    bool isHandled = false;

                    //
                    // Если введён персонаж, меняем стиль блока и переходим к реплике
                    //
                    if (cursorForwardText.isEmpty()) {
                        //
                        // Потенциально была введена страница
                        //
                        const QString backwardTextCorrected
                            = TextHelper::smartToLower(cursorBackwardText.trimmed());
                        if (editor()->dictionaries()->singlePageIntros().contains(
                                backwardTextCorrected)
                            || editor()->dictionaries()->multiplePageIntros().contains(
                                backwardTextCorrected)) {
                            editor()->setCurrentParagraphType(ComicBookParagraphType::Page);
                            editor()->addParagraph(jumpForEnter(ComicBookParagraphType::Page));
                            isHandled = true;
                        }
                    }
                    //
                    // Вставляем блок и применяем ему стиль описания действия
                    //
                    if (!isHandled) {
                        editor()->addParagraph(jumpForEnter(ComicBookParagraphType::Panel));
                    }
                } else {
                    //! Внутри блока

                    //
                    // Вставляем блок и применяем ему стиль описания действия
                    //
                    editor()->addParagraph(ComicBookParagraphType::Description);
                }
            }
        }
    }
}

void PanelHandler::handleTab(QKeyEvent*)
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
                editor()->setCurrentParagraphType(changeForTab(ComicBookParagraphType::Panel));
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
                    editor()->addParagraph(jumpForTab(ComicBookParagraphType::Panel));
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

void PanelHandler::handleOther(QKeyEvent* _event)
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
    if (cursorBackwardText.endsWith(" ") && _event != 0 && _event->text() == " ") {
        //
        // Потенциально была введена страница
        //
        const QString backwardTextCorrected
            = TextHelper::smartToLower(cursorBackwardText.trimmed());
        if (editor()->dictionaries()->singlePageIntros().contains(backwardTextCorrected)
            || editor()->dictionaries()->multiplePageIntros().contains(backwardTextCorrected)) {
            editor()->setCurrentParagraphType(ComicBookParagraphType::Page);
        }
    } else {
        //! В противном случае, обрабатываем в базовом классе

        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer

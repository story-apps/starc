#include "beat_heading_handler.h"

#include "../novel_outline_edit.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/novel/novel_dictionaries_model.h>
#include <business_layer/templates/novel_template.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QString>
#include <QTextBlock>
#include <QVector>

using BusinessLayer::TextParagraphType;
using Ui::NovelOutlineEdit;

namespace KeyProcessingLayer {

BeatHeadingHandler::BeatHeadingHandler(NovelOutlineEdit* _editor)
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
            // Ничего не делаем
            //
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::BeatHeading));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    editor()->addParagraph(TextParagraphType::BeatHeading);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    editor()->addParagraph(jumpForEnter(TextParagraphType::BeatHeading));
                } else {
                    //! Внутри блока

                    editor()->addParagraph(jumpForEnter(TextParagraphType::BeatHeading));
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

                //
                // Если строка пуста, то сменить стиль на имя героя
                //
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

} // namespace KeyProcessingLayer

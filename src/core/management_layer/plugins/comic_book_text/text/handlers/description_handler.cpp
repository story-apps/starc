#include "description_handler.h"

#include "../comic_book_text_edit.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QRegularExpression>
#include <QTextBlock>

using BusinessLayer::TextParagraphType;
using Ui::ComicBookTextEdit;

namespace KeyProcessingLayer {

DescriptionHandler::DescriptionHandler(ComicBookTextEdit* _editor)
    : StandardKeyHandler(_editor)
{
}

void DescriptionHandler::handleEnter(QKeyEvent*)
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
            editor()->addParagraph(TextParagraphType::Description);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль на место и время
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::Description));
            } else {
                //! Текст не пуст
                bool isHandled = false;

                //
                // Если введён персонаж, меняем стиль блока и переходим к реплике
                //
                const auto charactersModel
                    = qobject_cast<BusinessLayer::CharactersModel*>(editor()->characters());
                if (cursorForwardText.isEmpty()) {
                    if (cursorForwardText.isEmpty() && charactersModel
                        && charactersModel->exists(cursorBackwardText)) {
                        editor()->setCurrentParagraphType(TextParagraphType::Character);
                        editor()->addParagraph(TextParagraphType::Dialogue);
                        isHandled = true;
                    } else {
                        //
                        // Потенциально была введена страница или панель
                        //
                        const QString backwardTextCorrected
                            = TextHelper::smartToLower(cursorBackwardText.trimmed());
                        if (editor()->dictionaries()->singlePageIntros().contains(
                                backwardTextCorrected)
                            || editor()->dictionaries()->multiplePageIntros().contains(
                                backwardTextCorrected)) {
                            editor()->setCurrentParagraphType(TextParagraphType::PageHeading);
                            editor()->addParagraph(jumpForEnter(TextParagraphType::PageHeading));
                            isHandled = true;
                        } else if (editor()->dictionaries()->panelIntros().contains(
                                       backwardTextCorrected)) {
                            editor()->setCurrentParagraphType(TextParagraphType::PanelHeading);
                            editor()->addParagraph(jumpForEnter(TextParagraphType::PanelHeading));
                            isHandled = true;
                        }
                    }
                }
                //
                // Вставляем блок и применяем ему стиль описания действия
                //
                if (!isHandled) {
                    editor()->addParagraph(jumpForEnter(TextParagraphType::Description));
                }
            }
        }
    }
}

void DescriptionHandler::handleTab(QKeyEvent*)
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
                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::Description));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Меняем на блок персонажа
                    //
                    editor()->setCurrentParagraphType(TextParagraphType::Character);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставляем блок персонажа
                    //
                    editor()->addParagraph(jumpForTab(TextParagraphType::Description));
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

void DescriptionHandler::handleOther(QKeyEvent* _event)
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
    if (cursorBackwardText.contains(QRegularExpression("( |:)$")) && _event != nullptr
        && (_event->text() == " " || _event->text() == ":") && cursorForwardText.isEmpty()) {
        //
        // Потенциально была введена страница или панель
        //
        QString backwardTextCorrected = TextHelper::smartToLower(cursorBackwardText.trimmed());
        if (_event->text() == ":") {
            backwardTextCorrected.chop(1);
        }
        if (editor()->dictionaries()->singlePageIntros().contains(backwardTextCorrected)
            || editor()->dictionaries()->multiplePageIntros().contains(backwardTextCorrected)) {
            editor()->setCurrentParagraphType(TextParagraphType::PageHeading);
        } else if (editor()->dictionaries()->panelIntros().contains(backwardTextCorrected)) {
            editor()->setCurrentParagraphType(TextParagraphType::PanelHeading);
        }
    } else {
        //! В противном случае, обрабатываем в базовом классе

        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer

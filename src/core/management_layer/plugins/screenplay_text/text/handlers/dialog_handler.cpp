#include "dialog_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/templates/screenplay_template.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer
{

DialogHandler::DialogHandler(ScreenplayTextEdit* _editor)
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
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(ScreenplayParagraphType::Dialogue);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль блока на описание действия
                //
                editor()->setCurrentParagraphType(changeForEnter(ScreenplayParagraphType::Dialogue));
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
                    editor()->addParagraph(jumpForEnter(ScreenplayParagraphType::Dialogue));
                } else {
                    //! Внутри блока

                    //
                    // Разрываем диалог блоком персонажа, вставляя его имя
                    //
                    {
                        //
                        // Найти име персонажа, кому принадлежит реплика
                        //
                        QString characterName;
                        {
                            QTextCursor cursor = editor()->textCursor();
                            QTextBlock cursorBlock = cursor.block();
                            while ((ScreenplayBlockStyle::forBlock(cursorBlock) != ScreenplayParagraphType::Character
                                    || ScreenplayBlockStyle::forBlock(cursorBlock) == ScreenplayParagraphType::Dialogue
                                    || ScreenplayBlockStyle::forBlock(cursorBlock) == ScreenplayParagraphType::Parenthetical
                                    || ScreenplayBlockStyle::forBlock(cursorBlock) == ScreenplayParagraphType::Lyrics)
                                   && !cursor.atStart()) {
                                cursor.movePosition(QTextCursor::PreviousBlock);
                                cursorBlock = cursor.block();
                            }

                            if (ScreenplayBlockStyle::forBlock(cursorBlock) == ScreenplayParagraphType::Character) {
                                characterName = cursorBlock.text().simplified();
                            }
                        }

                        //
                        // Вставляем блок "герой" и добавляем имя
                        //
                        editor()->addParagraph(ScreenplayParagraphType::Character);
                        editor()->insertPlainText(characterName);

                        //
                        // Оставшийся текст форматируем, как "диалог"
                        //
                        editor()->addParagraph(ScreenplayParagraphType::Dialogue);
                    }
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

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль на ремарку
                //
                editor()->setCurrentParagraphType(changeForTab(ScreenplayParagraphType::Dialogue));
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
                    // Вставляем блок ремарки
                    //
                    editor()->addParagraph(jumpForTab(ScreenplayParagraphType::Dialogue));
                } else {
                    //! Внутри блока

                    //
                    // Разрываем диалог ремаркой
                    //

                    //
                    // ... оставляем пустой блок реплики
                    //
                    editor()->addParagraph(ScreenplayParagraphType::Dialogue);
                    editor()->addParagraph(ScreenplayParagraphType::Dialogue);

                    //
                    // ... возвращаем курсор к пустому блоку
                    //
                    cursor = editor()->textCursor();
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    editor()->setTextCursorReimpl(cursor);

                    //
                    // ... делаем блок под курсором ремаркой
                    //
                    editor()->setCurrentParagraphType(ScreenplayParagraphType::Parenthetical);
                }
            }
        }
    }
}

void DialogHandler::handleOther(QKeyEvent* _event)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст до курсора
    const QString cursorBackwardText = currentBlock.text().left(cursor.positionInBlock());
    // ... текст после курсора
    const QString cursorForwardText = currentBlock.text().mid(cursor.positionInBlock());


    //
    // Обработка
    //
    if (cursorBackwardText.endsWith("(")
        && _event != 0
        && _event->text() == "(") {
        //! Если нажата открывающая скобка

        //
        // Удалим лишнюю скобку
        //
        editor()->textCursor().deletePreviousChar();

        if (cursorForwardText.isEmpty()
            && cursorBackwardText == "(") {
            //! Если текст пуст

            //
            // Cменить стиль на ремарку
            //
            editor()->setCurrentParagraphType(ScreenplayParagraphType::Parenthetical);
        } else {
            //! Если текст не пуст

            //
            // Разрываем диалог ремаркой
            //

            //
            // ... оставляем пустой блок реплики
            //
            // если скобка нажата в начале строки, то делаем лишь один перевод строки
            //
            if (cursorBackwardText != "(") {
                editor()->addParagraph(ScreenplayParagraphType::Dialogue);
            }
            //
            // ... если после скобки нет текста, не добавляем новый параграф
            //
            if (!cursorForwardText.isEmpty()) {
                editor()->addParagraph(ScreenplayParagraphType::Dialogue);

                //
                // ... возвращаем курсор к пустому блоку
                //
                cursor = editor()->textCursor();
                cursor.movePosition(QTextCursor::PreviousBlock);
                editor()->setTextCursorReimpl(cursor);
            }

            //
            // ... делаем блок под курсором ремаркой
            //
            editor()->setCurrentParagraphType(ScreenplayParagraphType::Parenthetical);
        }
    } else {
        //! В противном случае, обрабатываем в базовом классе

        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer

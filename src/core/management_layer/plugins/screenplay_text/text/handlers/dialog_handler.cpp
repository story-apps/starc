#include "dialog_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <QKeyEvent>
#include <QTextBlock>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer {

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
            editor()->addParagraph(TextParagraphType::Dialogue);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль блока на описание действия
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::Dialogue));
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
                    editor()->addParagraph(jumpForEnter(TextParagraphType::Dialogue));
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
                            while ((TextBlockStyle::forBlock(cursorBlock)
                                        != TextParagraphType::Character
                                    || TextBlockStyle::forBlock(cursorBlock)
                                        == TextParagraphType::Dialogue
                                    || TextBlockStyle::forBlock(cursorBlock)
                                        == TextParagraphType::Parenthetical
                                    || TextBlockStyle::forBlock(cursorBlock)
                                        == TextParagraphType::Lyrics)
                                   && !cursor.atStart()) {
                                cursor.movePosition(QTextCursor::PreviousBlock);
                                cursor.movePosition(QTextCursor::StartOfBlock);
                                cursorBlock = cursor.block();
                            }

                            if (TextBlockStyle::forBlock(cursorBlock)
                                == TextParagraphType::Character) {
                                //
                                // ... оставляем только имя персонажа
                                //
                                characterName = BusinessLayer::ScreenplayCharacterParser::name(
                                    cursorBlock.text());
                            }
                        }

                        //
                        // Вставляем блок "герой" и добавляем имя
                        //
                        editor()->addParagraph(TextParagraphType::Character);
                        editor()->insertPlainText(characterName);

                        //
                        // Оставшийся текст форматируем, как "диалог"
                        //
                        editor()->addParagraph(TextParagraphType::Dialogue);
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

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль на ремарку
                //
                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::Dialogue));
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
                    editor()->addParagraph(jumpForTab(TextParagraphType::Dialogue));
                } else {
                    //! Внутри блока

                    //
                    // Разрываем диалог ремаркой
                    //

                    //
                    // ... оставляем пустой блок реплики
                    //
                    editor()->addParagraph(TextParagraphType::Dialogue);
                    editor()->addParagraph(TextParagraphType::Dialogue);

                    //
                    // ... возвращаем курсор к пустому блоку
                    //
                    cursor = editor()->textCursor();
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    editor()->setTextCursorAndKeepScrollBars(cursor);

                    //
                    // ... делаем блок под курсором ремаркой
                    //
                    editor()->setCurrentParagraphType(TextParagraphType::Parenthetical);
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
    // ... необходимо ли менять стиль при открытой скобке на ремарку
    const bool needToCheckOpenBracket
        = settingsValue(DataStorageLayer::
                            kComponentsScreenplayEditorUseOpenBracketInDialogueForParentheticalKey)
              .toBool();


    //
    // Обработка
    //
    if (needToCheckOpenBracket && cursorBackwardText.endsWith("(") && _event != nullptr
        && _event->text() == "(") {
        //! Если нажата открывающая скобка

        //
        // Удалим лишнюю скобку
        //
        editor()->textCursor().deletePreviousChar();

        if (cursorForwardText.isEmpty() && cursorBackwardText == "(") {
            //! Если текст пуст

            //
            // Cменить стиль на ремарку
            //
            editor()->setCurrentParagraphType(TextParagraphType::Parenthetical);
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
                editor()->addParagraph(TextParagraphType::Dialogue);
            }
            //
            // ... если после скобки нет текста, не добавляем новый параграф
            //
            if (!cursorForwardText.isEmpty()) {
                editor()->addParagraph(TextParagraphType::Dialogue);

                //
                // ... возвращаем курсор к пустому блоку
                //
                cursor = editor()->textCursor();
                cursor.movePosition(QTextCursor::PreviousBlock);
                editor()->setTextCursorAndKeepScrollBars(cursor);
            }

            //
            // ... делаем блок под курсором ремаркой
            //
            editor()->setCurrentParagraphType(TextParagraphType::Parenthetical);
        }
    } else {
        //! В противном случае, обрабатываем в базовом классе

        StandardKeyHandler::handleOther(_event);
    }
}

} // namespace KeyProcessingLayer

#include "panel_handler.h"

#include "../comic_book_text_edit.h"

#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/templates/comic_book_template.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QKeyEvent>
#include <QStringListModel>
#include <QTextBlock>
#include <QTimer>

using BusinessLayer::TextParagraphType;
using Ui::ComicBookTextEdit;


namespace KeyProcessingLayer {

PanelHandler::PanelHandler(ComicBookTextEdit* _editor)
    : StandardKeyHandler(_editor)
    , m_completerModel(new QStringListModel(_editor))
{
}

void PanelHandler::prehandle()
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());

    complete(currentBlockText, cursorBackwardText);
}

void PanelHandler::handleEnter(QKeyEvent* _event)
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
        // Вставить выбранный вариант
        //
        editor()->applyCompletion();

        //
        // Обновим курсор, т.к. после автозавершения он смещается
        //
        cursor = editor()->textCursor();

        //
        // Покажем подсказку, если это возможно
        //
        handleOther();

        //
        // Если нужно автоматически перепрыгиваем к следующему блоку
        //
        if (_event != nullptr) { // ... чтобы таб не переводил на новую строку
            //
            // Переходим к следующему блоку
            //
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addParagraph(jumpForEnter(TextParagraphType::PanelHeading));
        } else {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.insertText(": ");
            editor()->setTextCursor(cursor);
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(TextParagraphType::PanelHeading);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Меняем стиль блока на страницу
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::PanelHeading));
            } else {
                //! Текст не пуст

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставляем блок панели перед собой
                    //
                    editor()->addParagraph(TextParagraphType::PanelHeading);
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
                                backwardTextCorrected, Qt::CaseInsensitive)
                            || editor()->dictionaries()->multiplePageIntros().contains(
                                backwardTextCorrected, Qt::CaseInsensitive)) {
                            editor()->setCurrentParagraphType(TextParagraphType::PageHeading);
                            editor()->addParagraph(jumpForEnter(TextParagraphType::PageHeading));
                            isHandled = true;
                        }
                    }
                    //
                    // Вставляем блок и применяем ему стиль описания действия
                    //
                    if (!isHandled) {
                        editor()->addParagraph(jumpForEnter(TextParagraphType::PanelHeading));
                    }
                } else {
                    //! Внутри блока

                    //
                    // Вставляем блок и применяем ему стиль описания действия
                    //
                    editor()->addParagraph(jumpForEnter(TextParagraphType::PanelHeading));
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
                // Ни чего не делаем
                //
                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::PanelHeading));
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
                    editor()->addParagraph(jumpForTab(TextParagraphType::PanelHeading));
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

void PanelHandler::handleBackspace(QKeyEvent* _event)
{
    //
    // Блокируем отображение подсказки при удалении текущего блока
    //
    if (editor()->textCursor().positionInBlock() == 0 && !editor()->textCursor().hasSelection()) {
        m_completionAllowed = false;
    }

    StandardKeyHandler::handleBackspace(_event);

    m_completionAllowed = true;
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
    // ... текст блока
    QString currentBlockText = currentBlock.text();
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
        if (editor()->dictionaries()->singlePageIntros().contains(backwardTextCorrected,
                                                                  Qt::CaseInsensitive)
            || editor()->dictionaries()->multiplePageIntros().contains(backwardTextCorrected,
                                                                       Qt::CaseInsensitive)) {
            editor()->setCurrentParagraphType(TextParagraphType::PageHeading);
        }
    } else {
        //
        // Покажем подсказку, если это возможно
        //
        complete(currentBlockText, cursorBackwardText);
    }
}

void PanelHandler::handleInput(QInputMethodEvent*)
{
    handleOther();
}

void PanelHandler::complete(const QString& _currentBlockText, const QString& _cursorBackwardText)
{
    if (!m_completionAllowed) {
        return;
    }

    auto modelData = editor()->dictionaries()->singlePanelIntros();
    for (const auto& intro : reversed(editor()->dictionaries()->multiplePanelIntros())) {
        modelData.insert(1, intro);
    }
    m_completerModel->setStringList(modelData);

    //
    // Дополним текст
    //
    int cursorMovement = _currentBlockText.length();
    while (!_cursorBackwardText.endsWith(_currentBlockText.leftRef(cursorMovement),
                                         Qt::CaseInsensitive)) {
        --cursorMovement;
    }
    //
    // ... дополняем, когда цикл обработки событий выполнится, чтобы позиция курсора
    //     корректно определилась после изменения текста
    //
    QTimer::singleShot(0, editor(), [this, _currentBlockText, cursorMovement] {
        editor()->complete(m_completerModel, _currentBlockText, cursorMovement);
    });
}

} // namespace KeyProcessingLayer

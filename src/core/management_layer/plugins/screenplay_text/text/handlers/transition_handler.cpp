#include "transition_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/templates/screenplay_template.h>

#include <QKeyEvent>
#include <QStringListModel>
#include <QTextBlock>
#include <QTimer>

using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer {

TransitionHandler::TransitionHandler(Ui::ScreenplayTextEdit* _editor)
    : StandardKeyHandler(_editor)
    , m_completerModel(new QStringListModel(_editor))
{
}

void TransitionHandler::prehandle()
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

    complete(currentBlockText, cursorBackwardText);
}

void TransitionHandler::handleEnter(QKeyEvent* _event)
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
        // Если нужно автоматически перепрыгиваем к следующему блоку
        //
        if (_event != nullptr) { // ... чтобы таб не переводил на новую строку
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addParagraph(jumpForEnter(TextParagraphType::Transition));
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(TextParagraphType::Transition);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Сменить стиль
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::Transition));
            } else {
                //! Текст не пуст

                //
                // Сохраним переход
                //
                storeTransition();

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставим блок перехода перед собой
                    //
                    editor()->addParagraph(TextParagraphType::Transition);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставляем блок и применяем ему стиль время и место
                    //
                    editor()->addParagraph(jumpForEnter(TextParagraphType::Transition));
                } else {
                    //! Внутри блока

                    //
                    // Вставляем блок и применяем ему стиль время и место
                    //
                    editor()->addParagraph(TextParagraphType::SceneHeading);
                }
            }
        }
    }
}

void TransitionHandler::handleTab(QKeyEvent*)
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
        // Работаем, как ENTER
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
                // Сменить стиль
                //
                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::Transition));
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
                    // Сохраним переход
                    //
                    storeTransition();

                    //
                    // Вставить блок
                    //
                    editor()->addParagraph(jumpForTab(TextParagraphType::Transition));
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

void TransitionHandler::handleBackspace(QKeyEvent* _event)
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

void TransitionHandler::handleOther(QKeyEvent*)
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    const QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст блока
    const QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void TransitionHandler::handleInput(QInputMethodEvent* _event)
{
    Q_UNUSED(_event)

    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    const QTextCursor cursor = editor()->textCursor();
    int cursorPosition = cursor.positionInBlock();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursorPosition);

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void TransitionHandler::complete(const QString& _currentBlockText,
                                 const QString& _cursorBackwardText)
{
    Q_UNUSED(_cursorBackwardText)

    if (!m_completionAllowed) {
        return;
    }

    //
    // Дополним текст
    //
    m_completerModel->setStringList(editor()->dictionaries()->transitions().toList());
    //
    // ... дополняем, когда цикл обработки событий выполнится, чтобы позиция курсора
    //     корректно определилась после изменения текста
    //
    QTimer::singleShot(0, editor(), [this, _currentBlockText] {
        editor()->complete(m_completerModel, _currentBlockText, 0);
    });
}

void TransitionHandler::storeTransition() const
{
    //
    // Получим необходимые значения
    //
    // ... курсор в текущем положении
    const QTextCursor cursor = editor()->textCursor();
    // ... блок текста в котором находится курсор
    const QTextBlock currentBlock = cursor.block();
    // ... текст блока
    const QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... переход
    const QString transition = cursorBackwardText;

    //
    // Сохраняем персонажа
    //
    editor()->dictionaries()->addTransition(transition);
}

} // namespace KeyProcessingLayer

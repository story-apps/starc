#include "transition_handler.h"

#include "../ScenarioTextEdit.h"

#include <Domain/Transition.h>

#include <DataLayer/DataStorageLayer/StorageFacade.h>
#include <DataLayer/DataStorageLayer/TransitionStorage.h>

#include <QKeyEvent>
#include <QTextBlock>

using namespace KeyProcessingLayer;
using namespace DataStorageLayer;
using namespace BusinessLogic;
using UserInterface::ScenarioTextEdit;


TransitionHandler::TransitionHandler(ScenarioTextEdit* _editor) :
    StandardKeyHandler(_editor)
{
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
        if (_event != nullptr // ... чтобы таб не переводил на новую строку
            && autoJumpToNextBlock()) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::Transition));
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addScenarioBlock(ScenarioBlockStyle::Transition);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Сменить стиль
                //
                editor()->changeScenarioBlockType(changeForEnter(ScenarioBlockStyle::Transition));
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
                    editor()->addScenarioBlock(ScenarioBlockStyle::Transition);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставляем блок и применяем ему стиль время и место
                    //
                    editor()->addScenarioBlock(jumpForEnter(ScenarioBlockStyle::Transition));
                } else {
                    //! Внутри блока

                    //
                    // Вставляем блок и применяем ему стиль время и место
                    //
                    editor()->addScenarioBlock(ScenarioBlockStyle::SceneHeading);
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

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Сменить стиль
                //
                editor()->changeScenarioBlockType(changeForTab(ScenarioBlockStyle::Transition));
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
                    editor()->addScenarioBlock(jumpForTab(ScenarioBlockStyle::Transition));
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
#ifndef Q_OS_ANDROID
    Q_UNUSED(_event)
#endif
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
#ifdef Q_OS_ANDROID
    QString stringForInsert;
    if (!_event->preeditString().isEmpty()) {
        stringForInsert = _event->preeditString();
    } else {
        stringForInsert = _event->commitString();
    }
    currentBlockText.insert(cursorPosition, stringForInsert);
    cursorPosition += stringForInsert.length();
#endif
    // ... текст до курсора
    const QString cursorBackwardText = currentBlockText.left(cursorPosition);

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void TransitionHandler::complete(const QString& _currentBlockText, const QString& _cursorBackwardText)
{
    Q_UNUSED(_cursorBackwardText);

    //
    // Дополним текст
    //
    editor()->complete(StorageFacade::transitionStorage()->all(), _currentBlockText);
}

void TransitionHandler::storeTransition() const
{
    if (editor()->storeDataWhenEditing()) {
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
        StorageFacade::transitionStorage()->storeTransition(transition);
    }
}

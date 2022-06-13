#include "character_handler.h"

#include "../stageplay_text_edit.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_block_parser.h>
#include <business_layer/templates/stageplay_template.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QKeyEvent>
#include <QStringList>
#include <QStringListModel>
#include <QTextBlock>
#include <QTimer>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;
using Ui::StageplayTextEdit;


namespace {
/**
 * @brief Вставить двоеточие в конец блока
 */
void insertColunAtEnd(QTextCursor& _cursor)
{
    _cursor.movePosition(QTextCursor::EndOfBlock);
    if (!_cursor.block().text().trimmed().endsWith(':')) {
        while (!_cursor.block().text().isEmpty() && _cursor.block().text().endsWith(' ')) {
            _cursor.deletePreviousChar();
        }
        _cursor.insertText(":");
    }
}
} // namespace

namespace KeyProcessingLayer {

CharacterHandler::CharacterHandler(StageplayTextEdit* _editor)
    : StandardKeyHandler(_editor)
    , m_completerModel(new QStringListModel(_editor))
{
}

void CharacterHandler::prehandle()
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
void CharacterHandler::handleEnter(QKeyEvent* _event)
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
        if (_event != 0) { // ... чтобы таб не переводил на новую строку
            //
            // Добавим двоеточие после имени
            //
            insertColunAtEnd(cursor);

            //
            // Переходим в следующий блок
            //
            editor()->setTextCursor(cursor);

            //
            // Если диалоги располагаются в таблице
            //
            if (editor()->stageplayTemplate().placeDialoguesInTable()) {
                //
                // ... то переходим к следующему блоку, он уже отформатирован
                //
                editor()->moveCursor(QTextCursor::NextBlock);
            }
            //
            // В противном случае, добавляем новый абзац
            //
            else {
                editor()->addParagraph(jumpForEnter(TextParagraphType::Character));
            }
        }
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

                //
                // Cменить стиль
                //
                editor()->setCurrentParagraphType(changeForEnter(TextParagraphType::Character));
            } else {
                //! Текст не пуст

                if (!cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Сохраним имя персонажа
                    //
                    storeCharacter();

                    //
                    // Добавим двоеточие после имени
                    //
                    insertColunAtEnd(cursor);

                    //
                    // Если диалоги располагаются в таблице
                    //
                    if (editor()->stageplayTemplate().placeDialoguesInTable()) {
                        //
                        // ... то переходим к следующему блоку, он уже отформатирован
                        //
                        editor()->moveCursor(QTextCursor::NextBlock);
                    }
                    //
                    // В противном случае, добавляем новый абзац
                    //
                    else {
                        editor()->addParagraph(jumpForEnter(TextParagraphType::Character));
                    }
                } else {
                    //! В начале блока
                    //! Внутри блока

                    //
                    // Ничего не делаем
                    //
                }
            }
        }
    }
}

void CharacterHandler::handleTab(QKeyEvent*)
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
                // Cменить стиль на описание действия
                //
                editor()->setCurrentParagraphType(changeForTab(TextParagraphType::Character));
            } else {
                //! Текст не пуст

                if (!cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Сохраним имя персонажа
                    //
                    storeCharacter();

                    //
                    // Добавим двоеточие после имени
                    //
                    insertColunAtEnd(cursor);

                    //
                    // Если диалоги располагаются в таблице
                    //
                    if (editor()->stageplayTemplate().placeDialoguesInTable()) {
                        //
                        // ... то переходим к следующему блоку, он уже отформатирован
                        //
                        editor()->moveCursor(QTextCursor::NextBlock);
                    }
                    //
                    // В противном случае, добавляем новый абзац
                    //
                    else {
                        editor()->addParagraph(jumpForTab(TextParagraphType::Character));
                    }
                } else {
                    //! В начале блока
                    //! Внутри блока

                    //
                    // Ни чего не делаем
                    //
                }
            }
        }
    }
}

void CharacterHandler::handleBackspace(QKeyEvent* _event)
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

void CharacterHandler::handleOther(QKeyEvent*)
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

void CharacterHandler::handleInput(QInputMethodEvent*)
{
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

void CharacterHandler::complete(const QString& _currentBlockText,
                                const QString& _cursorBackwardText)
{
    if (!m_completionAllowed) {
        return;
    }

    if (_currentBlockText.isEmpty() && !editor()->showSuggestionsInEmptyBlocks()) {
        return;
    }

    //
    // Получим модель подсказок для текущей секции и выведем пользователю
    //
    QAbstractItemModel* sectionModel = 0;
    //
    // ... в соответствии со введённым в секции текстом
    //
    QString sectionText;

    QTextCursor cursor = editor()->textCursor();
    QStringList charactersToComplete;
    //
    // Определим персонажей сцены
    //
    cursor.movePosition(QTextCursor::PreviousBlock);
    while (!cursor.atStart()
           && TextBlockStyle::forBlock(cursor.block()) != TextParagraphType::SceneHeading) {
        if (TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::Character) {
            const QString characterName
                = BusinessLayer::StageplayCharacterParser::name(cursor.block().text());
            if (!characterName.isEmpty() && !charactersToComplete.contains(characterName)) {
                //
                // Персонажа, который говорил встречный диалог ставим выше,
                // т.к. высока вероятность того, что они общаются
                //
                if (charactersToComplete.size() == 1) {
                    charactersToComplete.prepend(characterName);
                }
                //
                // Остальных персонажей наполняем просто по очереди в тексте
                //
                else {
                    charactersToComplete.append(characterName);
                }
            }
        }
        cursor.movePosition(QTextCursor::PreviousBlock);
        cursor.movePosition(QTextCursor::StartOfBlock);
    }

    //
    // Все остальные персонажи
    //
    for (int characterRow = 0; characterRow < editor()->characters()->rowCount(); ++characterRow) {
        const auto characterIndex = editor()->characters()->index(characterRow, 0);
        const auto characterName
            = editor()->characters()->data(characterIndex, Qt::DisplayRole).toString();
        if (!charactersToComplete.contains(characterName)) {
            charactersToComplete.append(characterName);
        }
    }

    m_completerModel->setStringList(charactersToComplete);
    sectionModel = m_completerModel;
    sectionText = BusinessLayer::StageplayCharacterParser::name(_currentBlockText);

    //
    // Дополним текст
    //
    int cursorMovement = sectionText.length();
    while (!_cursorBackwardText.endsWith(sectionText.left(cursorMovement), Qt::CaseInsensitive)) {
        --cursorMovement;
    }
    //
    // ... дополняем, когда цикл обработки событий выполнится, чтобы позиция курсора
    //     корректно определилась после изменения текста
    //
    QMetaObject::invokeMethod(
        editor(),
        [this, sectionModel, sectionText, cursorMovement] {
            editor()->complete(sectionModel, sectionText, cursorMovement);
        },
        Qt::QueuedConnection);
}

void CharacterHandler::storeCharacter() const
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
    // ... имя персонажа
    const QString characterName
        = BusinessLayer::StageplayCharacterParser::name(cursorBackwardText.trimmed());

    //
    // Сохраняем персонажа
    //
    editor()->createCharacter(characterName);
}

} // namespace KeyProcessingLayer

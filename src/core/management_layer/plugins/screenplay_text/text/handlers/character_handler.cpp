#include "character_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/templates/screenplay_template.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QKeyEvent>
#include <QStringList>
#include <QStringListModel>
#include <QTextBlock>
#include <QTimer>

using BusinessLayer::TextBlockStyle;
using BusinessLayer::ScreenplayCharacterParser;
using BusinessLayer::TextParagraphType;
using BusinessLayer::ScreenplaySceneCharactersParser;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer {

CharacterHandler::CharacterHandler(ScreenplayTextEdit* _editor)
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
    // ... текущая секция
    ScreenplayCharacterParser::Section currentSection
        = ScreenplayCharacterParser::section(cursorBackwardText);


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
        // Дописать необходимые символы
        //
        switch (currentSection) {
        case ScreenplayCharacterParser::SectionExtension: {
            cursor.insertText(")");
            break;
        }

        default: {
            break;
        }
        }

        //
        // Если нужно автоматически перепрыгиваем к следующему блоку
        //
        if (_event != 0 // ... чтобы таб не переводил на новую строку
            && currentSection == ScreenplayCharacterParser::SectionName) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addParagraph(jumpForEnter(TextParagraphType::Character));
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(TextParagraphType::Character);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Cменить стиль
                //
                editor()->setCurrentParagraphType(
                    changeForEnter(TextParagraphType::Character));
            } else {
                //! Текст не пуст

                //
                // Сохраним имя персонажа
                //
                storeCharacter();

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставим блок имени героя перед собой
                    //
                    editor()->addParagraph(TextParagraphType::Character);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставить блок реплики героя
                    //
                    editor()->addParagraph(jumpForEnter(TextParagraphType::Character));
                } else {
                    //! Внутри блока

                    //
                    // Вставить блок реплики героя
                    //
                    editor()->addParagraph(TextParagraphType::Dialogue);
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

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Сохраним имя персонажа
                    //
                    storeCharacter();

                    //
                    // Вставить блок ремарки
                    //
                    editor()->addParagraph(jumpForTab(TextParagraphType::Character));
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

    //
    // Получим модель подсказок для текущей секции и выведем пользователю
    //
    QAbstractItemModel* sectionModel = 0;
    //
    // ... в соответствии со введённым в секции текстом
    //
    QString sectionText;

    QTextCursor cursor = editor()->textCursor();
    switch (ScreenplayCharacterParser::section(_cursorBackwardText)) {
    case ScreenplayCharacterParser::SectionName: {
        QStringList charactersToComplete;
        //
        // Определим персонажей сцены
        //
        cursor.movePosition(QTextCursor::PreviousBlock);
        while (!cursor.atStart()
               && TextBlockStyle::forBlock(cursor.block())
                   != TextParagraphType::SceneHeading) {
            if (TextBlockStyle::forBlock(cursor.block())
                == TextParagraphType::Character) {
                const QString characterName
                    = ScreenplayCharacterParser::name(cursor.block().text());
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
            } else if (TextBlockStyle::forBlock(cursor.block())
                       == TextParagraphType::SceneCharacters) {
                const QStringList characters
                    = ScreenplaySceneCharactersParser::characters(cursor.block().text());
                for (const QString& characterName : characters) {
                    if (!charactersToComplete.contains(characterName)) {
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
        for (int characterRow = 0; characterRow < editor()->characters()->rowCount();
             ++characterRow) {
            const auto characterIndex = editor()->characters()->index(characterRow, 0);
            const auto characterName
                = editor()->characters()->data(characterIndex, Qt::DisplayRole).toString();
            if (!charactersToComplete.contains(characterName)) {
                charactersToComplete.append(characterName);
            }
        }

        m_completerModel->setStringList(charactersToComplete);
        sectionModel = m_completerModel;
        sectionText = ScreenplayCharacterParser::name(_currentBlockText);
        break;
    }

    case ScreenplayCharacterParser::SectionExtension: {
        m_completerModel->setStringList(editor()->dictionaries()->characterExtensions().toList());
        sectionModel = m_completerModel;
        sectionText = ScreenplayCharacterParser::extension(_currentBlockText);
        break;
    }

    default: {
        break;
    }
    }

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
    const QString characterName = ScreenplayCharacterParser::name(cursorBackwardText);
    // ... состояние персонажа
    const QString characterExtension = ScreenplayCharacterParser::extension(cursorBackwardText);

    //
    // Сохраняем персонажа
    //
    editor()->createCharacter(characterName);
    editor()->dictionaries()->addCharacterExtension(characterExtension);
}

} // namespace KeyProcessingLayer

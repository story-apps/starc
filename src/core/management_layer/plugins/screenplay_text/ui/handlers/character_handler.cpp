#include "character_handler.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/templates/screenplay_template.h>

#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/document_object.h>

#include <ui/screenplay_text_edit.h>

#include <QKeyEvent>
#include <QStringListModel>
#include <QStringList>
#include <QTextBlock>

using BusinessLayer::CharacterParser;
using BusinessLayer::SceneCharactersParser;
using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer
{

CharacterHandler::CharacterHandler(ScreenplayTextEdit* _editor)
    : StandardKeyHandler(_editor),
      m_completerModel(new QStringListModel(_editor))
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

    //
    // Пробуем определить кто сейчас должен говорить
    //
    if (currentBlockText.isEmpty()) {
        QString previousCharacter, character;

        //
        // ... для этого ищем предпоследнего персонажа сцены
        //
        cursor.movePosition(QTextCursor::PreviousBlock);
        while (!cursor.atStart()
               && ScreenplayBlockStyle::forBlock(cursor.block()) != ScreenplayParagraphType::SceneHeading) {
            if (ScreenplayBlockStyle::forBlock(cursor.block()) == ScreenplayParagraphType::Character) {
                //
                // Нашли предыдущего персонажа
                //
                if (previousCharacter.isEmpty()) {
                    previousCharacter = CharacterParser::name(cursor.block().text());
                }
                //
                // Нашли потенциального говорящего
                //
                else {
                    //
                    // Выберем его в списке вариантов
                    //
                    character = CharacterParser::name(cursor.block().text());
                    if (character != previousCharacter) {
                        break;
                    }
                }
            }

            cursor.movePosition(QTextCursor::PreviousBlock);
            cursor.movePosition(QTextCursor::StartOfBlock);
        }

        //
        // Показываем всплывающую подсказку
        //
        QAbstractItemModel* model = 0;
        if (!character.isEmpty()) {
            m_completerModel->setStringList(QStringList() << character);
            model = m_completerModel;
        } else if (!previousCharacter.isEmpty()) {
            m_completerModel->setStringList(QStringList() << previousCharacter);
            model = m_completerModel;
        } else {
            model = editor()->characters();
        }
        editor()->complete(model, QString());
    }
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
    CharacterParser::Section currentSection = CharacterParser::section(cursorBackwardText);


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
            case CharacterParser::SectionState: {
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
            && autoJumpToNextBlock()
            && currentSection == CharacterParser::SectionName) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            editor()->setTextCursor(cursor);
            editor()->addScenarioBlock(jumpForEnter(ScreenplayParagraphType::Character));
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addScenarioBlock(ScreenplayParagraphType::Character);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Cменить стиль
                //
                editor()->changeScenarioBlockType(changeForEnter(ScreenplayParagraphType::Character));
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
                    editor()->addScenarioBlock(ScreenplayParagraphType::Character);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Вставить блок реплики героя
                    //
                    editor()->addScenarioBlock(jumpForEnter(ScreenplayParagraphType::Character));
                } else {
                    //! Внутри блока

                    //
                    // Вставить блок реплики героя
                    //
                    editor()->addScenarioBlock(ScreenplayParagraphType::Dialogue);
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

            if (cursorBackwardText.isEmpty()
                && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Cменить стиль на описание действия
                //
                editor()->changeScenarioBlockType(changeForTab(ScreenplayParagraphType::Character));
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
                    editor()->addScenarioBlock(jumpForTab(ScreenplayParagraphType::Character));
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

void CharacterHandler::complete(const QString& _currentBlockText, const QString& _cursorBackwardText)
{
    //
    // Текущая секция
    //
    CharacterParser::Section currentSection = CharacterParser::section(_cursorBackwardText);

    //
    // Получим модель подсказок для текущей секции и выведем пользователю
    //
    QAbstractItemModel* sectionModel = 0;
    //
    // ... в соответствии со введённым в секции текстом
    //
    QString sectionText;

    QTextCursor cursor = editor()->textCursor();
    switch (currentSection) {
        case CharacterParser::SectionName: {
            QStringList sceneCharacters;
            //
            // Когда введён один символ имени пробуем оптимизировать поиск персонажей из текущей сцены
            //
            if (_cursorBackwardText.length() < 2) {
                cursor.movePosition(QTextCursor::PreviousBlock);
                while (!cursor.atStart()
                       && ScreenplayBlockStyle::forBlock(cursor.block()) != ScreenplayParagraphType::SceneHeading) {
                    if (ScreenplayBlockStyle::forBlock(cursor.block()) == ScreenplayParagraphType::Character) {
                        const QString characterName = CharacterParser::name(cursor.block().text());
                        if (characterName.startsWith(_cursorBackwardText, Qt::CaseInsensitive)
                            && !sceneCharacters.contains(characterName)) {
                            sceneCharacters.append(characterName);
                        }
                    } else if (ScreenplayBlockStyle::forBlock(cursor.block()) == ScreenplayParagraphType::SceneCharacters) {
                        const QStringList characters = SceneCharactersParser::characters(cursor.block().text());
                        foreach (const QString& characterName, characters) {
                            if (characterName.startsWith(_cursorBackwardText, Qt::CaseInsensitive)
                                && !sceneCharacters.contains(characterName)) {
                                sceneCharacters.append(characterName);
                            }
                        }
                    }
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    cursor.movePosition(QTextCursor::StartOfBlock);
                }
            }

            //
            // По возможности используем список персонажей сцены
            //
            if (!sceneCharacters.isEmpty()) {
                m_completerModel->setStringList(sceneCharacters);
                sectionModel = m_completerModel;
            } else {
                sectionModel = editor()->characters();
            }
            sectionText = CharacterParser::name(_currentBlockText);
            break;
        }

        case CharacterParser::SectionState: {
            m_completerModel->setStringList(
                QStringList::fromVector(editor()->dictionaries()->characterExtensions()));
            sectionModel = m_completerModel;
            sectionText = CharacterParser::extension(_currentBlockText);
            break;
        }

        default: {
            break;
        }
    }

    //
    // Дополним текст
    //
    editor()->complete(sectionModel, sectionText);
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
    const QString characterName = CharacterParser::name(cursorBackwardText);
    // ... состояние персонажа
    const QString characterExtension = CharacterParser::extension(cursorBackwardText);

    //
    // Сохраняем персонажа
    //
    editor()->characters()->createCharacter(characterName);
    editor()->dictionaries()->addCharacterExtension(characterExtension);
}

} // namespace KeyProcessingLayer

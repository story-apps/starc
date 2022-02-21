#include "character_handler.h"

#include "../comic_book_text_edit.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/comic_book/comic_book_dictionaries_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_block_parser.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/templates/comic_book_template.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QKeyEvent>
#include <QStringList>
#include <QStringListModel>
#include <QTextBlock>
#include <QTimer>

using BusinessLayer::ComicBookBlockStyle;
using BusinessLayer::ComicBookCharacterParser;
using BusinessLayer::ComicBookParagraphType;
using Ui::ComicBookTextEdit;


namespace KeyProcessingLayer {

CharacterHandler::CharacterHandler(ComicBookTextEdit* _editor)
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
    ComicBookCharacterParser::Section currentSection
        = ComicBookCharacterParser::section(cursorBackwardText);


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
        case ComicBookCharacterParser::SectionState: {
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
            && currentSection == ComicBookCharacterParser::SectionName) {
            cursor.movePosition(QTextCursor::EndOfBlock);
            if (!cursor.block().text().trimmed().endsWith(":")) {
                cursor.insertText(":");
            }
            editor()->setTextCursor(cursor);
            editor()->moveCursor(QTextCursor::NextBlock);
        }
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(ComicBookParagraphType::Character);
        } else {
            //! Нет выделения

            if (cursorBackwardText.isEmpty() && cursorForwardText.isEmpty()) {
                //! Текст пуст

                //
                // Cменить стиль
                //
                editor()->setCurrentParagraphType(
                    changeForEnter(ComicBookParagraphType::Character));
            } else {
                //! Текст не пуст

                //
                // Сохраним имя персонажа
                //
                storeCharacter();
                //
                // ...  добавим двоеточие после имени
                //
                cursor.movePosition(QTextCursor::EndOfBlock);
                if (!cursor.block().text().trimmed().endsWith(":")) {
                    cursor.insertText(":");
                }

                if (cursorBackwardText.isEmpty()) {
                    //! В начале блока

                    //
                    // Вставим блок имени героя перед собой
                    //
                    editor()->addParagraph(ComicBookParagraphType::Character);
                } else if (cursorForwardText.isEmpty()) {
                    //! В конце блока

                    //
                    // Переходим к следующему блоку, он уже отформатирован должным образом
                    //
                    editor()->moveCursor(QTextCursor::NextBlock);
                } else {
                    //! Внутри блока

                    //
                    // Вставить блок реплики героя
                    //
                    editor()->addParagraph(ComicBookParagraphType::Dialogue);
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
                editor()->setCurrentParagraphType(changeForTab(ComicBookParagraphType::Character));
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
                    // ...  добавим двоеточие после имени
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock);
                    if (!cursor.block().text().trimmed().endsWith(":")) {
                        cursor.insertText(":");
                    }

                    //
                    // Переходим к следующему блоку
                    //
                    editor()->moveCursor(QTextCursor::NextBlock);
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

void CharacterHandler::handleOther(QKeyEvent* _event)
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
    // На двоеточии заканчивается ввод имени персонажа
    //
    if (cursorBackwardText.endsWith(':') && _event->text() == ":") {
        storeCharacter();
        editor()->moveCursor(QTextCursor::NextBlock);
    }

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
    switch (ComicBookCharacterParser::section(_cursorBackwardText)) {
    case ComicBookCharacterParser::SectionName: {
        QStringList charactersToComplete;
        //
        // Определим персонажей сцены
        //
        cursor.movePosition(QTextCursor::PreviousBlock);
        while (!cursor.atStart()
               && ComicBookBlockStyle::forBlock(cursor.block()) != ComicBookParagraphType::Page) {
            if (ComicBookBlockStyle::forBlock(cursor.block())
                == ComicBookParagraphType::Character) {
                const QString characterName = ComicBookCharacterParser::name(cursor.block().text());
                if (!characterName.isEmpty() && !charactersToComplete.contains(characterName)
                    && !editor()->dictionaries()->commonCharacters().contains(characterName)) {
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
        for (int characterRow = 0; characterRow < editor()->characters()->rowCount();
             ++characterRow) {
            const auto characterIndex = editor()->characters()->index(characterRow, 0);
            const auto characterName
                = editor()->characters()->data(characterIndex, Qt::DisplayRole).toString();
            if (!charactersToComplete.contains(characterName)) {
                charactersToComplete.append(characterName);
            }
        }

        //
        // Специфичные для комикса элементы
        //
        charactersToComplete.append(editor()->dictionaries()->commonCharacters().toList());

        m_completerModel->setStringList(charactersToComplete);
        sectionModel = m_completerModel;
        sectionText = ComicBookCharacterParser::name(_currentBlockText);
        break;
    }

    case ComicBookCharacterParser::SectionState: {
        m_completerModel->setStringList(editor()->dictionaries()->characterExtensions().toList());
        sectionModel = m_completerModel;
        sectionText = ComicBookCharacterParser::extension(_currentBlockText);
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
    QTimer::singleShot(0, [this, sectionModel, sectionText, cursorMovement] {
        editor()->complete(sectionModel, sectionText, cursorMovement);
    });
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
    const QString characterName = ComicBookCharacterParser::name(cursorBackwardText);
    // ... состояние персонажа
    const QString characterExtension = ComicBookCharacterParser::extension(cursorBackwardText);

    //
    // Сохраняем персонажа
    //
    if (!editor()->dictionaries()->commonCharacters().contains(characterName)) {
        editor()->characters()->createCharacter(characterName);
    }
    editor()->dictionaries()->addCharacterExtension(characterExtension);
}

} // namespace KeyProcessingLayer

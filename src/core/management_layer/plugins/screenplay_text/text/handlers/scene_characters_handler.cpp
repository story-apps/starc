#include "scene_characters_handler.h"

#include "../screenplay_text_edit.h"

#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QRegularExpression>
#include <QStringListModel>
#include <QTextBlock>
#include <QTimer>

using BusinessLayer::TextParagraphType;
using Ui::ScreenplayTextEdit;


namespace KeyProcessingLayer {

SceneCharactersHandler::SceneCharactersHandler(ScreenplayTextEdit* _editor)
    : StandardKeyHandler(_editor)
    , m_filteredCharactersModel(new QStringListModel(_editor))
{
}

void SceneCharactersHandler::handleEnter(QKeyEvent*)
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
    // ... префикс и постфикс стиля
    const auto& style
        = editor()->screenplayTemplate().paragraphStyle(TextParagraphType::SceneCharacters);
    QString stylePrefix = style.prefix();
    QString stylePostfix = style.postfix();

    //
    // Обработка
    //
    if (editor()->isCompleterVisible()) {
        //! Если открыт подстановщик

        //
        // Вставить выбранный вариант
        //
        editor()->applyCompletion();
    } else {
        //! Подстановщик закрыт

        if (cursor.hasSelection()) {
            //! Есть выделение

            //
            // Удаляем всё, но оставляем стилем блока текущий
            //
            editor()->addParagraph(TextParagraphType::SceneCharacters);
        } else {
            //! Нет выделения

            if ((cursorBackwardText.isEmpty() && cursorForwardText.isEmpty())
                || (cursorBackwardText + cursorForwardText == stylePrefix + stylePostfix)) {
                //! Текст пуст

                //
                // Cменить стиль на описание действия
                //
                editor()->setCurrentParagraphType(
                    changeForEnter(TextParagraphType::SceneCharacters));
            } else {
                //! Текст не пуст

                //
                // Сохраним персонажей
                //
                storeCharacters();

                if (cursorBackwardText.isEmpty() || cursorBackwardText == stylePrefix) {
                    //! В начале блока

                    //
                    // Ни чего не делаем
                    //
                } else if (cursorForwardText.isEmpty() || cursorForwardText == stylePostfix) {
                    //! В конце блока

                    //
                    // Перейдём к блоку действия
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock);
                    editor()->setTextCursor(cursor);
                    editor()->addParagraph(jumpForEnter(TextParagraphType::SceneCharacters));
                } else {
                    //! Внутри блока

                    //
                    // Переместим обрамление в правильное место
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock);
                    if (cursorForwardText.endsWith(stylePostfix)) {
                        for (int deleteReplays = stylePostfix.length(); deleteReplays > 0;
                             --deleteReplays) {
                            cursor.deletePreviousChar();
                        }
                    }
                    cursor = editor()->textCursor();
                    cursor.insertText(stylePostfix);

                    //
                    // Перейдём к блоку действия
                    //
                    editor()->setTextCursor(cursor);
                    editor()->addParagraph(TextParagraphType::Action);
                }
            }
        }
    }
}

void SceneCharactersHandler::handleTab(QKeyEvent* _event)
{
    handleEnter(_event);
}

void SceneCharactersHandler::handleBackspace(QKeyEvent* _event)
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

void SceneCharactersHandler::handleOther(QKeyEvent*)
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

    //
    // Покажем подсказку, если это возможно
    //
    complete(currentBlockText, cursorBackwardText);
}

void SceneCharactersHandler::handleInput(QInputMethodEvent* _event)
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

void SceneCharactersHandler::complete(const QString& _currentBlockText,
                                      const QString& _cursorBackwardText)
{
    if (!m_completionAllowed) {
        return;
    }

    QString cursorBackwardTextToComma = _cursorBackwardText;
    if (!cursorBackwardTextToComma.split(", ").isEmpty()) {
        cursorBackwardTextToComma = cursorBackwardTextToComma.split(", ").last();
    }
    // ... уберём префикс
    const auto& style
        = editor()->screenplayTemplate().paragraphStyle(TextParagraphType::SceneCharacters);
    QString stylePrefix = style.prefix();
    if (!stylePrefix.isEmpty() && cursorBackwardTextToComma.startsWith(stylePrefix)) {
        cursorBackwardTextToComma.remove(
            QRegularExpression(QString("^[%1]").arg(TextHelper::toRxEscaped(stylePrefix))));
    }

    //
    // Получим модель подсказок для текущей секции и выведем пользователю
    //
    QAbstractItemModel* characterModel = editor()->characters();

    //
    // Убрать из модели уже использованные элементы
    //
    // ... сформируем список уже введённых персонажей
    //
    QStringList enteredCharacters = TextHelper::smartToUpper(_currentBlockText).split(", ");
    enteredCharacters.removeOne(cursorBackwardTextToComma);
    //
    // ... скорректируем модель
    //
    QStringList filteredCharacters;
    for (int row = 0; row < characterModel->rowCount(); ++row) {
        const QString characterName = TextHelper::smartToUpper(
            characterModel->data(characterModel->index(row, 0)).toString());
        if (!enteredCharacters.contains(characterName)) {
            filteredCharacters.append(characterName);
        }
    }
    m_filteredCharactersModel->setStringList(filteredCharacters);

    //
    // Дополним текст
    //
    int cursorMovement = cursorBackwardTextToComma.length();
    while (!_cursorBackwardText.endsWith(cursorBackwardTextToComma.left(cursorMovement),
                                         Qt::CaseInsensitive)) {
        --cursorMovement;
    }
    //
    // ... дополняем, когда цикл обработки событий выполнится, чтобы позиция курсора
    //     корректно определилась после изменения текста
    //
    QTimer::singleShot(0, editor(), [this, cursorBackwardTextToComma, cursorMovement] {
        editor()->complete(m_filteredCharactersModel, cursorBackwardTextToComma, cursorMovement);
    });
}

void SceneCharactersHandler::storeCharacters() const
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
    // ... персонажы
    QStringList enteredCharacters
        = BusinessLayer::ScreenplaySceneCharactersParser::characters(currentBlockText);

    for (const QString& character : enteredCharacters) {
        editor()->createCharacter(character);
    }
}

} // namespace KeyProcessingLayer

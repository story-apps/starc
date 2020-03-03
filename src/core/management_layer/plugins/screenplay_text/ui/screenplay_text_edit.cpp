#include "screenplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"
#include "screenplay_text_cursor.h"
#include "screenplay_text_document.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <utils/helpers/text_helper.h>

#include <QScrollBar>

using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using BusinessLayer::ScreenplayTemplateFacade;

namespace Ui
{

class ScreenplayTextEdit::Implementation
{
public:
    BusinessLayer::ScreenplayTextModel* model = nullptr;
    ScreenplayTextDocument document;
};


// ****


ScreenplayTextEdit::ScreenplayTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent),
      d(new Implementation)
{
    setDocument(&d->document);
}

ScreenplayTextEdit::~ScreenplayTextEdit() = default;

void ScreenplayTextEdit::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->model = _model;
    d->document.setModel(_model);
}

BusinessLayer::ScreenplayDictionariesModel* ScreenplayTextEdit::dictionaries() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->dictionariesModel();
}

BusinessLayer::CharactersModel* ScreenplayTextEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersModel();
}

BusinessLayer::LocationsModel* ScreenplayTextEdit::locations() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->locationsModel();
}

void ScreenplayTextEdit::addParagraph(BusinessLayer::ScreenplayParagraphType _type)
{
    ScreenplayTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Вставим блок
    //
    cursor.insertBlock();
    setTextCursorReimpl(cursor);

    //
    // Применим стиль к новому блоку
    //
    applyParagraphType(_type);

    //
    // Уведомим о том, что стиль сменился
    //
    emit paragraphTypeChanged();

    cursor.endEditBlock();
}

void ScreenplayTextEdit::setCurrentParagraphType(BusinessLayer::ScreenplayParagraphType _type)
{
    ScreenplayTextCursor cursor = textCursor();
    const auto currentParagraphType = ScreenplayBlockStyle::forBlock(cursor.block());
    if (currentParagraphType == _type) {
        return;
    }

    //
    // Нельзя сменить стиль конечных элементов папок
    //
    if (currentParagraphType == ScreenplayParagraphType::FolderFooter) {
        return;
    }


    cursor.beginEditBlock();

    //
    // Первым делом очищаем пользовательские данные
    //
    cursor.block().setUserData(nullptr);

    //
    // Закроем подсказку
    //
    closeCompleter();

    //
    // Обработаем предшествующий установленный стиль
    //
    cleanParagraphType();

    //
    // Применим новый стиль к блоку
    //
    applyParagraphType(_type);

    cursor.endEditBlock();


    emit paragraphTypeChanged();
}

BusinessLayer::ScreenplayParagraphType ScreenplayTextEdit::currentParagraphType() const
{
    return ScreenplayBlockStyle::forBlock(textCursor().block());
}

void ScreenplayTextEdit::setTextCursorReimpl(const QTextCursor& _cursor)
{
    //
    // TODO: пояснить зачем это необходимо делать?
    //
    const int verticalScrollValue = verticalScrollBar()->value();
    setTextCursor(_cursor);
    verticalScrollBar()->setValue(verticalScrollValue);
}

void ScreenplayTextEdit::keyPressEvent(QKeyEvent* _event)
{
    if (isReadOnly()) {
        return;
    }

    //
    // Отмену и повтор последнего действия, делаем без последующей обработки
    //
    // Если так не делать, то это приведёт к вставке странных символов, которые непонятно откуда берутся :(
    // Например:
    // 1. после реплики идёт время и место
    // 2. вставляем после реплики описание действия
    // 3. отменяем последнее действие
    // 4. в последующем времени и месте появляется символ "кружочек со стрелочкой"
    //
    // FIXME: разобраться
    //
    if (_event == QKeySequence::Undo
        || _event == QKeySequence::Redo) {
        if (_event == QKeySequence::Undo) {
            emit undoRequest();
        }
        else if (_event == QKeySequence::Redo) {
            emit redoRequest();
        }
        _event->accept();
        return;
    }

    //
    // Получим обработчик
    //
    auto handler = KeyProcessingLayer::KeyPressHandlerFacade::instance(this);

    //
    // Получим курсор в текущем положении
    // Начнём блок операций
    //
    QTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    //
    // Подготовка к обработке
    //
    handler->prepare(_event);

    //
    // Предварительная обработка
    //
    handler->prepareForHandle(_event);

    //
    // Отправить событие в базовый класс
    //
    if (handler->needSendEventToBaseClass()) {
        if (!keyPressEventReimpl(_event)) {
            BaseTextEdit::keyPressEvent(_event);
        }

        updateEnteredText(_event->text());
    }

    //
    // Обработка
    //
    handler->handle(_event);

    //
    // Событие дошло по назначению
    //
    _event->accept();

    //
    // Завершим блок операций
    //
    cursor.endEditBlock();

    //
    // Убедимся, что курсор виден
    //
    if (handler->needEnsureCursorVisible()) {
        ensureCursorVisible();
    }

    //
    // Подготовим следующий блок к обработке
    //
    if (handler->needPrehandle()) {
        handler->prehandle();
    }
}

bool ScreenplayTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    bool isEventHandled = true;

    //
    // Переопределяем
    //
    // ... перевод курсора к следующему символу
    //
    if (_event == QKeySequence::MoveToNextChar) {
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            moveCursor(QTextCursor::NextCharacter);
        } else {
            moveCursor(QTextCursor::PreviousCharacter);
        }

        while (!textCursor().atEnd()
               && (!textCursor().block().isVisible()
                   || textCursor().blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection))) {
            moveCursor(QTextCursor::NextBlock);
        }
    }
    //
    // ... перевод курсора к предыдущему символу
    //
    else if (_event == QKeySequence::MoveToPreviousChar) {
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            moveCursor(QTextCursor::PreviousCharacter);
        } else {
            moveCursor(QTextCursor::NextCharacter);
        }
        while (!textCursor().atStart()
               && (!textCursor().block().isVisible()
                   || textCursor().blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection))) {
            moveCursor(QTextCursor::StartOfBlock);
            if (textCursor().block().textDirection() == Qt::LeftToRight) {
                moveCursor(QTextCursor::PreviousCharacter);
            } else {
                moveCursor(QTextCursor::NextCharacter);
            }
        }
    }
    //
    // Обрабатываем в базовом классе
    //
    else {
        isEventHandled = BaseTextEdit::keyPressEventReimpl(_event);
    }

    return isEventHandled;
}

bool ScreenplayTextEdit::updateEnteredText(const QString& _eventText)
{
    if (!capitalizeWords()) {
        return false;
    }

    if (_eventText.isEmpty()) {
        return false;
    }

    //
    // Получим значения
    //
    // ... курсора
    QTextCursor cursor = textCursor();
    // ... блок текста в котором находится курсор
    QTextBlock currentBlock = cursor.block();
    // ... текст блока
    QString currentBlockText = currentBlock.text();
    // ... текст до курсора
    QString cursorBackwardText = currentBlockText.left(cursor.positionInBlock());
    // ... текст после курсора
    QString cursorForwardText = currentBlockText.mid(cursor.positionInBlock());
    // ... стиль шрифта блока
    QTextCharFormat currentCharFormat = currentBlock.charFormat();

    //
    // Определяем необходимость установки верхнего регистра для первого символа блока
    //
    if (currentCharFormat.boolProperty(ScreenplayBlockStyle::PropertyIsFirstUppercase)
        && cursorBackwardText != " "
        && (cursorBackwardText == _eventText
            || cursorBackwardText == (currentCharFormat.stringProperty(ScreenplayBlockStyle::PropertyPrefix)
                                      + _eventText))
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сформируем правильное представление строки
        //
        QString correctedText = _eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        for (int repeats = 0; repeats < _eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursor(cursor);

        return true;
    }

    return BaseTextEdit::updateEnteredText(_eventText);
}

void ScreenplayTextEdit::cleanParagraphType()
{
    ScreenplayTextCursor cursor = textCursor();
    const auto oldBlockStyle
            = ScreenplayTemplateFacade::getTemplate().blockStyle(
                  ScreenplayBlockStyle::forBlock(cursor.block()));

    //
    // Удалить завершающий блок папки
    //
    if (oldBlockStyle.isEmbeddableHeader()) {
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::NextBlock);

        // ... открытые группы на пути поиска необходимого для обновления блока
        int openedGroups = 0;
        bool isFooterUpdated = false;
        do {
            const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
            if (currentType == oldBlockStyle.embeddableFooter()) {
                if (openedGroups == 0) {
                    //
                    // Запомним стиль предыдущего блока
                    //
                    cursor.movePosition(QTextCursor::PreviousBlock);
                    const auto previousBlockType = ScreenplayBlockStyle::forBlock(cursor.block());
                    cursor.movePosition(QTextCursor::NextBlock);
                    //
                    // Удаляем закрывающий блок
                    //
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.deleteChar();
                    cursor.deletePreviousChar();
                    //
                    // Восстановим стиль предыдущего блока
                    //
                    if (ScreenplayBlockStyle::forBlock(cursor.block()) != previousBlockType) {
                        QTextCursor lastTextCursor = textCursor();
                        setTextCursor(cursor);
                        applyParagraphType(previousBlockType);
                        setTextCursor(lastTextCursor);
                    }
                    isFooterUpdated = true;
                } else {
                    --openedGroups;
                }
            } else if (currentType == oldBlockStyle.type()) {
                //
                // Встретилась новая папка
                //
                ++openedGroups;
            }
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);
        } while (!isFooterUpdated
                 && !cursor.atEnd());
    }
}

void ScreenplayTextEdit::applyParagraphType(BusinessLayer::ScreenplayParagraphType _type)
{
    ScreenplayTextCursor cursor = textCursor();
    cursor.beginEditBlock();

    const auto newBlockStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(_type);

    //
    // Обновим стили
    //
    cursor.setBlockCharFormat(newBlockStyle.charFormat());
    cursor.setBlockFormat(newBlockStyle.blockFormat());

    //
    // Применим стиль текста ко всему блоку, выделив его,
    // т.к. в блоке могут находиться фрагменты в другом стиле
    // + сохраняем форматирование выделений
    //
    {
        cursor.movePosition(QTextCursor::StartOfBlock);

        //
        // Если в блоке есть выделения, обновляем цвет только тех частей, которые не входят в выделения
        //
        QTextBlock currentBlock = cursor.block();
        if (!currentBlock.textFormats().isEmpty()) {
            const auto formats = currentBlock.textFormats();
            for (const auto& range : formats) {
                if (range.format.boolProperty(ScreenplayBlockStyle::PropertyIsReviewMark)) {
                    continue;
                }
                cursor.setPosition(currentBlock.position() + range.start);
                cursor.setPosition(cursor.position() + range.length, QTextCursor::KeepAnchor);
                cursor.mergeCharFormat(newBlockStyle.charFormat());
            }
        }
        //
        // Если выделений нет, обновляем блок целиком
        //
        else {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(newBlockStyle.charFormat());
        }
    }

    //
    // Для заголовка папки нужно создать завершение, захватив всё содержимое сцены
    //
    if (newBlockStyle.isEmbeddableHeader()) {
        const auto footerStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(newBlockStyle.embeddableFooter());

        //
        // Запомним позицию курсора
        //
        const int lastCursorPosition = textCursor().position();

        //
        // Ищем конец сцены
        //
        do {
            cursor.movePosition(QTextCursor::EndOfBlock);
            cursor.movePosition(QTextCursor::NextBlock);
        } while (!cursor.atEnd()
                 && ScreenplayBlockStyle::forBlock(cursor.block()) != ScreenplayParagraphType::SceneHeading
                 && ScreenplayBlockStyle::forBlock(cursor.block()) != ScreenplayParagraphType::FolderHeader
                 && ScreenplayBlockStyle::forBlock(cursor.block()) != ScreenplayParagraphType::FolderFooter);

        //
        // Если забежали на блок следующей сцены, вернёмся на один символ назад
        //
        if (!cursor.atEnd() && cursor.atBlockStart()) {
            cursor.movePosition(QTextCursor::PreviousCharacter);
        }

        //
        // Когда дошли до конца сцены, вставляем закрывающий блок
        //
        cursor.insertBlock();
        cursor.setBlockCharFormat(footerStyle.charFormat());
        cursor.setBlockFormat(footerStyle.blockFormat());

        //
        // т.к. вставлен блок, нужно вернуть курсор на место
        //
        cursor.setPosition(lastCursorPosition);
        setTextCursor(cursor);

        //
        // Эмулируем нажатие кнопки клавиатуры, чтобы обновился футер стиля
        //
        QKeyEvent empyEvent(QEvent::KeyPress, -1, Qt::NoModifier);
        keyPressEvent(&empyEvent);
    }

    cursor.endEditBlock();
}

} // namespace Ui

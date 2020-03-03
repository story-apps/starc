#include "screenplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"
#include "screenplay_text_cursor.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
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
};


// ****


ScreenplayTextEdit::ScreenplayTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent),
      d(new Implementation)
{
}

ScreenplayTextEdit::~ScreenplayTextEdit() = default;

void ScreenplayTextEdit::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    QSignalBlocker signalBlocker(this);

    d->model = _model;
    document()->clear();

    if (d->model == nullptr) {
        return;
    }

    //
    // Начинаем операцию вставки
    //
    ScreenplayTextCursor cursor(document());
    cursor.beginEditBlock();

    //
    // Последовательно формируем текст документа
    //
    bool isFirstParagraph = true;
    std::function<void(const QModelIndex&)> readDocumentFromModel;
    readDocumentFromModel = [model = d->model, &cursor, &isFirstParagraph, &readDocumentFromModel] (const QModelIndex& _parent) {
        for (int itemRow = 0; itemRow < model->rowCount(_parent); ++itemRow) {
            const auto itemIndex = model->index(itemRow, 0, _parent);
            const auto item = model->itemForIndex(itemIndex);
            switch (item->type()) {
                case BusinessLayer::ScreenplayTextModelItemType::Scene: {
                    break;
                }

                case BusinessLayer::ScreenplayTextModelItemType::Text: {
                    const auto textItem = static_cast<BusinessLayer::ScreenplayTextModelTextItem*>(item);

                    //
                    // Если это не первый абзац, вставим блок для него
                    //
                    if (!isFirstParagraph) {
                        cursor.insertBlock();
                    }
                    //
                    // ... в противном же случае, новый блок нет необходимости вставлять
                    //
                    else {
                        isFirstParagraph = false;
                    }

                    //
                    // Установим стиль блока
                    //
                    const auto currentStyle
                            = ScreenplayTemplateFacade::getTemplate().blockStyle(textItem->paragraphType());
                    cursor.setBlockFormat(currentStyle.blockFormat());
                    cursor.setBlockCharFormat(currentStyle.charFormat());
                    cursor.setCharFormat(currentStyle.charFormat());

                    //
                    // Вставим текст абзаца
                    //
                    const auto textToInsert = TextHelper::fromHtmlEscaped(textItem->text());
                    cursor.insertText(textToInsert);

                    break;
                }

                default: {
                    Q_ASSERT(false);
                    break;
                }
            }

            //
            // Считываем информацию о детях
            //
            readDocumentFromModel(itemIndex);
        }
    };
    readDocumentFromModel({});

    //
    // Завершаем операцию
    //
    cursor.endEditBlock();
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

}

void ScreenplayTextEdit::setCurrentParagraphType(BusinessLayer::ScreenplayParagraphType _type)
{

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

} // namespace Ui

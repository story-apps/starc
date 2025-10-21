#include "script_text_edit.h"

#include <business_layer/document/text/text_cursor.h>
#include <business_layer/templates/text_template.h>
#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/debouncer.h>

#include <QPainter>
#include <QRegularExpression>
#include <QTextTable>

using BusinessLayer::TextBlockStyle;


namespace Ui {

class ScriptTextEdit::Implementation
{
public:
    Implementation(ScriptTextEdit* _q);


    ScriptTextEdit* q = nullptr;

    /**
     * @brief Показывать автодополения в пустых блоках
     */
    bool showSuggestionsInEmptyBlocks = true;

    /**
     * @brief Последнее положение мыши
     */
    QPoint lastMousePos;

    /**
     * @brief Курсоры соавторов
     */
    QVector<Domain::CursorInfo> collaboratorsCursorInfo;
    QVector<Domain::CursorInfo> pendingCollaboratorsCursorInfo;
    Debouncer collaboratorCursorInfoUpdateDebouncer;
};

ScriptTextEdit::Implementation::Implementation(ScriptTextEdit* _q)
    : q(_q)
    , collaboratorCursorInfoUpdateDebouncer(500)
{
    connect(&collaboratorCursorInfoUpdateDebouncer, &Debouncer::gotWork, q, [this] {
        std::swap(collaboratorsCursorInfo, pendingCollaboratorsCursorInfo);
        q->update();
    });
}


// ****


ScriptTextEdit::ScriptTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent)
    , d(new Implementation(this))
{
}

ScriptTextEdit::~ScriptTextEdit() = default;

bool ScriptTextEdit::showSuggestionsInEmptyBlocks() const
{
    return d->showSuggestionsInEmptyBlocks;
}

void ScriptTextEdit::setShowSuggestionsInEmptyBlocks(bool _show)
{
    d->showSuggestionsInEmptyBlocks = _show;
}

void ScriptTextEdit::setTextCursorAndKeepScrollBars(const QTextCursor& _cursor)
{
    //
    // При позиционировании курсора в невидимый блок, Qt откидывает прокрутку в самый верх, поэтому
    // перед редактированием документа запомним прокрутку, а после завершения редактирования
    // восстановим значения полос прокрутки
    //
    const auto verticalScrollValue = verticalScroll();
    const auto horizontalScrollValue = horizontalScroll();

    //
    // Задаём курсор
    //
    setTextCursor(_cursor);

    //
    // Восстанавливаем значения полос прокрутки
    //
    setVerticalScroll(verticalScrollValue);
    setHorizontalScroll(horizontalScrollValue);
}

void ScriptTextEdit::setCollaboratorsCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->pendingCollaboratorsCursorInfo = _cursors;
    d->collaboratorCursorInfoUpdateDebouncer.orderWork();
}

void ScriptTextEdit::updateCollaboratorsCursors(int _position, int _charsRemoved, int _charsAdded)
{
    for (auto& collaboratorCursor : d->collaboratorsCursorInfo) {
        int collaboratorCursorPosition = collaboratorCursor.cursorData.toInt();
        if (collaboratorCursorPosition >= _position) {
            collaboratorCursorPosition += _charsAdded - _charsRemoved;
            collaboratorCursor.cursorData = QByteArray::number(collaboratorCursorPosition);
        }
    }

    d->collaboratorCursorInfoUpdateDebouncer.abortWork();
}

void ScriptTextEdit::paintCollaboratorsCursors(QPainter& _painter, const QUuid& _documentUuid,
                                               const QTextBlock& _topBlock,
                                               const QTextBlock& _bottomBlock) const
{
    if (!d->collaboratorsCursorInfo.isEmpty()) {
        for (const auto& cursorInfo : std::as_const(d->collaboratorsCursorInfo)) {
            //
            // Пропускаем курсоры из других документов
            //
            if (cursorInfo.documentUuid != _documentUuid) {
                continue;
            }

            //
            // Пропускаем курсоры, которые находятся за пределами экрана
            //
            const auto cursorPosition = cursorInfo.cursorData.toInt();
            if (_bottomBlock.isValid()
                && (cursorPosition < _topBlock.position()
                    || cursorPosition > (_bottomBlock.position() + _bottomBlock.length()))) {
                continue;
            }


            BusinessLayer::TextCursor cursor(document());
            cursor.setPosition(cursorPosition);
            if (!cursor.block().isVisible()) {
                continue;
            }

            const auto cursorR = cursorRect(cursor).adjusted(-DesignSystem::layout().px(), 0,
                                                             DesignSystem::layout().px(), 0);
            const auto backgroundColor = ColorHelper::forText(cursorInfo.name);

            //
            // ... рисуем его
            //
            _painter.fillRect(cursorR, backgroundColor);

            //
            // ... выводим имя соавтора, если курсор мыши около курсора соавтора
            //
            if (cursorR.adjusted(-DesignSystem::layout().px4(), 0, DesignSystem::layout().px4(), 0)
                    .contains(d->lastMousePos)) {
                _painter.setFont(DesignSystem::font().subtitle2());
                const QRect usernameRect(cursorR.left() - DesignSystem::layout().px4(),
                                         cursorR.top() - DesignSystem::layout().px24(),
                                         TextHelper::fineTextWidth(cursorInfo.name, _painter.font())
                                             + DesignSystem::layout().px12(),
                                         DesignSystem::layout().px24());
                _painter.setPen(Qt::NoPen);
                _painter.setBrush(backgroundColor);
                _painter.drawRoundedRect(usernameRect, DesignSystem::button().borderRadius(),
                                         DesignSystem::button().borderRadius());
                _painter.setPen(ColorHelper::contrasted(backgroundColor));
                _painter.drawText(usernameRect, Qt::AlignCenter, cursorInfo.name);
            }
        }
    }
}

void ScriptTextEdit::mouseMoveEvent(QMouseEvent* _event)
{
    d->lastMousePos = _event->pos();

    BaseTextEdit::mouseMoveEvent(_event);
}

bool ScriptTextEdit::keyPressEventReimpl(QKeyEvent* _event)
{
    //
    // Стандартно обрабатываем в базовом классе
    //
    const bool isEventHandled = BaseTextEdit::keyPressEventReimpl(_event);
    //
    // ... если это было вырезание и курсор остался в пустом контейнере таблицы, то нужно его
    //     удалить, чтобы он не оставлял за собой артефакта в виде открывающего/закрывающего блока
    //
    if (_event == QKeySequence::Cut) {
        auto cursor = textCursor();
        if (cursor.block().text().isEmpty()
            && BusinessLayer::TextBlockStyle::forCursor(cursor)
                == BusinessLayer::TextParagraphType::PageSplitter) {
            if (!cursor.atStart()) {
                cursor.deletePreviousChar();
                cursor.movePosition(QTextCursor::PreviousCharacter);
            } else {
                cursor.deleteChar();
                cursor.movePosition(QTextCursor::NextCharacter);
            }
            setTextCursor(cursor);
        }
    }

    return isEventHandled;
}

bool ScriptTextEdit::updateEnteredText(const QKeyEvent* _event)
{
    //
    // Ничего не делаем, если текст пуст,
    // или нажат хотя бы один из модификаторов делающих нажатие шорткатом
    //
    if (_event->text().isEmpty() || _event->modifiers().testFlag(Qt::ControlModifier)
        || _event->modifiers().testFlag(Qt::AltModifier)
        || _event->modifiers().testFlag(Qt::MetaModifier)) {
        return false;
    }

    //
    // Получим значения
    //
    const auto eventText = _event->text();
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
    if (currentCharFormat.boolProperty(TextBlockStyle::PropertyIsFirstUppercase)
        && cursorBackwardText != " " && cursorBackwardText == eventText
        && eventText[0] != TextHelper::smartToUpper(eventText[0])) {
        //
        // Сформируем правильное представление строки
        //
        QString correctedText = eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        cursor.beginEditBlock();
        for (int repeats = 0; repeats < eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursorAndKeepScrollBars(cursor);
        cursor.endEditBlock();

        return true;
    }

    //
    // Если перед нами конец предложения
    // и не сокращение
    // и после курсора нет текста (для ремарки допустима скобка)
    //
    static QRegularExpression endOfSentenceRx;
    endOfSentenceRx.setPattern(
        QString("([.]|[?]|[!]|[…]) %1$").arg(TextHelper::toRxEscaped(eventText)));
    if (cursorBackwardText.contains(endOfSentenceRx) && cursorForwardText.isEmpty()
        && eventText[0] != TextHelper::smartToUpper(eventText[0])) {
        //
        // Сделаем первую букву заглавной
        //
        QString correctedText = eventText;
        correctedText[0] = TextHelper::smartToUpper(correctedText[0]);

        //
        // Стираем предыдущий введённый текст
        //
        cursor.beginEditBlock();
        for (int repeats = 0; repeats < eventText.length(); ++repeats) {
            cursor.deletePreviousChar();
        }

        //
        // Выводим необходимый
        //
        cursor.insertText(correctedText);
        setTextCursorAndKeepScrollBars(cursor);
        cursor.endEditBlock();

        return true;
    }

    return BaseTextEdit::updateEnteredText(_event);
}

} // namespace Ui

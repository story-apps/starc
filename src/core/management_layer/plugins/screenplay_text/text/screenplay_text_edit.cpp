#include "screenplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/screenplay/text/screenplay_text_block_data.h>
#include <business_layer/document/screenplay/text/screenplay_text_corrector.h>
#include <business_layer/document/screenplay/text/screenplay_text_cursor.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/import/fountain_importer.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QCoreApplication>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextTable>

using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayParagraphType;
using BusinessLayer::ScreenplayTemplateFacade;

namespace Ui
{

class ScreenplayTextEdit::Implementation
{
public:
    BusinessLayer::ScreenplayTextModel* model = nullptr;
    BusinessLayer::ScreenplayTextDocument document;

    bool showSceneNumber = false;
    bool showSceneNumberOnLeft = false;
    bool showSceneNumberOnRight = false;
    bool showDialogueNumber = false;
};


// ****


ScreenplayTextEdit::ScreenplayTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent),
      d(new Implementation)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setShowPageNumbers(true);

    setDocument(&d->document);
    setCapitalizeWords(false);
}

ScreenplayTextEdit::~ScreenplayTextEdit() = default;

void ScreenplayTextEdit::setShowSceneNumber(bool _show, bool _onLeft, bool _onRight)
{
    d->showSceneNumber = _show;
    d->showSceneNumberOnLeft = _onLeft;
    d->showSceneNumberOnRight = _onRight;
    update();
}

void ScreenplayTextEdit::setShowDialogueNumber(bool _show)
{
    d->showDialogueNumber = _show;
    update();
}

void ScreenplayTextEdit::initWithModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->model = _model;

    const auto currentTemplate = BusinessLayer::ScreenplayTemplateFacade::getTemplate();
    setPageFormat(currentTemplate.pageSizeId());
    setPageMargins(currentTemplate.pageMargins());
    setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний изменений
    //
    d->document.setModel(_model);
}

void ScreenplayTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);

    //
    // Пересчитаем хронометраж
    //
    if (d->model != nullptr) {
        d->model->recalculateDuration();
    }
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

void ScreenplayTextEdit::undo()
{
    if (d->model == nullptr) {
        return;
    }

    const auto lastCursorPosition = textCursor().position();
    //
    d->model->undo();
    //
    if (d->document.characterCount() > lastCursorPosition) {
        auto cursor = textCursor();
        cursor.setPosition(lastCursorPosition);
        setTextCursorReimpl(cursor);
        ensureCursorVisible();
    }
}

void ScreenplayTextEdit::redo()
{
    if (d->model == nullptr) {
        return;
    }

    const auto lastCursorPosition = textCursor().position();
    //
    d->model->redo();
    //
    if (d->document.characterCount() > lastCursorPosition) {
        auto cursor = textCursor();
        cursor.setPosition(lastCursorPosition);
        setTextCursorReimpl(cursor);
        ensureCursorVisible();
    }
}

void ScreenplayTextEdit::addParagraph(BusinessLayer::ScreenplayParagraphType _type)
{
    d->document.addParagraph(_type, textCursor());

    emit paragraphTypeChanged();
}

void ScreenplayTextEdit::setCurrentParagraphType(BusinessLayer::ScreenplayParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    d->document.setParagraphType(_type, textCursor());

    //
    // Если вставили папку, то нужно перейти к предыдущему блоку (из футера к хидеру)
    //
    if (_type == ScreenplayParagraphType::FolderHeader) {
        moveCursor(QTextCursor::PreviousBlock);
    }

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

QModelIndex ScreenplayTextEdit::currentModelIndex() const
{
    if (d->model == nullptr) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto screenplayBlockData = static_cast<BusinessLayer::ScreenplayTextBlockData*>(userData);
    return d->model->indexForItem(screenplayBlockData->item());
}

void ScreenplayTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    BusinessLayer::ScreenplayTextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));
    ensureCursorVisible(textCursor);
}

int ScreenplayTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void ScreenplayTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor, const QString& _comment)
{
    BusinessLayer::ScreenplayTextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, cursor);
}

void ScreenplayTextEdit::keyPressEvent(QKeyEvent* _event)
{
    if (isReadOnly()) {
        return;
    }

    //
    // Подготовим событие к обработке
    //
    _event->setAccepted(false);

    //
    // Получим обработчик
    //
    auto handler = KeyProcessingLayer::KeyPressHandlerFacade::instance(this);

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
        if (keyPressEventReimpl(_event)) {
            _event->accept();
        } else {
            BaseTextEdit::keyPressEvent(_event);
            _event->ignore();
        }

        updateEnteredText(_event->text());
    }

    //
    // Обработка
    //
    if (!_event->isAccepted()) {
        handler->handle(_event);
    }

    //
    // Событие дошло по назначению
    //
    _event->accept();

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
    // ... отмена последнего действия
    //
    if (_event == QKeySequence::Undo) {
        undo();
    }
    //
    // ... отмена последнего действия
    //
    else if (_event == QKeySequence::Redo) {
        redo();
    }
    //
    // ... вырезать текст
    //
    else if (_event == QKeySequence::Cut) {
        copy();
        BusinessLayer::ScreenplayTextCursor cursor = textCursor();
        cursor.removeCharacters(this);
        d->model->saveChanges();
    }
    //
    // ... вставить текст
    //
    else if (_event == QKeySequence::Paste) {
        paste();
        d->model->saveChanges();
    }
    //
    // ... перевод курсора к следующему символу
    //
    else  if (_event == QKeySequence::MoveToNextChar) {
        if (textCursor().block().textDirection() == Qt::LeftToRight) {
            moveCursor(QTextCursor::NextCharacter);
        } else {
            moveCursor(QTextCursor::PreviousCharacter);
        }

        while (!textCursor().atEnd()
               && (!textCursor().block().isVisible()
                   || ScreenplayBlockStyle::forBlock(textCursor().block()) == ScreenplayParagraphType::PageSplitter
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
                   || ScreenplayBlockStyle::forBlock(textCursor().block()) == ScreenplayParagraphType::PageSplitter
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
    // ... вставим перенос строки внутри абзаца
    //
    else if ((_event->key() == Qt::Key_Enter || _event->key() == Qt::Key_Return)
             && _event->modifiers().testFlag(Qt::ShiftModifier)) {
        textCursor().insertText(QChar(QChar::LineSeparator));
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
        && cursorBackwardText == _eventText
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

    //
    // Если перед нами конец предложения
    // и не сокращение
    // и после курсора нет текста (для ремарки допустима скобка)
    //
    const QString endOfSentancePattern = QString("([.]|[?]|[!]|[…]) %1$").arg(_eventText);
    if (cursorBackwardText.contains(QRegularExpression(endOfSentancePattern))
        && cursorForwardText.isEmpty()
        && _eventText[0] != TextHelper::smartToUpper(_eventText[0])) {
        //
        // Сделаем первую букву заглавной
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

    //
    // Если была попытка ввести несколько пробелов подряд, или пробел в начале строки,
    // удаляем этот лишний пробел
    //
    if (cursorBackwardText == " "
        || cursorBackwardText.endsWith("  ")) {
        cursor.deletePreviousChar();

        return true;
    }

    return BaseTextEdit::updateEnteredText(_eventText);
}

void ScreenplayTextEdit::paintEvent(QPaintEvent* _event)
{
    BaseTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight = viewport()->width();
    const qreal spaceBetweenSceneNumberAndText = 10 * Ui::DesignSystem::scaleFactor();;
    const qreal textLeft = pageLeft
                           - (isLeftToRight ? 0 : horizontalScrollBar()->maximum())
                           + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight
                            + (isLeftToRight ? horizontalScrollBar()->maximum() : 0)
                            - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScrollBar()->value();
//    int colorRectWidth = 0;
    qreal verticalMargin = 0;
    const qreal splitterX = leftDelta + textLeft
                            + (textRight - textLeft)
                            * ScreenplayTemplateFacade::getTemplate().leftHalfOfPageWidthPercents() / 100;


    //
    // Определим начальный блок на экране
    //
    QTextBlock topBlock = document()->lastBlock();
    {
        QTextCursor topCursor;
        for (int delta = 0; delta < viewport()->height()/4; delta += 10) {
            topCursor = cursorForPosition(viewport()->mapFromParent(QPoint(0, delta)));
            if (topBlock.blockNumber() > topCursor.block().blockNumber()) {
                topBlock = topCursor.block();
            }
        }
    }
    //
    // ... идём до начала сцены
    //
    while (ScreenplayBlockStyle::forBlock(topBlock) != ScreenplayParagraphType::SceneHeading
           && ScreenplayBlockStyle::forBlock(topBlock) != ScreenplayParagraphType::FolderHeader
           && topBlock != document()->firstBlock()) {
        topBlock = topBlock.previous();
    }

    //
    // Определим последний блок на экране
    //
    QTextBlock bottomBlock = document()->firstBlock();
    {
        BusinessLayer::ScreenplayTextCursor bottomCursor;
        for (int delta = viewport()->height(); delta > viewport()->height()*3/4; delta -= 10) {
            bottomCursor = cursorForPosition(viewport()->mapFromParent(QPoint(0, delta)));
            if (bottomBlock.blockNumber() < bottomCursor.block().blockNumber()) {
                bottomBlock = bottomCursor.block();
            }
        }
    }
    if (bottomBlock == document()->firstBlock()) {
        bottomBlock = document()->lastBlock();
    }
    bottomBlock = bottomBlock.next();
    //
    // ... в случае, если блок попал в таблицу, нужно дойти до конца таблицы
    //
    {
        BusinessLayer::ScreenplayTextCursor bottomCursor(document());
        bottomCursor.setPosition(bottomBlock.position());
        while (bottomCursor.inTable() && bottomCursor.movePosition(QTextCursor::NextBlock)) {
            bottomBlock = bottomCursor.block();
        }
    }

    //
    // Прорисовка дополнительных элементов редактора
    //
    {
        //
        // Декорации текста
        //
        {
            QPainter painter(viewport());
            clipPageDecorationRegions(&painter);

            //
            // Проходим блоки на экране и декорируем их
            //
            QTextBlock block = topBlock;
            const QRectF viewportGeometry = viewport()->geometry();
            int lastSceneBlockBottom = 0;
            QColor lastSceneColor;
            int lastCharacterBlockBottom = 0;
            QColor lastCharacterColor;

            BusinessLayer::ScreenplayTextCursor cursor(document());
            while (block.isValid() && block != bottomBlock) {
                //
                // Стиль текущего блока
                //
                const auto blockType = ScreenplayBlockStyle::forBlock(block);

                cursor.setPosition(block.position());
                const QRect cursorR = cursorRect(cursor);
                cursor.movePosition(QTextCursor::EndOfBlock);
                const QRect cursorREnd = cursorRect(cursor);
                //
                verticalMargin = cursorR.height() / 2;

//                //
//                // Определим цвет сцены
//                //
//                if (blockType == ScenarioBlockStyle::SceneHeading
//                    || blockType == ScenarioBlockStyle::FolderHeader) {
//                    lastSceneBlockBottom = cursorR.top();
//                    colorRectWidth = QFontMetrics(cursor.charFormat().font()).width(".");
//                    lastSceneColor = QColor();
//                    if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(block.userData())) {
//                        if (!info->colors().isEmpty()) {
//                            lastSceneColor = QColor(info->colors().split(";").first());
//                        }
//                    }
//                }

//                //
//                // Нарисуем цвет сцены
//                //
//                if (lastSceneColor.isValid()) {
//                    const QPointF topLeft(isLeftToRight
//                                    ? textRight + leftDelta
//                                    : textLeft - colorRectWidth + leftDelta,
//                                    lastSceneBlockBottom - verticalMargin);
//                    const QPointF bottomRight(isLeftToRight
//                                        ? textRight + colorRectWidth + leftDelta
//                                        : textLeft + leftDelta,
//                                        cursorREnd.bottom() + verticalMargin);
//                    const QRectF rect(topLeft, bottomRight);
//                    painter.fillRect(rect, lastSceneColor);
//                }

//                //
//                // Определим цвет персонажа
//                //
//                if (blockType == ScenarioBlockStyle::Character) {
//                    lastCharacterBlockBottom = cursorR.top();
//                    colorRectWidth = QFontMetrics(cursor.charFormat().font()).width(".");
//                    lastCharacterColor = QColor();
//                    const QString characterName = BusinessLogic::CharacterParser::name(block.text());
//                    if (auto character = DataStorageLayer::StorageFacade::researchStorage()->character(characterName)) {
//                        if (character->color().isValid()) {
//                            lastCharacterColor = character->color();
//                        }
//                    }
//                } else if (blockType != ScenarioBlockStyle::Parenthetical
//                           && blockType != ScenarioBlockStyle::Dialogue
//                           && blockType != ScenarioBlockStyle::Lyrics) {
//                    lastCharacterColor = QColor();
//                }

//                //
//                // Нарисуем цвет персонажа
//                //
//                if (lastCharacterColor.isValid()) {
//                    const QPointF topLeft(isLeftToRight
//                                    ? textLeft - colorRectWidth + leftDelta
//                                    : textRight + leftDelta,
//                                    lastCharacterBlockBottom - verticalMargin);
//                    const QPointF bottomRight(isLeftToRight
//                                        ? textLeft + leftDelta
//                                        : textRight + colorRectWidth + leftDelta,
//                                        cursorREnd.bottom() + verticalMargin);
//                    const QRectF rect(topLeft, bottomRight);
//                    painter.fillRect(rect, lastCharacterColor);
//                }

                //
                // Курсор на экране
                //
                // ... ниже верхней границы
                if ((cursorR.top() > 0 || cursorR.bottom() > 0)
                    // ... и выше нижней
                    && cursorR.top() < viewportGeometry.bottom()) {

//                    //
//                    // Прорисовка закладок
//                    //
//                    if (blockInfo != nullptr
//                        && blockInfo->hasBookmark()) {
//                        //
//                        // Определим область для отрисовки и выведем закладку в редактор
//                        //
//                        QPointF topLeft(isLeftToRight
//                                        ? pageLeft + leftDelta
//                                        : textRight + leftDelta,
//                                        cursorR.top());
//                        QPointF bottomRight(isLeftToRight
//                                            ? textLeft + leftDelta
//                                            : pageRight + leftDelta,
//                                            cursorR.bottom());
//                        QRectF rect(topLeft, bottomRight);
//                        painter.setBrush(blockInfo->bookmarkColor());
//                        painter.setPen(Qt::transparent);
//                        painter.drawRect(rect);
//                        painter.setPen(Qt::white);
//                    } else {
//                        painter.setPen(palette().text().color());
//                    }

//                    //
//                    // Новый способ отрисовки
//                    //
//                    //
//                    // Определим область для отрисовки и выведем номер сцены в редактор в зависимости от стороны
//                    //
//                    QPointF topLeft(isLeftToRight
//                                    ? pageLeft + leftDelta
//                                    : textRight + leftDelta,
//                                    cursorR.top());
//                    QPointF bottomRight(isLeftToRight
//                                        ? textLeft + leftDelta
//                                        : pageRight + leftDelta,
//                                        cursorR.bottom());
//                    QRectF rect(topLeft, bottomRight);
//                    rect.adjust(38, 0, 0, 0);
//                    painter.setFont(DesignSystem::font().iconsMid());
//                    const int size = painter.fontMetrics().lineSpacing();
//                    QRectF circle(rect.left() - size, rect.top() - size, size * 3, size * 3);
//                    painter.setBrush(ColorHelper::transparent(palette().text().color(), Ui::DesignSystem::hoverBackgroundOpacity()));
//                    painter.setPen(Qt::NoPen);
//                    painter.drawRect(circle);
//                    painter.setPen(palette().text().color());
//                    painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, u8"\U000F024B");

                    //
                    // Прорисовка декораций пустой строки
                    //
                    if (!block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)
                        && blockType != ScreenplayParagraphType::PageSplitter
                        && block.text().simplified().isEmpty()) {
                        //
                        // Для пустого футера рисуем плейсхолдер
                        //
                        if (blockType == ScreenplayParagraphType::FolderFooter) {
                            painter.setFont(block.charFormat().font());

                            //
                            // Ищем открывающий блок папки
                            //
                            auto headerBlock = block.previous();
                            int openedFolders = 0;
                            while (headerBlock.isValid()) {
                                const auto headerBlockType = ScreenplayBlockStyle::forBlock(headerBlock);
                                if (headerBlockType == ScreenplayParagraphType::FolderHeader) {
                                    if (openedFolders > 0) {
                                        --openedFolders;
                                    } else {
                                        break;
                                    }
                                } else if (headerBlockType == ScreenplayParagraphType::FolderFooter) {
                                    ++openedFolders;
                                }

                                headerBlock = headerBlock.previous();
                            }

                            //
                            // Определим область для отрисовки плейсхолдера
                            //
                            const auto placeholderText
                                    = QString("%1 %2")
                                      .arg(QCoreApplication::translate("KeyProcessingLayer::FolderFooterHandler", "END OF"),
                                           headerBlock.text());
                            const QPoint topLeft = QPoint(textLeft
                                                          + leftDelta
                                                          + spaceBetweenSceneNumberAndText,
                                                          cursorR.top());
                            const QPoint bottomRight = QPoint(textRight
                                                              + leftDelta
                                                              - spaceBetweenSceneNumberAndText,
                                                              cursorR.bottom());
                            const QRect rect(topLeft, bottomRight);
                            painter.drawText(rect, block.blockFormat().alignment(), placeholderText);
                        }
                        //
                        // В остальных случаях рисуем индикатор пустой строки
                        //
                        else {
                            painter.setFont(block.charFormat().font());
                            const QString emptyLineMark = "» ";
                            //
                            // Определим область для отрисовки и выведем символ в редактор
                            //
                            // ... в тексте или в первой колоке таблички
                            //
                            if (!cursor.inTable() || cursor.inFirstColumn()) {
                                const QPointF topLeft(isLeftToRight
                                                      ? pageLeft + leftDelta
                                                      : textRight + leftDelta,
                                                      cursorR.top());
                                const QPointF bottomRight(isLeftToRight
                                                          ? textLeft + leftDelta
                                                          : pageRight + leftDelta,
                                                          cursorR.bottom() + 2);
                                const QRectF rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, emptyLineMark);
                            }
                            //
                            // ... во второй колонке таблички
                            //
                            else {
                                const qreal x = splitterX - cursor.currentTable()->format().border();
                                const QPointF topLeft(x - painter.fontMetrics().horizontalAdvance(emptyLineMark),
                                                      cursorR.top());
                                const QPointF bottomRight(x, cursorR.bottom() + 2);
                                const QRectF rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, emptyLineMark);
                            }
                        }
                    }
                    //
                    // Прорисовка декораций непустых строк
                    //
                    else {
                        //
                        // Прорисовка значков папки (можно использовать для закладок)
                        //
                        if (blockType == ScreenplayParagraphType::FolderHeader) {
                            //
                            // Определим область для отрисовки и выведем номер сцены в редактор в зависимости от стороны
                            //
                            QPointF topLeft(isLeftToRight
                                            ? pageLeft + leftDelta
                                            : textRight + leftDelta,
                                            cursorR.top());
                            QPointF bottomRight(isLeftToRight
                                                ? textLeft + leftDelta
                                                : pageRight + leftDelta,
                                                cursorR.bottom());
                            QRectF rect(topLeft, bottomRight);
                            rect.adjust(0, -4, - QFontMetricsF(cursor.charFormat().font()).horizontalAdvance(".")/2, 0);
                            painter.setFont(DesignSystem::font().iconsBig());
                            painter.setPen(palette().text().color());
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, u8"\U000F024B");
                        }
                        //
                        // Прорисовка номеров сцен, если необходимо
                        //
                        if (d->showSceneNumber
                            && blockType == ScreenplayParagraphType::SceneHeading) {
                            //
                            // Определим номер сцены
                            //
                            const auto sceneNumber = d->document.sceneNumber(block);
                            if (!sceneNumber.isEmpty()) {
                                //
                                // Определим область для отрисовки и выведем номер сцены в редактор в зависимости от стороны
                                //
                                if (d->showSceneNumberOnLeft) {
                                    QPointF topLeft(isLeftToRight
                                                    ? pageLeft + leftDelta
                                                    : textRight + leftDelta,
                                                    cursorR.top());
                                    QPointF bottomRight(isLeftToRight
                                                        ? textLeft + leftDelta
                                                        : pageRight + leftDelta,
                                                        cursorR.bottom());
                                    QRectF rect(topLeft, bottomRight);
                                    painter.setFont(cursor.charFormat().font());
                                    painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, sceneNumber);
                                }
                                if (d->showSceneNumberOnRight) {
                                    QPointF topLeft(isLeftToRight
                                                    ? textRight - leftDelta
                                                    : pageLeft - leftDelta,
                                                    cursorR.top());
                                    QPointF bottomRight(isLeftToRight
                                                        ? pageRight - leftDelta
                                                        : textLeft - leftDelta,
                                                        cursorR.bottom());
                                    QRectF rect(topLeft, bottomRight);
                                    painter.setFont(cursor.charFormat().font());
                                    painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, sceneNumber);
                                }
                            }
                        }
                        //
                        // Прорисовка номеров реплик, если необходимо
                        //
                        if (d->showDialogueNumber
                            && blockType == ScreenplayParagraphType::Character) {
                            //
                            // Определим номер реплики
                            //
                            const auto dialogueNumber = d->document.dialogueNumber(block);
                            if (!dialogueNumber.isEmpty()) {
                                //
                                // Определим область для отрисовки и выведем номер реплики в редактор
                                //
                                painter.setFont(cursor.charFormat().font());
                                const int numberDelta = painter.fontMetrics().horizontalAdvance(dialogueNumber);
                                QRectF rect;
                                //
                                // Если имя персонажа находится не с самого края листа
                                //
                                if (block.blockFormat().leftMargin() > numberDelta) {
                                    //
                                    // ... то поместим номер реплики внутри текстовой области,
                                    //     чтобы их было удобно отличать от номеров сцен
                                    //
                                    QPointF topLeft(isLeftToRight
                                                    ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                                    : textRight + leftDelta - spaceBetweenSceneNumberAndText - numberDelta,
                                                    cursorR.top());
                                    QPointF bottomRight(isLeftToRight
                                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText + numberDelta
                                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                                        cursorR.bottom());
                                    rect = QRectF(topLeft, bottomRight);
                                }
                                //
                                // В противном же случае
                                //
                                else {
                                    //
                                    // ... позиционируем номера реплик на полях, так же как и номера сцен
                                    //
                                    QPointF topLeft(isLeftToRight
                                                    ? pageLeft + leftDelta
                                                    : textRight + leftDelta,
                                                    cursorR.top());
                                    QPointF bottomRight(isLeftToRight
                                                        ? textLeft + leftDelta
                                                        : pageRight + leftDelta,
                                                        cursorR.bottom());
                                    rect = QRectF(topLeft, bottomRight);
                                }
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, dialogueNumber);
                            }
                        }

                        //
                        // Прорисовка автоматических (ПРОД) для реплик
                        //
                        if (blockType == ScreenplayParagraphType::Character
                            && block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCharacterContinued)
                            && !block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                            painter.setFont(cursor.charFormat().font());

                            //
                            // Определим место положение конца имени персонажа
                            //
                            const int continuedTermWidth
                                    = painter.fontMetrics().horizontalAdvance(
                                          BusinessLayer::ScreenplayTextCorrector::continuedTerm());
                            const QPoint topLeft = isLeftToRight
                                                   ? cursorREnd.topLeft()
                                                   : cursorREnd.topRight() - QPoint(continuedTermWidth, 0);
                            const QPoint bottomRight = isLeftToRight
                                                       ? cursorREnd.bottomRight() + QPoint(continuedTermWidth, 0)
                                                       : cursorREnd.bottomLeft();
                            const QRect rect(topLeft, bottomRight);
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignTop,
                                             BusinessLayer::ScreenplayTextCorrector::continuedTerm());
                        }
                    }

                    //
                    // Прорисовка префикса/постфикса для блока текста, если это не пустая декорация
                    //
                    if (!block.text().isEmpty()
                        || !block.blockFormat().boolProperty(ScreenplayBlockStyle::PropertyIsCorrection)) {
                        //
                        // ... префикс
                        //
                        if (block.charFormat().hasProperty(ScreenplayBlockStyle::PropertyPrefix)) {
                            painter.setFont(block.charFormat().font());

                            const auto prefix = block.charFormat().stringProperty(ScreenplayBlockStyle::PropertyPrefix);
                            const QPoint topLeft = QPoint(cursorR.left()
                                                          - painter.fontMetrics().horizontalAdvance(prefix),
                                                          cursorR.top());
                            const QPoint bottomRight = QPoint(cursorR.left(),
                                                              cursorR.bottom());
                            const QRect rect(topLeft, bottomRight);
                            painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, prefix);
                        }
                        //
                        // ... постфикс
                        //
                        if (block.charFormat().hasProperty(ScreenplayBlockStyle::PropertyPostfix)) {
                            painter.setFont(block.charFormat().font());

                            const auto postfix = block.charFormat().stringProperty(ScreenplayBlockStyle::PropertyPostfix);
                            const QPoint topLeft = QPoint(cursorREnd.left(),
                                                          cursorREnd.top());
                            const QPoint bottomRight = QPoint(cursorREnd.left()
                                                              + painter.fontMetrics().horizontalAdvance(postfix),
                                                              cursorREnd.bottom());
                            const QRect rect(topLeft, bottomRight);
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, postfix);
                        }
                    }
                }

                lastSceneBlockBottom = cursorREnd.bottom();

                block = block.next();
            }
        }

//        //
//        // Курсоры соавторов
//        //
//        {
//            //
//            // Ширина области курсора, для отображения имени автора курсора
//            //
//            const unsigned cursorAreaWidth = 20;

//            if (!m_additionalCursors.isEmpty()
//                && m_document != nullptr) {
//                QPainter painter(viewport());
//                painter.setFont(QFont("Sans", 8));
//                painter.setPen(Qt::white);

//                const QRectF viewportGeometry = viewport()->geometry();
//                QPoint mouseCursorPos = mapFromGlobal(QCursor::pos());
//                mouseCursorPos.setY(mouseCursorPos.y() + viewport()->mapFromParent(QPoint(0,0)).y());
//                int cursorIndex = 0;
//                foreach (const QString& username, m_additionalCursorsCorrected.keys()) {
//                    QTextCursor cursor(m_document);
//                    m_document->setCursorPosition(cursor, m_additionalCursorsCorrected.value(username));
//                    const QRect cursorR = cursorRect(cursor).adjusted(0, 0, 1, 0);

//                    //
//                    // Если курсор на экране
//                    //
//                    // ... ниже верхней границы
//                    if ((cursorR.top() > 0 || cursorR.bottom() > 0)
//                        // ... и выше нижней
//                        && cursorR.top() < viewportGeometry.bottom()) {
//                        //
//                        // ... рисуем его
//                        //
//                        painter.fillRect(cursorR, ColorHelper::cursorColor(cursorIndex));

//                        //
//                        // ... декорируем
//                        //
//                        {
//                            //
//                            // Если мышь около него, то выводим имя соавтора
//                            //
//                            QRect extandedCursorR = cursorR;
//                            extandedCursorR.setLeft(extandedCursorR.left() - cursorAreaWidth/2);
//                            extandedCursorR.setWidth(cursorAreaWidth);
//                            if (extandedCursorR.contains(mouseCursorPos)) {
//                                const QRect usernameRect(
//                                    cursorR.left() - 1,
//                                    cursorR.top() - painter.fontMetrics().height() - 2,
//                                    painter.fontMetrics().width(username) + 2,
//                                    painter.fontMetrics().height() + 2);
//                                painter.fillRect(usernameRect, ColorHelper::cursorColor(cursorIndex));
//                                painter.drawText(usernameRect, Qt::AlignCenter, username);
//                            }
//                            //
//                            // Если нет, то рисуем небольшой квадратик
//                            //
//                            else {
//                                painter.fillRect(cursorR.left() - 2, cursorR.top() - 5, 5, 5,
//                                    ColorHelper::cursorColor(cursorIndex));
//                            }
//                        }
//                    }

//                    ++cursorIndex;
//                }
//            }
//        }
    }
}

ContextMenu* ScreenplayTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    auto menu = BaseTextEdit::createContextMenu(_position, _parent);

    auto splitAction = new QAction;
    if (BusinessLayer::ScreenplayTextCursor cursor = textCursor(); cursor.inTable()) {
        splitAction->setText(tr("Merge paragraph"));
        splitAction->setIconText(u8"\U000f10e7");
    } else {
        splitAction->setText(tr("Split paragraph"));
        splitAction->setIconText(u8"\U000f10e7");
    }
    connect(splitAction, &QAction::triggered, this, [this, menu] {
        menu->hideContextMenu();
        BusinessLayer::ScreenplayTextCursor cursor = textCursor();
        if (cursor.inTable()) {
            d->document.mergeParagraph(cursor);
        } else {
            d->document.splitParagraph(cursor);

            //
            // После разделения, возвращаемся в первую ячейку таблицы
            //
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::EndOfBlock);
        }
    });

    menu->setActions({ splitAction });

    return menu;
}

bool ScreenplayTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first())
            || _source->hasText();
}

QMimeData* ScreenplayTextEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    BusinessLayer::ScreenplayTextCursor cursor = textCursor();
    const auto selection = cursor.selectionInterval();

    //
    // Сформируем в текстовом виде, для вставки наружу
    // TODO: экспорт в фонтан
    //
    {
        QByteArray text;
        auto cursor = textCursor();
        cursor.setPosition(selection.from);
        do {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (cursor.position() > selection.to) {
                cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
            }
            if (!text.isEmpty()) {
                text.append("\r\n");
            }
            text.append(cursor.blockCharFormat().fontCapitalization() == QFont::AllUppercase
                        ? TextHelper::smartToUpper(cursor.selectedText())
                        : cursor.selectedText());
        } while (cursor.position() < textCursor().selectionEnd()
                 && !cursor.atEnd()
                 && cursor.movePosition(QTextCursor::NextBlock));

        mimeData->setData("text/plain", text);
    }

    //
    // Поместим в буфер данные о тексте в специальном формате
    //
    {
        mimeData->setData(d->model->mimeTypes().first(),
                          d->document.mimeFromSelection(selection.from, selection.to).toUtf8());
    }

    return mimeData;
}

void ScreenplayTextEdit::insertFromMimeData(const QMimeData* _source)
{
    if (isReadOnly()) {
        return;
    }

    //
    // Удаляем выделенный текст
    //
    if (textCursor().hasSelection()) {
        textCursor().deleteChar();
    }

    //
    // Вставляем сценарий из майм-данных
    //
    QString textToInsert;

    //
    // Если вставляются данные в сценарном формате, то вставляем как положено
    //
    if (_source->formats().contains(d->model->mimeTypes().first())) {
        textToInsert = _source->data(d->model->mimeTypes().first());
    }
    //
    // Если простой текст, то вставляем его, импортировав с фонтана
    // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
    //       не воспринимался как титульная страница
    //
    else if (_source->hasText()) {
        BusinessLayer::FountainImporter fountainImporter;
        textToInsert = fountainImporter.importScreenplay("\n" + _source->text()).text;
    }

    d->document.insertFromMime(textCursor().position(), textToInsert);
}

void ScreenplayTextEdit::dropEvent(QDropEvent* _event)
{
    if (_event->dropAction() == Qt::MoveAction) {
        BusinessLayer::ScreenplayTextCursor cursor = textCursor();
        cursor.removeCharacters(this);
    }

    d->document.startMimeDropping();
    PageTextEdit::dropEvent(_event);
    d->document.finishMimeDropping();
}

} // namespace Ui

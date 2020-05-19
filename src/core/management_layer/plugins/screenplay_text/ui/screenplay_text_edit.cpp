#include "screenplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"
#include "screenplay_text_block_data.h"
#include "screenplay_text_corrector.h"
#include "screenplay_text_cursor.h"
#include "screenplay_text_document.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <ui/widgets/context_menu/context_menu.h>

#include <utils/helpers/text_helper.h>

#include <QCoreApplication>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QScrollBar>
#include <QStandardItemModel>
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
    ScreenplayTextDocument document;
};


// ****


ScreenplayTextEdit::ScreenplayTextEdit(QWidget* _parent)
    : BaseTextEdit(_parent),
      d(new Implementation)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);

    setDocument(&d->document);
    setCapitalizeWords(false);

    connect(this, &ScreenplayTextEdit::cursorPositionChanged, this, [this] {
        if (d->model == nullptr) {
            return;
        }

        auto userData = textCursor().block().userData();
        if (userData == nullptr) {
            return;
        }

        auto screenplayBlockData = static_cast<Ui::ScreenplayTextBlockData*>(userData);
        emit currentModelIndexChanged(d->model->indexForItem(screenplayBlockData->item()));
    });
}

ScreenplayTextEdit::~ScreenplayTextEdit() = default;

void ScreenplayTextEdit::initWithModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->model = _model;

    const auto currentTemplate = BusinessLayer::ScreenplayTemplateFacade::getTemplate();
    setPageFormat(currentTemplate.pageSizeId());
    setPageMargins(currentTemplate.pageMargins());
    setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
    d->document.setDefaultFont(currentTemplate.blockStyle(ScreenplayParagraphType::SceneHeading).font());

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний изменений
    //
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
    d->document.addParagraph(_type, textCursor());

    emit paragraphTypeChanged();
}

void ScreenplayTextEdit::setCurrentParagraphType(BusinessLayer::ScreenplayParagraphType _type)
{
    d->document.setCurrentParagraphType(_type, textCursor());

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
    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto screenplayBlockData = static_cast<Ui::ScreenplayTextBlockData*>(userData);
    return d->model->indexForItem(screenplayBlockData->item());
}

void ScreenplayTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    ScreenplayTextCursor textCursor(document());
    textCursor.setPosition(d->document.itemPosition(_index));
    ensureCursorVisible(textCursor);
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

    return BaseTextEdit::updateEnteredText(_eventText);
}

void ScreenplayTextEdit::paintEvent(QPaintEvent* _event)
{
    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const int pageLeft = 0;
    const int pageRight = viewport()->width();
    const int spaceBetweenSceneNumberAndText = 10;
    const int textLeft = pageLeft
                         - (isLeftToRight ? 0 : horizontalScrollBar()->maximum())
                         + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const int textRight = pageRight
                          + (isLeftToRight ? horizontalScrollBar()->maximum() : 0)
                          - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const int leftDelta = (isLeftToRight ? -1 : 1) * horizontalScrollBar()->value();
//    int colorRectWidth = 0;
    int verticalMargin = 0;
    const int splitterX = leftDelta + textLeft
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
        ScreenplayTextCursor bottomCursor;
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
        ScreenplayTextCursor bottomCursor(document());
        bottomCursor.setPosition(bottomBlock.position());
        while (bottomCursor.inTable() && bottomCursor.movePosition(QTextCursor::NextBlock)) {
            bottomBlock = bottomCursor.block();
        }
    }


//    //
//    // Подсветка блоков и строки, если нужно
//    //
//    {
//        QPainter painter(viewport());
//        if (m_highlightBlocks) {
//            const int opacity = 33;

//            //
//            // Если курсор в блоке реплики, закрасить примыкающие блоки диалогов
//            //
//            {
//                const QVector<ScenarioBlockStyle::Type> dialogueTypes = { ScenarioBlockStyle::Character,
//                                                                          ScenarioBlockStyle::Parenthetical,
//                                                                          ScenarioBlockStyle::Dialogue,
//                                                                          ScenarioBlockStyle::Lyrics};
//                if (dialogueTypes.contains(scenarioBlockType())) {
//                    //
//                    // Идём до самого начала блоков диалогов
//                    //
//                    QTextCursor cursor = textCursor();
//                    while (cursor.movePosition(QTextCursor::PreviousBlock)) {
//                        if (!dialogueTypes.contains(ScenarioBlockStyle::forBlock(cursor.block()))) {
//                            cursor.movePosition(QTextCursor::NextBlock);
//                            break;
//                        }
//                    }

//                    //
//                    // Проходим каждый из блоков по-очереди
//                    //
//                    do {
//                        const QRect topCursorRect = cursorRect(cursor);
//                        const QString characterName = BusinessLogic::CharacterParser::name(cursor.block().text());
//                        const Domain::Research* character = DataStorageLayer::StorageFacade::researchStorage()->character(characterName);
//                        const QColor characterColor = character == nullptr ? QColor() : character->color();

//                        //
//                        // Идём до конца блока диалога
//                        //
//                        while (cursor.movePosition(QTextCursor::EndOfBlock)
//                               && cursor.movePosition(QTextCursor::NextBlock)) {
//                            const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(cursor.block());
//                            //
//                            // Если дошли до персонажа, или до конца диалога, возвращаемся на блок назад и раскрашиваем
//                            //
//                            if (blockType == ScenarioBlockStyle::Character
//                                || !dialogueTypes.contains(blockType)) {
//                                cursor.movePosition(QTextCursor::StartOfBlock);
//                                cursor.movePosition(QTextCursor::PreviousCharacter);
//                                break;
//                            }
//                        }
//                        const QRect bottomCursorRect = cursorRect(cursor);
//                        cursor.movePosition(QTextCursor::NextBlock);

//                        if (characterName.isEmpty()
//                            && !characterColor.isValid()) {
//                            continue;
//                        }

//                        //
//                        // Раскрашиваем
//                        //
//                        verticalMargin = topCursorRect.height() / 2;
//                        QColor noizeColor = textColor();
//                        if (characterColor.isValid()) {
//                            noizeColor = characterColor;
//                        }
//                        noizeColor.setAlpha(opacity);
//                        const QRect noizeRect(QPoint(textLeft + leftDelta, topCursorRect.top() - verticalMargin),
//                                              QPoint(textRight + leftDelta, bottomCursorRect.bottom() + verticalMargin));
//                        painter.fillRect(noizeRect, noizeColor);
//                    } while(!cursor.atEnd()
//                            && dialogueTypes.contains(ScenarioBlockStyle::forBlock(cursor.block())));
//                }
//            }

//            //
//            // Закрасить блок сцены/папки
//            //
//            {
//                //
//                // Идём до начала блока сцены, считая по пути вложенные папки
//                //
//                QTextCursor cursor = textCursor();
//                int closedFolders = 0;
//                bool isScene = scenarioBlockType() == ScenarioBlockStyle::SceneHeading;
//                if (scenarioBlockType() != ScenarioBlockStyle::SceneHeading
//                    && scenarioBlockType() != ScenarioBlockStyle::FolderHeader) {
//                    while (cursor.movePosition(QTextCursor::PreviousBlock)) {
//                        const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(cursor.block());
//                        if (blockType == ScenarioBlockStyle::SceneHeading) {
//                            isScene = true;
//                            break;
//                        } else if (blockType == ScenarioBlockStyle::FolderFooter) {
//                            ++closedFolders;
//                        } else if (blockType == ScenarioBlockStyle::FolderHeader) {
//                            if (closedFolders > 0) {
//                                --closedFolders;
//                            } else {
//                                break;
//                            }
//                        }
//                    }
//                }
//                const QRect topCursorRect = cursorRect(cursor);
//                QColor noizeColor = textColor();
//                if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(cursor.block().userData())) {
//                    if (!info->colors().isEmpty()) {
//                        noizeColor = QColor(info->colors().split(";").first());
//                    }
//                }
//                noizeColor.setAlpha(opacity);

//                //
//                // Идём до конца блока сцены, считая по пути вложенные папки
//                //
//                cursor = textCursor();
//                int openedFolders = 0;
//                while (cursor.movePosition(QTextCursor::NextBlock)) {
//                    const ScenarioBlockStyle::Type blockType = ScenarioBlockStyle::forBlock(cursor.block());
//                    if (isScene
//                        && blockType == ScenarioBlockStyle::SceneHeading) {
//                        cursor.movePosition(QTextCursor::PreviousBlock);
//                        break;
//                    } else if (blockType == ScenarioBlockStyle::FolderHeader) {
//                        ++openedFolders;
//                    } else if (blockType == ScenarioBlockStyle::FolderFooter) {
//                        if (openedFolders > 0) {
//                            --openedFolders;
//                        } else {
//                            if (isScene) {
//                                cursor.movePosition(QTextCursor::PreviousBlock);
//                            }
//                            break;
//                        }
//                    }
//                }
//                cursor.movePosition(QTextCursor::EndOfBlock);
//                const QRect bottomCursorRect = cursorRect(cursor);

//                verticalMargin = topCursorRect.height() / 2;
//                colorRectWidth = QFontMetrics(cursor.charFormat().font()).width(".");

//                //
//                // Закрасим область сцены
//                //
//                const QRect noizeRect(QPoint(pageLeft, topCursorRect.top() - verticalMargin),
//                                      QPoint(pageRight, bottomCursorRect.bottom() + verticalMargin));
//                painter.fillRect(noizeRect, noizeColor);
//            }
//        }

//        //
//        // Подсветка строки
//        //
//        if (m_highlightCurrentLine) {
//            const QRect cursorR = cursorRect();
//            const QRect highlightRect(0, cursorR.top(), viewport()->width(), cursorR.height());
//            QColor lineColor = palette().highlight().color().lighter();
//            lineColor.setAlpha(40);
//            painter.fillRect(highlightRect, lineColor);
//        }

//        //
//        // Подсветка дифов
//        //
//        {
//            QTextBlock block = topBlock;
//            ScriptTextCursor cursor(document());
//            while (block.isValid() && block != bottomBlock) {
//                TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(block.userData());
//                if (blockInfo != nullptr
//                    && blockInfo->diffColor().isValid()) {
//                    cursor.setPosition(block.position());
//                    const QRect topCursorRect = cursorRect(cursor);
//                    cursor.movePosition(QTextCursor::EndOfBlock);
//                    const QRect bottomCursorRect = cursorRect(cursor);
//                    //
//                    const int topMargin = std::max(block.blockFormat().topMargin(), block.previous().blockFormat().bottomMargin()) / 2;
//                    const int bottomMargin = std::max(block.blockFormat().bottomMargin(), block.next().blockFormat().topMargin()) / 2;
//                    const QRect diffRect(QPoint(pageLeft, topCursorRect.top() - topMargin),
//                                         QPoint(pageRight, bottomCursorRect.bottom() + bottomMargin));
//                    painter.fillRect(diffRect, blockInfo->diffColor());
//                }

//                block = block.next();
//            }
//        }
//    }

    BaseTextEdit::paintEvent(_event);

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

            ScreenplayTextCursor cursor(document());
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
//                    TextBlockInfo* blockInfo = dynamic_cast<TextBlockInfo*>(block.userData());
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
//                        //
//                        // Прорисовка номеров сцен, если необходимо
//                        //
//                        if (m_showSceneNumbers
//                            && blockType == ScenarioBlockStyle::SceneHeading) {
//                            //
//                            // Определим номер сцены
//                            //
//                            if (SceneHeadingBlockInfo* info = dynamic_cast<SceneHeadingBlockInfo*>(blockInfo)) {
//                                const QString sceneNumber = m_sceneNumbersPrefix + info->sceneNumber() + ".";

//                                //
//                                // Определим область для отрисовки и выведем номер сцены в редактор
//                                //
//                                QPointF topLeft(isLeftToRight
//                                                ? pageLeft + leftDelta
//                                                : textRight + leftDelta,
//                                                cursorR.top());
//                                QPointF bottomRight(isLeftToRight
//                                                    ? textLeft + leftDelta
//                                                    : pageRight + leftDelta,
//                                                    cursorR.bottom());
//                                QRectF rect(topLeft, bottomRight);
//                                painter.setFont(cursor.charFormat().font());
//                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, sceneNumber);
//                            }
//                        }
//                        //
//                        // Прорисовка номеров реплик, если необходимо
//                        //
//                        if (m_showDialoguesNumbers
//                            && blockType == ScenarioBlockStyle::Character
//                            && (!block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrection)
//                                || block.blockFormat().boolProperty(ScenarioBlockStyle::PropertyIsCorrectionCharacter))) {
//                            //
//                            // Определим номер реплики
//                            //
//                            if (CharacterBlockInfo* info = dynamic_cast<CharacterBlockInfo*>(blockInfo)) {
//                                const QString dialogueNumber = QString::number(info->dialogueNumber()) + ":";

//                                //
//                                // Определим область для отрисовки и выведем номер реплики в редактор
//                                //
//                                painter.setFont(cursor.charFormat().font());
//                                const int numberDelta = painter.fontMetrics().width(dialogueNumber);
//                                QRectF rect;
//                                //
//                                // Если имя персонажа находится не с самого края листа
//                                //
//                                if (block.blockFormat().leftMargin() > numberDelta) {
//                                    //
//                                    // ... то поместим номер реплики внутри текстовой области,
//                                    //     чтобы их было удобно отличать от номеров сцен
//                                    //
//                                    QPointF topLeft(isLeftToRight
//                                                    ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
//                                                    : textRight + leftDelta - spaceBetweenSceneNumberAndText - numberDelta,
//                                                    cursorR.top());
//                                    QPointF bottomRight(isLeftToRight
//                                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText + numberDelta
//                                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText,
//                                                        cursorR.bottom());
//                                    rect = QRectF(topLeft, bottomRight);
//                                }
//                                //
//                                // В противном же случае
//                                //
//                                else {
//                                    //
//                                    // ... позиционируем номера реплик на полях, так же как и номера сцен
//                                    //
//                                    QPointF topLeft(isLeftToRight
//                                                    ? pageLeft + leftDelta
//                                                    : textRight + leftDelta,
//                                                    cursorR.top());
//                                    QPointF bottomRight(isLeftToRight
//                                                        ? textLeft + leftDelta
//                                                        : pageRight + leftDelta,
//                                                        cursorR.bottom());
//                                    rect = QRectF(topLeft, bottomRight);
//                                }
//                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, dialogueNumber);
//                            }
//                        }

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
                                    = painter.fontMetrics().horizontalAdvance(ScreenplayTextCorrector::continuedTerm());
                            const QPoint topLeft = isLeftToRight
                                                   ? cursorREnd.topLeft()
                                                   : cursorREnd.topRight() - QPoint(continuedTermWidth, 0);
                            const QPoint bottomRight = isLeftToRight
                                                       ? cursorREnd.bottomRight() + QPoint(continuedTermWidth, 0)
                                                       : cursorREnd.bottomLeft();
                            const QRect rect(topLeft, bottomRight);
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, ScreenplayTextCorrector::continuedTerm());
                        }
                    }

                    //
                    // Прорисовка перфикса блока
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
                    // Прорисовка постфикса блока
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

    //
    // TODO: Разделить на две колонки можно только диалоги
    //
    QStandardItemModel* model = new QStandardItemModel(menu);
    auto addDocument = new QStandardItem(tr("Add document"));
    addDocument->setData(u8"\uf415", Qt::DecorationRole);
    model->appendRow(addDocument);
    menu->setModel(model);
    connect(menu, &ContextMenu::clicked, this, [this, menu] {
        menu->hideContextMenu();
        ScreenplayTextCursor cursor = textCursor();
        if (cursor.inTable()) {
            d->document.unsplitParagraph(cursor);
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

    return menu;
}

} // namespace Ui

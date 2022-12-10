#include "stageplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/stageplay/text/stageplay_text_corrector.h>
#include <business_layer/document/stageplay/text/stageplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/import/stageplay/stageplay_fountain_importer.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_block_parser.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QCoreApplication>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QRegularExpression>
#include <QScrollBar>
#include <QTextTable>
#include <QTimer>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;

namespace Ui {

class StageplayTextEdit::Implementation
{
public:
    explicit Implementation(StageplayTextEdit* _q);

    const BusinessLayer::StageplayTemplate& stageplayTemplate() const;

    void revertAction(bool previous);


    StageplayTextEdit* q = nullptr;

    QPointer<BusinessLayer::StageplayTextModel> model;
    BusinessLayer::StageplayTextDocument document;

    bool showBlockNumbers = false;
    bool continueBlockNumber = false;

    QVector<Domain::CursorInfo> collaboratorsCursorInfo;
};

StageplayTextEdit::Implementation::Implementation(StageplayTextEdit* _q)
    : q(_q)
{
}

const BusinessLayer::StageplayTemplate& StageplayTextEdit::Implementation::stageplayTemplate() const
{
    const auto currentTemplateId
        = model && model->informationModel() ? model->informationModel()->templateId() : "";
    return TemplatesFacade::stageplayTemplate(currentTemplateId);
}

void StageplayTextEdit::Implementation::revertAction(bool previous)
{
    if (model == nullptr) {
        return;
    }

    const auto lastCursorPosition = q->textCursor().position();
    //
    if (previous) {
        model->undo();
    } else {
        model->redo();
    }
    //
    if (document.characterCount() > lastCursorPosition) {
        auto cursor = q->textCursor();
        cursor.setPosition(lastCursorPosition);
        q->setTextCursorAndKeepScrollBars(cursor);
        q->ensureCursorVisible();

        //
        // При отмене последнего действия позиция курсора могла и не поменяться,
        // но тип параграфа сменился, поэтому перестраховываемся и говорим будто бы
        // сменилась позиция курсора, чтобы обновить состояние панелей
        //
        emit q->cursorPositionChanged();
    }
}


// ****


StageplayTextEdit::StageplayTextEdit(QWidget* _parent)
    : ScriptTextEdit(_parent)
    , d(new Implementation(this))
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setShowPageNumbers(true);
    setShowPageNumberAtFirstPage(false);

    setDocument(&d->document);
    setCapitalizeWords(false);
}

StageplayTextEdit::~StageplayTextEdit() = default;

void StageplayTextEdit::setShowBlockNumbers(bool _show, bool _continue)
{
    d->showBlockNumbers = _show;
    d->continueBlockNumber = _continue;
    update();
}

void StageplayTextEdit::setCorrectionOptions(bool _needToCorrectPageBreaks)
{
    d->document.setCorrectionOptions(_needToCorrectPageBreaks);
}

void StageplayTextEdit::initWithModel(BusinessLayer::StageplayTextModel* _model)
{
    if (d->model) {
        d->model->disconnect(this);
        if (d->model->informationModel()) {
            d->model->informationModel()->disconnect(this);
        }
    }
    d->model = _model;

    //
    // Сбрасываем модель, чтобы не вылезали изменения документа при изменении параметров страницы
    //
    d->document.setModel(nullptr);

    //
    // Обновляем параметры страницы из шаблона
    //
    if (usePageMode()) {
        const auto& currentTemplate = d->stageplayTemplate();
        setPageFormat(currentTemplate.pageSizeId());
        setPageMarginsMm(currentTemplate.pageMargins());
        setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
    }

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний
    // изменений
    //
    d->document.setModel(d->model);

    //
    // Отслеживаем изменения некоторых параметров
    //
    if (d->model && d->model->informationModel()) {
        setHeader(d->model->informationModel()->header());
        setFooter(d->model->informationModel()->footer());

        connect(d->model, &BusinessLayer::StageplayTextModel::dataChanged, this,
                qOverload<>(&StageplayTextEdit::update));
        connect(d->model->informationModel(),
                &BusinessLayer::StageplayInformationModel::headerChanged, this,
                &StageplayTextEdit::setHeader);
        connect(d->model->informationModel(),
                &BusinessLayer::StageplayInformationModel::footerChanged, this,
                &StageplayTextEdit::setFooter);
    }
}

void StageplayTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);

    //
    // Пересчитаем всё, что считается во время выполнения
    //
    if (d->model != nullptr) {
        d->model->updateRuntimeDictionaries();
    }
}

const BusinessLayer::StageplayTemplate& StageplayTextEdit::stageplayTemplate() const
{
    return d->stageplayTemplate();
}

QAbstractItemModel* StageplayTextEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersModel();
}

void StageplayTextEdit::createCharacter(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createCharacter(_name);
}

void StageplayTextEdit::undo()
{
    d->revertAction(true);
}

void StageplayTextEdit::redo()
{
    d->revertAction(false);
}

void StageplayTextEdit::addParagraph(BusinessLayer::TextParagraphType _type)
{
    QString mimeDataToMove;

    //
    // Выводим курсор за пределы таблицы, чтобы вставка происходила за её пределами и не создавались
    // многоуровневые таблицы
    //
    if (BusinessLayer::TextCursor cursor = textCursor(); cursor.inTable()) {
        //
        // Курсор обязательно должен быть во второй колонке
        //
        Q_ASSERT(!cursor.inFirstColumn());

        //
        // Если до конца блока есть текст вырезаем его
        //
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
        if (cursor.hasSelection()) {
            mimeDataToMove = d->document.mimeFromSelection(cursor.selectionInterval().from,
                                                           cursor.selectionInterval().to);
            cursor.removeSelectedText();
        }

        //
        // Выходим из таблицы
        //
        cursor.movePosition(QTextCursor::NextBlock);
        Q_ASSERT(!cursor.inTable());

        setTextCursorForced(cursor);
    }

    //
    // Вставляем параграф на уровне модели
    //
    d->document.addParagraph(_type, textCursor());

    //
    // Если диалоги нужно размещать в таблице, переметим добавляемый диалог в таблицу
    //
    if (d->stageplayTemplate().placeDialoguesInTable()) {
        //
        // Если вставляется персонаж, то разделяем страницу, для добавления реплики
        //
        if (_type == BusinessLayer::TextParagraphType::Character) {
            const auto cursorPosition = textCursor().position();
            d->document.splitParagraph(textCursor());
            auto cursor = textCursor();
            cursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            setTextCursor(cursor);
            cursor.movePosition(QTextCursor::NextBlock);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Dialogue, cursor);
        }
        //
        // Если вставляется реплика, то разделяем страницу и ставим курсор во вторую колонку
        //
        else if (_type == BusinessLayer::TextParagraphType::Dialogue) {
            const auto cursorPosition = textCursor().position();
            d->document.splitParagraph(textCursor());
            auto cursor = textCursor();
            cursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            setTextCursor(cursor);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Character, cursor);
            cursor.movePosition(QTextCursor::NextBlock);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Dialogue, cursor);
            setTextCursor(cursor);
        }
    }

    //
    // Вставляем вырезанные данные
    //
    if (!mimeDataToMove.isEmpty()) {
        d->document.insertFromMime(textCursor().position(), mimeDataToMove);
    }

    emit paragraphTypeChanged();
}

void StageplayTextEdit::setCurrentParagraphType(TextParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    BusinessLayer::TextCursor cursor = textCursor();

    //
    // Меняем тип блока на персонажа
    //
    if (d->stageplayTemplate().placeDialoguesInTable() && _type == TextParagraphType::Character) {
        //
        // Если текущий блок не в таблице, то создаём её и текущий блок помещаем в неё как персонажа
        //
        if (!cursor.inTable()) {
            d->document.setParagraphType(_type, cursor);
            const auto cursorPosition = cursor.position();
            d->document.splitParagraph(cursor);
            auto otherCursor = textCursor();
            otherCursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            setTextCursor(otherCursor);
            otherCursor.movePosition(QTextCursor::NextBlock);
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Dialogue, otherCursor);
        }
        //
        //
        //
        else {
            return;
        }
    }
    //
    // На реплику
    //
    else if (d->stageplayTemplate().placeDialoguesInTable()
             && _type == TextParagraphType::Dialogue) {
        //
        // Если текущий блок не в таблице, то создаём её и текущий блок помещаем в неё как персонажа
        //
        if (!cursor.inTable()) {
            d->document.setParagraphType(_type, cursor);
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            const auto blockMimeData = d->document.mimeFromSelection(
                cursor.selectionInterval().from, cursor.selectionInterval().to);
            cursor.removeSelectedText();

            const auto cursorPosition = cursor.position();
            d->document.splitParagraph(cursor);
            auto otherCursor = textCursor();
            otherCursor.setPosition(cursorPosition + 1); // +1 чтобы войти внутрь таблицы
            d->document.setParagraphType(BusinessLayer::TextParagraphType::Character, otherCursor);
            otherCursor.movePosition(QTextCursor::NextBlock);
            d->document.insertFromMime(otherCursor.position(), blockMimeData);
            setTextCursor(otherCursor);
        }
        //
        //
        //
        else {
            return;
        }
    }
    //
    // На любой другой
    //
    else {
        //
        // ... в таблице
        //
        if (cursor.inTable()) {
            //
            // ... если таблица пуста, то удаляем таблицу и применяем оставшемуся блоку новый тип
            //
            const bool skipCurrentBlockEmptynessCheck = true;
            if (cursor.isTableEmpty(skipCurrentBlockEmptynessCheck)) {
                d->document.mergeParagraph(cursor);
                d->document.setParagraphType(_type, cursor);
            }
            //
            // ... если таблица не пуста, ничего не делаем
            //
            else {
                return;
            }
        }
        //
        // ... за пределами таблицы - устанавливаем заданный тип
        //
        else {
            d->document.setParagraphType(_type, cursor);
        }
    }

    //
    // Если вставили папку, то нужно перейти к предыдущему блоку (из футера к хидеру)
    //
    if (_type == TextParagraphType::SequenceHeading) {
        moveCursor(QTextCursor::PreviousBlock);
    }

    emit paragraphTypeChanged();
}

BusinessLayer::TextParagraphType StageplayTextEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

QModelIndex StageplayTextEdit::currentModelIndex() const
{
    if (d->model == nullptr) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto stageplayBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(stageplayBlockData->item());
}

void StageplayTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.model() != d->model) {
        return;
    }

    BusinessLayer::TextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));
    ensureCursorVisible(textCursor);
}

int StageplayTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void StageplayTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                      const QString& _comment)
{
    BusinessLayer::TextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, cursor);
}

void StageplayTextEdit::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->collaboratorsCursorInfo = _cursors;

    update();
}

void StageplayTextEdit::keyPressEvent(QKeyEvent* _event)
{
    if (isReadOnly()) {
        ScriptTextEdit::keyPressEvent(_event);
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
            ScriptTextEdit::keyPressEvent(_event);
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

bool StageplayTextEdit::keyPressEventReimpl(QKeyEvent* _event)
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
        BusinessLayer::TextCursor cursor = textCursor();
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
    // Обрабатываем в базовом классе
    //
    else {
        isEventHandled = ScriptTextEdit::keyPressEventReimpl(_event);
    }

    return isEventHandled;
}

void StageplayTextEdit::paintEvent(QPaintEvent* _event)
{
    ScriptTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight = viewport()->width();
    const qreal spaceBetweenSceneNumberAndText = 10 * Ui::DesignSystem::scaleFactor();
    const qreal textLeft = pageLeft - (isLeftToRight ? 0 : horizontalScrollBar()->maximum())
        + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight + (isLeftToRight ? horizontalScrollBar()->maximum() : 0)
        - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScrollBar()->value();
    //    int colorRectWidth = 0;
    qreal verticalMargin = 0;
    const qreal splitterX = leftDelta + textLeft
        + (textRight - textLeft) * d->stageplayTemplate().leftHalfOfPageWidthPercents() / 100;


    //
    // Определим начальный блок на экране
    //
    QTextBlock topBlock = document()->lastBlock();
    {
        const auto topCursor = cursorForPositionReimpl(viewport()->mapFromParent(QPoint(0, 0)));
        if (topBlock.blockNumber() > topCursor.block().blockNumber()) {
            topBlock = topCursor.block();
        }
    }
    //
    // ... идём до начала сцены
    //
    while (TextBlockStyle::forBlock(topBlock) != TextParagraphType::SceneHeading
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::SequenceHeading
           && topBlock != document()->firstBlock()) {
        topBlock = topBlock.previous();
    }

    //
    // Определим последний блок на экране
    //
    QTextBlock bottomBlock = document()->firstBlock();
    {
        const auto bottomCursor
            = cursorForPositionReimpl(viewport()->mapFromParent(QPoint(0, viewport()->height())));
        if (bottomBlock.blockNumber() < bottomCursor.block().blockNumber()) {
            bottomBlock = bottomCursor.block();
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
        BusinessLayer::TextCursor bottomCursor(document());
        bottomCursor.setPosition(bottomBlock.position());
        while (bottomCursor.inTable() && bottomCursor.movePosition(QTextCursor::NextBlock)) {
            bottomBlock = bottomCursor.block();
        }
    }

    //
    // Прорисовка дополнительных элементов редактора
    //
    QPainter painter(viewport());

    //
    // Декорации текста
    //
    {
        clipPageDecorationRegions(&painter);

        //
        // Проходим блоки на экране и декорируем их
        //
        QTextBlock block = topBlock;
        const QRectF viewportGeometry = viewport()->geometry();
        int lastSceneBlockBottom = 0;
        QColor lastSceneColor;
        QColor lastCharacterColor;
        int lastCharacterColorWithNumberRectBottom = 0;

        auto setPainterPen = [&painter, &block, this](const QColor& _color) {
            painter.setPen(ColorHelper::transparent(
                _color,
                1.0
                    - (isFocusCurrentParagraph() && block != textCursor().block()
                           ? Ui::DesignSystem::inactiveTextOpacity()
                           : 0.0)));
        };

        BusinessLayer::TextCursor cursor(document());
        while (block.isValid() && block != bottomBlock) {
            //
            // Стиль текущего блока
            //
            const auto blockType = TextBlockStyle::forBlock(block);

            //
            // Пропускаем невидимые блоки
            //
            if (!block.isVisible()) {
                block = block.next();
                continue;
            }

            cursor.setPosition(block.position());
            const QRect cursorR = cursorRect(cursor);
            cursor.movePosition(QTextCursor::EndOfBlock);
            const QRect cursorREnd = cursorRect(cursor);
            //
            verticalMargin = cursorR.height() / 2;

            //
            // Определим цвет сцены
            //
            if (blockType == TextParagraphType::SceneHeading
                || blockType == TextParagraphType::SequenceHeading) {
                lastSceneBlockBottom = cursorR.top();
                lastSceneColor = d->document.itemColor(block);
            }

            //
            // Нарисуем цвет сцены
            //
            if (lastSceneColor.isValid()) {
                const QPointF topLeft(isLeftToRight
                                          ? textRight + leftDelta + DesignSystem::layout().px8()
                                          : (textLeft - DesignSystem::layout().px4() + leftDelta),
                                      lastSceneBlockBottom - verticalMargin);
                const QPointF bottomRight(isLeftToRight
                                              ? textRight + DesignSystem::layout().px4() + leftDelta
                                              : textLeft + leftDelta,
                                          cursorREnd.bottom() + verticalMargin);
                const QRectF rect(topLeft, bottomRight);
                painter.fillRect(rect, lastSceneColor);
            }

            //
            // Определим цвет персонажа
            //
            if (blockType == TextParagraphType::Character && d->model
                && d->model->charactersModel() != nullptr) {
                lastCharacterColor = QColor();
                lastCharacterColorWithNumberRectBottom = 0;
                const QString characterName
                    = BusinessLayer::StageplayCharacterParser::name(block.text());
                if (auto character = d->model->character(characterName)) {
                    if (character->color().isValid()) {
                        lastCharacterColor = character->color();
                    }
                }
            } else if (blockType != TextParagraphType::Parenthetical
                       && blockType != TextParagraphType::Dialogue
                       && blockType != TextParagraphType::Lyrics) {
                lastCharacterColor = QColor();
            }

            //
            // Нарисуем цвет персонажа
            //
            if (lastCharacterColor.isValid()) {
                QRectF colorRect;
                //
                // ... если у стиля персонажа есть пустое пространство слева, то
                //     поместим цвет реплики внутри текстовой области
                //
                if (d->stageplayTemplate()
                        .paragraphStyle(TextParagraphType::Character)
                        .marginsOnHalfPage()
                        .left()
                    > 0) {
                    QPointF topLeft(isLeftToRight
                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                            + DesignSystem::layout().px4()
                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                    cursorR.top());
                    const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                              cursorREnd.bottom());
                    colorRect = QRectF(topLeft, bottomRight);
                }
                //
                // ... если нет, то рисуем на полях
                //
                else {
                    const QPointF topLeft(
                        isLeftToRight ? (textLeft + leftDelta - DesignSystem::layout().px16())
                                      : (pageRight + leftDelta + DesignSystem::layout().px4()),
                        cursorR.top());
                    const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                              cursorREnd.bottom());
                    colorRect = QRectF(topLeft, bottomRight);
                }

                const auto isBlockCharacterWithNumber
                    = blockType == TextParagraphType::Character && d->showBlockNumbers;
                if (isBlockCharacterWithNumber) {
                    lastCharacterColorWithNumberRectBottom = colorRect.bottom();
                } else {
                    if (lastCharacterColorWithNumberRectBottom > colorRect.top()) {
                        colorRect.setTop(lastCharacterColorWithNumberRectBottom);
                    }
                    painter.fillRect(colorRect, lastCharacterColor);
                }
            }

            //
            // Курсор на экране
            //
            // ... ниже верхней границы
            if ((cursorR.top() > 0 || cursorR.bottom() > 0)
                // ... и выше нижней
                && cursorR.top() < viewportGeometry.bottom()) {

                //
                // Прорисовка закладок
                //
                const auto bookmark = d->document.bookmark(block);
                if (bookmark.isValid()) {
                    setPainterPen(bookmark.color);
                    painter.setFont(DesignSystem::font().iconsForEditors());

                    //
                    // Определим область для отрисовки
                    //
                    QPointF topLeft(isLeftToRight
                                        ? (pageLeft + leftDelta
                                           + Ui::DesignSystem::card().shadowMargins().left())
                                        : (textRight + leftDelta),
                                    cursorR.top());
                    QPointF bottomRight(isLeftToRight
                                            ? textLeft + leftDelta
                                            : (pageRight + leftDelta
                                               - Ui::DesignSystem::card().shadowMargins().right()),
                                        cursorR.bottom());
                    QRectF rect(topLeft, bottomRight);
                    const auto yDelta = Ui::DesignSystem::layout().px(32) - rect.height() / 2.0;
                    //
                    // корректируем размер области, чтобы получить квадрат для отрисовки иконки
                    // закладки
                    //
                    if (yDelta > 0) {
                        rect.adjust(0, -yDelta, 0, yDelta);
                    }
                    if (isLeftToRight) {
                        rect.setWidth(rect.height());
                    } else {
                        rect.setLeft(rect.right() - rect.height());
                    }
                    painter.fillRect(rect,
                                     ColorHelper::transparent(
                                         bookmark.color, Ui::DesignSystem::elevationEndOpacity()));
                    painter.drawText(rect, Qt::AlignCenter, u8"\U000F00C0");
                }

                //
                // Прорисовка тайтлов блоков
                //
                const auto blockStyle = d->stageplayTemplate().paragraphStyle(blockType);
                if (blockStyle.isTitleVisible()) {
                    setPainterPen(palette().text().color());
                    painter.setFont(cursor.charFormat().font());

                    //
                    // Определим область для отрисовки (отступы используем от стиля персонажа)
                    //
                    const auto characterStyle
                        = d->stageplayTemplate().paragraphStyle(TextParagraphType::Character);
                    const QPointF topLeft(
                        isLeftToRight ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                + characterStyle.blockFormat(true).leftMargin()
                                      : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                        cursorR.top());
                    const QPointF bottomRight(
                        isLeftToRight ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                + block.blockFormat().leftMargin()
                                      : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                        cursorR.bottom());
                    const auto rect = QRectF(topLeft, bottomRight);
                    QString space;
                    space.fill(' ', 100);
                    painter.drawText(
                        rect, Qt::AlignLeft | Qt::AlignTop,
                        QString("%1:%2").arg(!blockStyle.title().isEmpty()
                                                 ? blockStyle.title()
                                                 : BusinessLayer::textParagraphTitle(blockType),
                                             space));
                    if (lastCharacterColor.isValid()) {
                        setPainterPen(palette().text().color());
                    }
                }

                //
                // Прорисовка декораций пустой строки
                //
                if (!block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                    && blockType != TextParagraphType::PageSplitter
                    && block.text().simplified().isEmpty()) {
                    //
                    // Настроим цвет
                    //
                    setPainterPen(ColorHelper::transparent(
                        palette().text().color(), Ui::DesignSystem::inactiveItemOpacity()));

                    //
                    // Для пустого футера рисуем плейсхолдер
                    //
                    if (blockType == TextParagraphType::SequenceFooter) {
                        painter.setFont(block.charFormat().font());

                        //
                        // Ищем открывающий блок папки
                        //
                        auto headerBlock = block.previous();
                        int openedFolders = 0;
                        while (headerBlock.isValid()) {
                            const auto headerBlockType = TextBlockStyle::forBlock(headerBlock);
                            if (headerBlockType == TextParagraphType::SequenceHeading) {
                                if (openedFolders > 0) {
                                    --openedFolders;
                                } else {
                                    break;
                                }
                            } else if (headerBlockType == TextParagraphType::SequenceFooter) {
                                ++openedFolders;
                            }

                            headerBlock = headerBlock.previous();
                        }

                        //
                        // Определим область для отрисовки плейсхолдера
                        //
                        const auto placeholderText = QString("%1 %2").arg(
                            QCoreApplication::translate("KeyProcessingLayer::FolderFooterHandler",
                                                        "END OF"),
                            headerBlock.text());
                        const QPoint topLeft = QPoint(
                            textLeft + leftDelta + spaceBetweenSceneNumberAndText, cursorR.top());
                        const QPoint bottomRight
                            = QPoint(textRight + leftDelta - spaceBetweenSceneNumberAndText,
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
                            const QPointF topLeft(isLeftToRight ? pageLeft + leftDelta
                                                                : textRight + leftDelta,
                                                  cursorR.top());
                            const QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
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
                            const QPointF topLeft(
                                x - TextHelper::fineTextWidthF(emptyLineMark, painter.font()),
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
                    // Прорисовка значков папки
                    //
                    if (blockType == TextParagraphType::SequenceHeading) {
                        setPainterPen(palette().text().color());
                        painter.setFont(DesignSystem::font().iconsForEditors());

                        //
                        // Определим область для отрисовки и выведем номер сцены в редактор в
                        // зависимости от стороны
                        //
                        QPointF topLeft(isLeftToRight ? pageLeft + leftDelta
                                                      : textRight + leftDelta,
                                        cursorR.top());
                        QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                          : pageRight + leftDelta,
                                            cursorR.bottom());
                        QRectF rect(topLeft, bottomRight);
                        const auto yDelta = (TextHelper::fineLineSpacing(cursor.charFormat().font())
                                             - TextHelper::fineLineSpacing(
                                                 DesignSystem::font().iconsForEditors()))
                            / 2;
                        rect.adjust(
                            0, yDelta,
                            -TextHelper::fineTextWidthF(".", cursor.charFormat().font()) / 2, 0);
                        painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, u8"\U000F024B");
                    }
                    //
                    // Прорисовка номеров блоков, если необходимо
                    //
                    if (d->showBlockNumbers
                        && (blockType == TextParagraphType::Dialogue
                            || blockType == TextParagraphType::Sound
                            || blockType == TextParagraphType::Music
                            || blockType == TextParagraphType::Cue)) {
                        //
                        // Определим номер блока
                        //
                        const auto dialogueNumber = d->document.blockNumber(block);
                        if (!dialogueNumber.isEmpty()) {
                            setPainterPen(palette().text().color());
                            QFont font = cursor.charFormat().font();
                            font.setBold(false);
                            font.setUnderline(false);
                            painter.setFont(font);

                            //
                            // Определим область для отрисовки и выведем номер реплики в
                            // редактор
                            //
                            const int numberDelta
                                = TextHelper::fineTextWidthF(dialogueNumber, painter.font());
                            QRectF numberRect;
                            //
                            // ... если у стиля персонажа есть пустое пространство слева, то
                            //     поместим номер реплики внутри текстовой области
                            //
                            if (d->stageplayTemplate()
                                    .paragraphStyle(TextParagraphType::Character)
                                    .marginsOnHalfPage()
                                    .left()
                                > 0) {
                                const QPointF topLeft(
                                    isLeftToRight
                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText
                                            - numberDelta,
                                    cursorR.top());
                                const QPointF bottomRight(
                                    isLeftToRight
                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                            + numberDelta
                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                    cursorR.bottom());
                                numberRect = QRectF(topLeft, bottomRight);
                            }
                            //
                            // ... если нет, то рисуем на полях
                            //
                            else {
                                const QPointF topLeft(isLeftToRight ? pageLeft + leftDelta
                                                                    : textRight + leftDelta,
                                                      cursorR.top());
                                const QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                                        : pageRight + leftDelta,
                                                          cursorR.bottom());
                                numberRect = QRectF(topLeft, bottomRight);
                            }

                            if (lastCharacterColor.isValid()) {
                                setPainterPen(lastCharacterColor);
                            }
                            painter.drawText(numberRect, Qt::AlignRight | Qt::AlignTop,
                                             dialogueNumber);
                            if (lastCharacterColor.isValid()) {
                                setPainterPen(palette().text().color());
                            }
                        }
                    }
                }
            }

            //
            // Прорисовка префикса/постфикса для блока текста, если это не пустая декорация
            //
            if (!block.text().isEmpty()
                || !block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
                setPainterPen(palette().text().color());
                painter.setFont(block.charFormat().font());
                //
                // ... префикс
                //
                if (block.charFormat().hasProperty(TextBlockStyle::PropertyPrefix)) {
                    const auto prefix
                        = block.charFormat().stringProperty(TextBlockStyle::PropertyPrefix);
                    const QPoint topLeft = block.text().isRightToLeft()
                        ? QPoint(cursorREnd.left()
                                     - TextHelper::fineTextWidthF(prefix, painter.font()),
                                 cursorREnd.top())
                        : QPoint(cursorR.left()
                                     - TextHelper::fineTextWidthF(prefix, painter.font()),
                                 cursorR.top());
                    const QPoint bottomRight = block.text().isRightToLeft()
                        ? QPoint(cursorREnd.left(), cursorREnd.bottom())
                        : QPoint(cursorR.left(), cursorR.bottom());
                    const QRect rect(topLeft, bottomRight);
                    painter.drawText(rect, Qt::AlignLeft | Qt::AlignVCenter, prefix);
                }
                //
                // ... постфикс
                //
                if (block.charFormat().hasProperty(TextBlockStyle::PropertyPostfix)) {
                    const auto postfix
                        = block.charFormat().stringProperty(TextBlockStyle::PropertyPostfix);
                    const QPoint topLeft = block.text().isRightToLeft()
                        ? QPoint(cursorR.left(), cursorR.top())
                        : QPoint(cursorREnd.left(), cursorREnd.top());
                    const QPoint bottomRight = block.text().isRightToLeft()
                        ? QPoint(cursorR.left()
                                     + TextHelper::fineTextWidthF(postfix, painter.font()),
                                 cursorR.bottom())
                        : QPoint(cursorREnd.left()
                                     + TextHelper::fineTextWidthF(postfix, painter.font()),
                                 cursorREnd.bottom());
                    const QRect rect(topLeft, bottomRight);
                    painter.drawText(rect, Qt::AlignRight | Qt::AlignVCenter, postfix);
                }
            }

            lastSceneBlockBottom = cursorREnd.bottom();

            block = block.next();
        }
    }

    //
    // Курсоры соавторов
    //
    if (!d->collaboratorsCursorInfo.isEmpty()) {
        for (const auto& cursorInfo : std::as_const(d->collaboratorsCursorInfo)) {
            //
            // Пропускаем курсоры, которые находятся за пределами экрана
            //
            const auto cursorPosition = cursorInfo.cursorData.toInt();
            if (bottomBlock.isValid()
                && (cursorPosition < topBlock.position()
                    || cursorPosition > (bottomBlock.position() + bottomBlock.length()))) {
                continue;
            }


            QTextCursor cursor(document());
            cursor.setPosition(cursorPosition);
            const auto cursorR = cursorRect(cursor).adjusted(0, 0, 1, 0);

            const auto backgroundColor = ColorHelper::forText(cursorInfo.name);

            //
            // ... рисуем его
            //
            painter.fillRect(cursorR, backgroundColor);

            //
            // ... выводим имя соавтора
            //
            painter.setFont(DesignSystem::font().subtitle2());
            const QRect usernameRect(cursorR.left() - Ui::DesignSystem::layout().px4(),
                                     cursorR.top() - Ui::DesignSystem::layout().px24(),
                                     TextHelper::fineTextWidth(cursorInfo.name, painter.font())
                                         + Ui::DesignSystem::layout().px12(),
                                     Ui::DesignSystem::layout().px24());
            painter.setPen(Qt::NoPen);
            painter.setBrush(backgroundColor);
            painter.drawRoundedRect(usernameRect, Ui::DesignSystem::button().borderRadius(),
                                    Ui::DesignSystem::button().borderRadius());
            painter.setPen(ColorHelper::contrasted(backgroundColor));
            painter.drawText(usernameRect, Qt::AlignCenter, cursorInfo.name);
        }
    }
}

ContextMenu* StageplayTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    //
    // Сначала нужно создать контекстное меню в базовом классе, т.к. в этот момент может
    // измениться курсор, который установлен в текстовом редакторе, и использовать его
    //
    auto menu = ScriptTextEdit::createContextMenu(_position, _parent);
    if (isReadOnly() || (!textCursor().hasSelection() && isMispelledWordUnderCursor(_position))) {
        return menu;
    }

    const BusinessLayer::TextCursor cursor = textCursor();

    //
    // Работа с закладками
    //
    auto bookmarkAction = new QAction(this);
    bookmarkAction->setText(tr("Bookmark"));
    bookmarkAction->setIconText(u8"\U000F00C3");
    if (!d->document.bookmark(cursor.block()).isValid()) {
        auto createBookmark = new QAction(bookmarkAction);
        createBookmark->setText(tr("Add"));
        createBookmark->setIconText(u8"\U000F00C4");
        connect(createBookmark, &QAction::triggered, this,
                &StageplayTextEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this, &StageplayTextEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &StageplayTextEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this, &StageplayTextEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(bookmarkAction);
    menu->setActions(actions);

    return menu;
}

bool StageplayTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* StageplayTextEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    BusinessLayer::TextCursor cursor = textCursor();
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
                            ? TextHelper::smartToUpper(cursor.selectedText()).toUtf8()
                            : cursor.selectedText().toUtf8());
        } while (cursor.position() < textCursor().selectionEnd() && !cursor.atEnd()
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

void StageplayTextEdit::insertFromMimeData(const QMimeData* _source)
{
    if (isReadOnly()) {
        return;
    }

    //
    // Удаляем выделенный текст
    //
    BusinessLayer::TextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.removeCharacters(this);
    }

    //
    // Если в моменте входа мы в состоянии редактирования (такое возможно в момент дропа),
    // то запомним это состояние, чтобы восстановить после дропа, а для вставки важно,
    // чтобы режим редактирования был выключен, т.к. данные будут загружаться через модель
    //
    const bool wasInEditBlock = cursor.isInEditBlock();
    if (wasInEditBlock) {
        cursor.endEditBlock();
    }

    //
    // Вставляем сценарий из майм-данных
    //
    QString textToInsert;

    //
    // Если вставляются данные в сценарном формате, то вставляем как положено
    //
    const int invalidPosition = -1;
    int removeCharacterAtPosition = invalidPosition;
    if (_source->formats().contains(d->model->mimeTypes().constFirst())) {
        textToInsert = _source->data(d->model->mimeTypes().constFirst());
    }
    //
    // Если простой текст
    //
    else if (_source->hasText()) {
        const auto text = _source->text();

        //
        // ... если в тексте всего одна строка и вставка происходит в пустой абзац, то вставим в
        // него пробел, чтобы его стиль не изменился, а сам текст будем вставлять в начало абзаца
        //
        if (!text.contains('\n') && cursor.block().text().isEmpty()) {
            removeCharacterAtPosition = cursor.position();
            cursor.insertText(" ");
            setTextCursor(cursor);
        }

        //
        // ... если строк несколько, то вставляем его, импортировав с фонтана
        // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
        //       не воспринимался как титульная страница
        //
        BusinessLayer::StageplayFountainImporter fountainImporter;
        textToInsert = fountainImporter.importStageplay("\n" + text).text;
    }

    //
    // Собственно вставка данных
    //
    auto cursorPosition = d->document.insertFromMime(textCursor().position(), textToInsert);

    //
    // Удалим лишний пробел, который вставляли
    //
    if (removeCharacterAtPosition != invalidPosition) {
        cursor.setPosition(removeCharacterAtPosition);
        cursor.deleteChar();
        if (removeCharacterAtPosition < cursorPosition) {
            --cursorPosition;
        }
    }

    //
    // Восстанавливаем режим редактирования, если нужно
    //
    if (wasInEditBlock) {
        cursor.beginEditBlock();
    }

    //
    // Позиционируем курсор
    //
    if (cursorPosition >= 0) {
        cursor.setPosition(cursorPosition);
        setTextCursor(cursor);
    }
}

void StageplayTextEdit::dropEvent(QDropEvent* _event)
{
    //
    // Если в момент вставки было выделение
    //
    if (textCursor().hasSelection()) {
        BusinessLayer::TextCursor cursor = textCursor();
        //
        // ... и это перемещение содержимого внутри редактора
        //
        if (_event->source() == this) {
            //
            // ... то удалим выделенный текст
            //
            cursor.removeCharacters(this);
        }
        //
        // ... а если контент заносят снаружи
        //
        else {
            //
            // ... то очистим выделение, чтобы оставить контент
            //
            cursor.clearSelection();
        }
    }

    PageTextEdit::dropEvent(_event);
}

} // namespace Ui

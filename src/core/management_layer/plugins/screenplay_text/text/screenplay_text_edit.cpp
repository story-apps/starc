#include "screenplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/screenplay/text/screenplay_text_corrector.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/import/screenplay/screenplay_fountain_importer.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/screenplay/screenplay_dictionaries_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/screenplay_template.h>
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

class ScreenplayTextEdit::Implementation
{
public:
    explicit Implementation(ScreenplayTextEdit* _q);

    /**
     * @brief Текущий шаблон документа
     */
    const BusinessLayer::ScreenplayTemplate& screenplayTemplate() const;

    /**
     * @brief Отменить/повторить последнее действие
     */
    void revertAction(bool previous);

    /**
     * @brief Получить текстовый элемент в текущем курсоре
     */
    BusinessLayer::TextModelItem* currentItem() const;


    ScreenplayTextEdit* q = nullptr;

    QPointer<BusinessLayer::ScreenplayTextModel> model;
    BusinessLayer::ScreenplayTextDocument document;

    bool showSceneNumber = false;
    bool showSceneNumberOnLeft = false;
    bool showSceneNumberOnRight = false;
    bool showDialogueNumber = false;

    QVector<Domain::CursorInfo> collaboratorsCursorInfo;
};

ScreenplayTextEdit::Implementation::Implementation(ScreenplayTextEdit* _q)
    : q(_q)
{
    document.setTreatmentVisible(false);
}

const BusinessLayer::ScreenplayTemplate& ScreenplayTextEdit::Implementation::screenplayTemplate()
    const
{
    const auto currentTemplateId
        = model && model->informationModel() ? model->informationModel()->templateId() : "";
    return TemplatesFacade::screenplayTemplate(currentTemplateId);
}

void ScreenplayTextEdit::Implementation::revertAction(bool previous)
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

BusinessLayer::TextModelItem* ScreenplayTextEdit::Implementation::currentItem() const
{
    if (model == nullptr) {
        return nullptr;
    }

    auto userData = q->textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto screenplayBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return screenplayBlockData->item();
}


// ****


ScreenplayTextEdit::ScreenplayTextEdit(QWidget* _parent)
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

void ScreenplayTextEdit::setCorrectionOptions(bool _needToCorrectCharactersNames,
                                              bool _needToCorrectPageBreaks)
{
    d->document.setCorrectionOptions(_needToCorrectCharactersNames, _needToCorrectPageBreaks);
}

void ScreenplayTextEdit::initWithModel(BusinessLayer::ScreenplayTextModel* _model)
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
        const auto& currentTemplate = d->screenplayTemplate();
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

        connect(d->model, &BusinessLayer::ScreenplayTextModel::dataChanged, this,
                qOverload<>(&ScreenplayTextEdit::update));
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::headerChanged, this,
                &ScreenplayTextEdit::setHeader);
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::footerChanged, this,
                &ScreenplayTextEdit::setFooter);
    }
    //
    // Добавляем словарные термины в список исключений для проверки орфографии
    //
    if (d->model && d->model->dictionariesModel()) {
        for (const auto& sceneIntro : d->model->dictionariesModel()->sceneIntros()) {
            ignoreWord(sceneIntro.endsWith('.') ? sceneIntro.chopped(1) : sceneIntro);
        }
    }
}

void ScreenplayTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);

    //
    // Пересчитаем всё, что считается во время выполнения
    //
    if (d->model != nullptr) {
        d->model->recalculateDuration();
        d->model->updateRuntimeDictionaries();
    }
}

const BusinessLayer::ScreenplayTemplate& ScreenplayTextEdit::screenplayTemplate() const
{
    return d->screenplayTemplate();
}

BusinessLayer::ScreenplayDictionariesModel* ScreenplayTextEdit::dictionaries() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->dictionariesModel();
}

QAbstractItemModel* ScreenplayTextEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersModel();
}

void ScreenplayTextEdit::createCharacter(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createCharacter(_name);
}

QAbstractItemModel* ScreenplayTextEdit::locations() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->locationsModel();
}

void ScreenplayTextEdit::createLocation(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createLocation(_name);
}

void ScreenplayTextEdit::undo()
{
    d->revertAction(true);
}

void ScreenplayTextEdit::redo()
{
    d->revertAction(false);
}

void ScreenplayTextEdit::addParagraph(TextParagraphType _type)
{
    //
    // При попытке вставки папки или сцены в таблицу, подменяем тип на описание действия
    //
    if (_type == TextParagraphType::ActHeading || _type == TextParagraphType::SequenceHeading
        || _type == TextParagraphType::SceneHeading) {
        BusinessLayer::TextCursor cursor = textCursor();
        if (cursor.inTable()) {
            _type = TextParagraphType::Action;
        }
    }

    d->document.addParagraph(_type, textCursor());

    emit paragraphTypeChanged();
}

void ScreenplayTextEdit::setCurrentParagraphType(TextParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    BusinessLayer::TextCursor cursor = textCursor();

    //
    // Если тип блока меняется на заголовок папки илисцены, но это единственный текстовый блок бита,
    // то добавим сцену отдельным блоком после него, т.к. бит не может включать в себя сцену
    //
    if (_type == TextParagraphType::ActHeading || _type == TextParagraphType::SequenceHeading
        || _type == TextParagraphType::SceneHeading) {
        //
        // Внтури таблицы нельзя создавать папки и сцены
        //
        if (cursor.inTable()) {
            return;
        }

        const auto item = d->currentItem();
        Q_ASSERT(item);
        if (item->parent() != nullptr
            && item->parent()->type() == BusinessLayer::TextModelItemType::Group) {
            const auto groupItem = static_cast<BusinessLayer::TextModelGroupItem*>(item->parent());
            //
            // 2, т.к. у нас всегда есть ещё заголовок самого бита
            //
            if (groupItem->groupType() == BusinessLayer::TextGroupType::Beat
                && groupItem->childCount() == 2) {
                addParagraph(_type);
                return;
            }
        }
    }

    d->document.setParagraphType(_type, textCursor());

    //
    // Если вставили папку, то нужно перейти к предыдущему блоку (из футера к хидеру)
    //
    if (_type == TextParagraphType::ActHeading || _type == TextParagraphType::SequenceHeading) {
        moveCursor(QTextCursor::PreviousBlock);
    }

    emit paragraphTypeChanged();
}

BusinessLayer::TextParagraphType ScreenplayTextEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

void ScreenplayTextEdit::setTextCursorAndKeepScrollBars(const QTextCursor& _cursor)
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

    auto screenplayBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(screenplayBlockData->item());
}

void ScreenplayTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    BusinessLayer::TextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));

    //
    // В кейсе с битами мы попадаем на невидимый блок, но интересует нас следующий за ним -
    // первый параграф бита
    //
    if (!textCursor.block().isVisible()) {
        textCursor.movePosition(BusinessLayer::TextCursor::NextBlock);
    }

    ensureCursorVisible(textCursor);
}

int ScreenplayTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void ScreenplayTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                       const QString& _comment)
{
    BusinessLayer::TextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, cursor);
}

void ScreenplayTextEdit::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->collaboratorsCursorInfo = _cursors;

    update();
}

void ScreenplayTextEdit::keyPressEvent(QKeyEvent* _event)
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
    if (currentCharFormat.boolProperty(TextBlockStyle::PropertyIsFirstUppercase)
        && cursorBackwardText != " " && cursorBackwardText == _eventText
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

    return ScriptTextEdit::updateEnteredText(_eventText);
}

void ScreenplayTextEdit::paintEvent(QPaintEvent* _event)
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
        + (textRight - textLeft) * d->screenplayTemplate().leftHalfOfPageWidthPercents() / 100;


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
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ActHeading
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
        bool isLastBlockSceneHeadingWithNumberAtRight = false;
        struct {
            bool isPainted = true;
            QString text;
            QColor color;
        } lastBeat;
        QColor lastCharacterColor;
        //
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
                //
                // ... но запоминаем информацию о бите
                //
                if (blockType == TextParagraphType::BeatHeading) {
                    lastBeat.isPainted = false;
                    lastBeat.text = block.text();
                    lastBeat.color = d->document.itemColor(block);
                    if (!lastBeat.color.isValid()) {
                        lastBeat.color = palette().text().color();
                    }
                }

                block = block.next();
                continue;
            }

            //
            // Если информация о бите была нарисонавана, затрём её
            //
            if (lastBeat.isPainted) {
                lastBeat = {};
            } else {
                lastBeat.isPainted = true;
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
                || blockType == TextParagraphType::SequenceHeading
                || blockType == TextParagraphType::ActHeading) {
                lastSceneBlockBottom = cursorR.top();
                lastSceneColor = d->document.itemColor(block);
            }

            //
            // Нарисуем цвет сцены
            //
            if (lastSceneColor.isValid()) {
                const auto isBlockSceneHeadingWithNumberAtRight
                    = blockType == TextParagraphType::SceneHeading && d->showSceneNumber
                    && d->showSceneNumberOnRight;
                if (!isBlockSceneHeadingWithNumberAtRight) {
                    const QPointF topLeft(
                        isLeftToRight ? textRight + leftDelta + DesignSystem::layout().px8()
                                      : (textLeft - DesignSystem::layout().px4() + leftDelta),
                        isLastBlockSceneHeadingWithNumberAtRight
                            ? cursorR.top() - verticalMargin
                            : lastSceneBlockBottom - verticalMargin);
                    const QPointF bottomRight(isLeftToRight ? textRight
                                                      + DesignSystem::layout().px4() + leftDelta
                                                            : textLeft + leftDelta,
                                              cursorREnd.bottom() + verticalMargin);
                    const QRectF rect(topLeft, bottomRight);
                    painter.fillRect(rect, lastSceneColor);
                }

                isLastBlockSceneHeadingWithNumberAtRight = isBlockSceneHeadingWithNumberAtRight;
            }

            //
            // Определим цвет персонажа
            //
            if (blockType == TextParagraphType::Character && d->model
                && d->model->charactersModel() != nullptr) {
                lastCharacterColor = QColor();
                const QString characterName
                    = BusinessLayer::ScreenplayCharacterParser::name(block.text());
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
                const auto isBlockCharacterWithNumber
                    = blockType == TextParagraphType::Character && d->showDialogueNumber;
                if (!isBlockCharacterWithNumber) {
                    QRectF rect;
                    if (cursor.inTable() && cursor.inFirstColumn()) {
                        QPointF topLeft(
                            isLeftToRight ? textLeft + leftDelta - spaceBetweenSceneNumberAndText
                                    + DesignSystem::layout().px4()
                                          : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                            cursorR.top());
                        const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                                  cursorREnd.bottom());
                        rect = QRectF(topLeft, bottomRight);
                    } else if (cursor.inTable() && !cursor.inFirstColumn()) {
                        QPointF topLeft(isLeftToRight ? splitterX - spaceBetweenSceneNumberAndText
                                                + DesignSystem::layout().px4()
                                                      : textRight + leftDelta
                                                - spaceBetweenSceneNumberAndText,
                                        cursorR.top());
                        const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                                  cursorREnd.bottom());
                        rect = QRectF(topLeft, bottomRight);
                    } else {
                        QPointF topLeft(
                            isLeftToRight ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                    + DesignSystem::layout().px4()
                                          : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                            cursorR.top());
                        const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                                  cursorREnd.bottom());
                        rect = QRectF(topLeft, bottomRight);
                    }
                    painter.fillRect(rect, lastCharacterColor);
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
                // Прорисовка декораций пустой строки
                //
                if (!block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)
                    && blockType != TextParagraphType::PageSplitter
                    && block.text().simplified().isEmpty()) {
                    //
                    // Настроим цвет
                    //
                    setPainterPen(ColorHelper::transparent(
                        palette().text().color(), Ui::DesignSystem::inactiveTextOpacity()));

                    //
                    // Для пустого футера рисуем плейсхолдер
                    //
                    if (blockType == TextParagraphType::ActFooter
                        || blockType == TextParagraphType::SequenceFooter) {
                        painter.setFont(block.charFormat().font());

                        //
                        // Ищем открывающий блок папки
                        //
                        auto headerBlock = block.previous();
                        int openedFolders = 0;
                        while (headerBlock.isValid()) {
                            const auto headerBlockType = TextBlockStyle::forBlock(headerBlock);
                            if (headerBlockType == TextParagraphType::ActHeading
                                || headerBlockType == TextParagraphType::SequenceHeading) {
                                if (openedFolders > 0) {
                                    --openedFolders;
                                } else {
                                    break;
                                }
                            } else if (headerBlockType == TextParagraphType::ActFooter
                                       || headerBlockType == TextParagraphType::SequenceFooter) {
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
                    // Исключением является случай пустой строки начала бита - не рисуем индикатор
                    // пустой строки, а рисуем индикатор бита
                    //
                    else if (!lastBeat.color.isValid()) {
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

                    //
                    // Для первого блока бита, если там ещё нет текста, рисуем текст бита
                    //
                    if (!lastBeat.text.isEmpty()) {
                        painter.setFont(block.charFormat().font());

                        //
                        // Определим область для отрисовки плейсхолдера
                        //
                        const QPoint topLeft = QPoint(
                            textLeft + leftDelta + spaceBetweenSceneNumberAndText, cursorR.top());
                        const QPoint bottomRight
                            = QPoint(textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                     cursorR.bottom());
                        const QRect rect(topLeft, bottomRight);
                        painter.drawText(rect, block.blockFormat().alignment(),
                                         painter.fontMetrics().elidedText(
                                             lastBeat.text, Qt::ElideRight, rect.width()));
                    }
                }
                //
                // Прорисовка декораций непустых строк
                //
                else {
                    //
                    // Прорисовка значков папки
                    //
                    if (blockType == TextParagraphType::ActHeading
                        || blockType == TextParagraphType::SequenceHeading) {
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
                        painter.drawText(rect, Qt::AlignRight | Qt::AlignTop,
                                         blockType == TextParagraphType::ActHeading
                                             ? u8"\U000F0253"
                                             : u8"\U000F024B");
                    }
                    //
                    // Прорисовка номеров сцен, если необходимо
                    //
                    if (d->showSceneNumber && blockType == TextParagraphType::SceneHeading) {
                        //
                        // Определим номер сцены
                        //
                        const auto sceneNumber = d->document.sceneNumber(block);
                        if (!sceneNumber.isEmpty()) {
                            setPainterPen(palette().text().color());
                            auto font = cursor.charFormat().font();
                            font.setUnderline(false);
                            painter.setFont(font);

                            //
                            // Определим область для отрисовки и выведем номер сцены в редактор
                            // в зависимости от стороны
                            //
                            if (d->showSceneNumberOnLeft) {
                                QPointF topLeft(isLeftToRight ? pageLeft + leftDelta
                                                              : textRight + leftDelta,
                                                cursorR.top());
                                QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                                  : pageRight + leftDelta,
                                                    cursorR.bottom());
                                QRectF rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, sceneNumber);
                            }
                            if (d->showSceneNumberOnRight) {
                                QPointF topLeft(isLeftToRight ? textRight + leftDelta
                                                              : pageLeft - leftDelta,
                                                cursorR.top());
                                QPointF bottomRight(isLeftToRight ? pageRight
                                                                  : textLeft - leftDelta,
                                                    cursorR.bottom());
                                QRectF rect(topLeft, bottomRight);
                                if (lastSceneColor.isValid()) {
                                    setPainterPen(lastSceneColor);
                                }
                                painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, sceneNumber);
                                if (lastSceneColor.isValid()) {
                                    setPainterPen(palette().text().color());
                                }
                            }
                        }
                    }
                    //
                    // Прорисовка номеров реплик, если необходимо
                    //
                    if (d->showDialogueNumber && blockType == TextParagraphType::Character) {
                        //
                        // Определим номер реплики
                        //
                        const auto dialogueNumber = d->document.dialogueNumber(block);
                        if (!dialogueNumber.isEmpty()) {
                            setPainterPen(palette().text().color());
                            painter.setFont(cursor.charFormat().font());

                            //
                            // Определим область для отрисовки и выведем номер реплики в редактор
                            //
                            // ... в тексте или в первой колоке таблички
                            //
                            QRectF rect;
                            if (!cursor.inTable() || cursor.inFirstColumn()) {
                                const int numberDelta
                                    = TextHelper::fineTextWidthF(dialogueNumber, painter.font());
                                //
                                // ... поместим номер реплики внутри текстовой области,
                                //     чтобы их было удобно отличать от номеров сцен
                                //
                                QPointF topLeft(
                                    isLeftToRight
                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText
                                            - numberDelta,
                                    cursorR.top());
                                QPointF bottomRight(
                                    isLeftToRight
                                        ? textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                            + numberDelta
                                        : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                    cursorR.bottom());
                                rect = QRectF(topLeft, bottomRight);
                            }
                            //
                            // ... во второй колонке таблички
                            //
                            else {
                                const qreal x = splitterX + spaceBetweenSceneNumberAndText
                                    + cursor.currentTable()->format().border();
                                const int numberDelta
                                    = TextHelper::fineTextWidthF(dialogueNumber, painter.font());
                                const QPointF topLeft(x, cursorR.top());
                                const QPointF bottomRight(x + numberDelta, cursorR.bottom());
                                rect = QRectF(topLeft, bottomRight);
                            }

                            if (lastCharacterColor.isValid()) {
                                setPainterPen(lastCharacterColor);
                            }
                            painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, dialogueNumber);
                            if (lastCharacterColor.isValid()) {
                                setPainterPen(palette().text().color());
                            }
                        }
                    }

                    //
                    // Прорисовка автоматических (ПРОД) для реплик
                    //
                    if (blockType == TextParagraphType::Character
                        && block.blockFormat().boolProperty(
                            TextBlockStyle::PropertyIsCharacterContinued)
                        && !block.blockFormat().boolProperty(
                            TextBlockStyle::PropertyIsCorrection)) {
                        setPainterPen(palette().text().color());
                        painter.setFont(cursor.charFormat().font());

                        //
                        // Определим место положение конца имени персонажа
                        //
                        const int continuedTermWidth = TextHelper::fineTextWidthF(
                            BusinessLayer::ScreenplayTextCorrector::continuedTerm(),
                            painter.font());
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
                // Нарисуем цвет бита
                //
                if (lastBeat.color.isValid()) {
                    setPainterPen(lastBeat.color);
                    painter.setFont(DesignSystem::font().iconsForEditors());

                    //
                    // Определим область для отрисовки и выведем номер сцены в редактор в
                    // зависимости от стороны
                    //
                    QPointF topLeft(isLeftToRight ? pageLeft + leftDelta : textRight + leftDelta,
                                    cursorR.top());
                    QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                      : pageRight + leftDelta,
                                        cursorR.bottom());
                    QRectF rect(topLeft, bottomRight);
                    const auto yDelta
                        = (TextHelper::fineLineSpacing(cursor.charFormat().font())
                           - TextHelper::fineLineSpacing(DesignSystem::font().iconsForEditors()))
                        / 2;
                    rect.adjust(0, yDelta,
                                -TextHelper::fineTextWidthF(".", cursor.charFormat().font()) / 2,
                                0);
                    painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, u8"\U000F09DE");
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

ContextMenu* ScreenplayTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
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

    auto splitAction = new QAction(this);
    splitAction->setSeparator(true);
    if (cursor.inTable()) {
        splitAction->setText(tr("Merge paragraph"));
        splitAction->setIconText(u8"\U000f10e7");
    } else {
        splitAction->setText(tr("Split paragraph"));
        splitAction->setIconText(u8"\U000f10e7");

        //
        // Запрещаем разделять некоторые блоки
        //
        const auto blockType = TextBlockStyle::forBlock(cursor.block());
        splitAction->setEnabled(blockType != TextParagraphType::SceneHeading
                                && blockType != TextParagraphType::SceneHeadingShadow
                                && blockType != TextParagraphType::BeatHeading
                                && blockType != TextParagraphType::BeatHeadingShadow
                                && blockType != TextParagraphType::ActHeading
                                && blockType != TextParagraphType::ActFooter
                                && blockType != TextParagraphType::SequenceHeading
                                && blockType != TextParagraphType::SequenceFooter);
    }
    connect(splitAction, &QAction::triggered, this, [this] {
        BusinessLayer::TextCursor cursor = textCursor();
        if (cursor.inTable()) {
            d->document.mergeParagraph(cursor);
        } else {
            d->document.splitParagraph(cursor);

            //
            // После разделения, возвращаемся в первую ячейку таблицы
            //
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::PreviousBlock);
            moveCursor(QTextCursor::EndOfBlock);
        }
    });

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
                &ScreenplayTextEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this,
                &ScreenplayTextEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &ScreenplayTextEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this, &ScreenplayTextEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.prepend(splitAction);
    actions.prepend(bookmarkAction);
    menu->setActions(actions);

    return menu;
}

bool ScreenplayTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* ScreenplayTextEdit::createMimeDataFromSelection() const
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

            //
            // Для текстового представления не копируем невидимые блоки с содержанием текста сцен
            // т.к. пользователи этого не ожидают
            //
            if (!cursor.block().isVisible()) {
                continue;
            }

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
        //
        // При работе со внутренним форматом, копируем все блоки, включая текст сценария,
        // т.к. пользователь может захотеть перенести блоки вырезав и вставив их в другое место
        //
        const auto mime = d->document.mimeFromSelection(selection.from, selection.to);
        mimeData->setData(d->model->mimeTypes().first(), mime.toUtf8());
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

        //
        // Акта и папки не меняем ни на что
        //
        if ((TextBlockStyle::forBlock(cursor) == TextParagraphType::ActHeading
             || TextBlockStyle::forBlock(cursor) == TextParagraphType::ActFooter
             || TextBlockStyle::forBlock(cursor) == TextParagraphType::SequenceHeading
             || TextBlockStyle::forBlock(cursor) == TextParagraphType::SequenceFooter)
            && cursor.block().text().isEmpty()) {
            removeCharacterAtPosition = cursor.position();
            cursor.insertText(" ");
            setTextCursor(cursor);
        }
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
        BusinessLayer::ScreenplayFountainImporter fountainImporter;
        textToInsert = fountainImporter.importScreenplay("\n" + text).text;
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

void ScreenplayTextEdit::dropEvent(QDropEvent* _event)
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

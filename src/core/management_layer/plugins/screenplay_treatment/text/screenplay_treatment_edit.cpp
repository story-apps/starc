#include "screenplay_treatment_edit.h"

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
#include <business_layer/model/screenplay/text/screenplay_text_mime_handler.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/tools/debouncer.h>

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

class ScreenplayTreatmentEdit::Implementation
{
public:
    explicit Implementation(ScreenplayTreatmentEdit* _q);

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


    ScreenplayTreatmentEdit* q = nullptr;

    QPointer<BusinessLayer::ScreenplayTextModel> model;
    BusinessLayer::ScreenplayTextDocument document;

    bool showSceneNumber = false;
    bool showSceneNumberOnLeft = false;
    bool showSceneNumberOnRight = false;
    bool showDialogueNumber = false;

    QVector<Domain::CursorInfo> collaboratorsCursorInfo;
    QVector<Domain::CursorInfo> pendingCollaboratorsCursorInfo;
    Debouncer collaboratorCursorInfoUpdateDebouncer;
};

ScreenplayTreatmentEdit::Implementation::Implementation(ScreenplayTreatmentEdit* _q)
    : q(_q)
    , collaboratorCursorInfoUpdateDebouncer(500)
{
    document.setTreatmentDocument(true);

    connect(&collaboratorCursorInfoUpdateDebouncer, &Debouncer::gotWork, q, [this] {
        std::swap(collaboratorsCursorInfo, pendingCollaboratorsCursorInfo);
        q->update();
    });
}

const BusinessLayer::ScreenplayTemplate& ScreenplayTreatmentEdit::Implementation::
    screenplayTemplate() const
{
    const auto currentTemplateId
        = model && model->informationModel() ? model->informationModel()->templateId() : "";
    return TemplatesFacade::screenplayTemplate(currentTemplateId);
}

void ScreenplayTreatmentEdit::Implementation::revertAction(bool previous)
{
    if (model == nullptr) {
        return;
    }

    auto finalCursorPosition = q->textCursor().position();

    BusinessLayer::ChangeCursor changeCursor;
    if (previous) {
        changeCursor = model->undo();
    } else {
        changeCursor = model->redo();
    }
    //
    if (changeCursor.item != nullptr) {
        const auto item = static_cast<BusinessLayer::TextModelItem*>(changeCursor.item);
        const auto itemIndex = model->indexForItem(item);
        finalCursorPosition = document.itemEndPosition(itemIndex);
        if (changeCursor.position >= 0) {
            finalCursorPosition += changeCursor.position;
        } else if (item->type() == BusinessLayer::TextModelItemType::Text) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            finalCursorPosition += textItem->text().length();
        }
    }

    auto cursor = q->textCursor();
    cursor.setPosition(finalCursorPosition);
    q->setTextCursorAndKeepScrollBars(cursor);
    q->ensureCursorVisible();

    //
    // При отмене/повторе последнего действия позиция курсора могла и не поменяться,
    // но тип параграфа сменился, поэтому перестраховываемся и говорим будто бы
    // сменилась позиция курсора, чтобы обновить состояние панелей
    //
    emit q->cursorPositionChanged();
}

BusinessLayer::TextModelItem* ScreenplayTreatmentEdit::Implementation::currentItem() const
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


ScreenplayTreatmentEdit::ScreenplayTreatmentEdit(QWidget* _parent)
    : ScriptTextEdit(_parent)
    , d(new Implementation(this))
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    setFrameShape(QFrame::NoFrame);
    setShowPageNumbers(true);
    setShowPageNumberAtFirstPage(false);

    setDocument(&d->document);
    setCapitalizeWords(false);


    connect(document(), &QTextDocument::contentsChange, this,
            [this](int _position, int _charsRemoved, int _charsAdded) {
                auto updateCursors = [_position, _charsRemoved,
                                      _charsAdded](QVector<Domain::CursorInfo>& _cursors) {
                    for (auto& collaboratorCursor : _cursors) {
                        int collaboratorCursorPosition = collaboratorCursor.cursorData.toInt();
                        if (collaboratorCursorPosition >= _position) {
                            collaboratorCursorPosition += _charsAdded - _charsRemoved;
                            collaboratorCursor.cursorData
                                = QByteArray::number(collaboratorCursorPosition);
                        }
                    }
                };
                updateCursors(d->collaboratorsCursorInfo);

                d->collaboratorCursorInfoUpdateDebouncer.abortWork();
            });
}

ScreenplayTreatmentEdit::~ScreenplayTreatmentEdit() = default;

void ScreenplayTreatmentEdit::setShowSceneNumber(bool _show, bool _onLeft, bool _onRight)
{
    d->showSceneNumber = _show;
    d->showSceneNumberOnLeft = _onLeft;
    d->showSceneNumberOnRight = _onRight;
    update();
}

void ScreenplayTreatmentEdit::setShowDialogueNumber(bool _show)
{
    d->showDialogueNumber = _show;
    update();
}

void ScreenplayTreatmentEdit::setCorrectionOptions(bool _needToCorrectCharactersNames,
                                                   bool _needToCorrectPageBreaks)
{
    d->document.setCorrectionOptions(_needToCorrectCharactersNames, _needToCorrectPageBreaks);
}

void ScreenplayTreatmentEdit::initWithModel(BusinessLayer::ScreenplayTextModel* _model)
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
                qOverload<>(&ScreenplayTreatmentEdit::update));
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::headerChanged, this,
                &ScreenplayTreatmentEdit::setHeader);
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::footerChanged, this,
                &ScreenplayTreatmentEdit::setFooter);
    }
    //
    // Добавляем словарные термины в список исключений для проверки орфографии
    //
    if (d->model && d->model->dictionariesModel()) {
        for (const auto& sceneIntro : d->model->dictionariesModel()->sceneIntros()) {
            ignoreWord(sceneIntro.endsWith('.') ? sceneIntro.chopped(1) : sceneIntro);
        }
    }

    emit cursorPositionChanged();
}

void ScreenplayTreatmentEdit::reinit()
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

const BusinessLayer::ScreenplayTemplate& ScreenplayTreatmentEdit::screenplayTemplate() const
{
    return d->screenplayTemplate();
}

BusinessLayer::ScreenplayDictionariesModel* ScreenplayTreatmentEdit::dictionaries() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->dictionariesModel();
}

QAbstractItemModel* ScreenplayTreatmentEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersList();
}

void ScreenplayTreatmentEdit::createCharacter(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createCharacter(_name);
}

QAbstractItemModel* ScreenplayTreatmentEdit::locations() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->locationsList();
}

void ScreenplayTreatmentEdit::createLocation(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createLocation(_name);
}

void ScreenplayTreatmentEdit::undo()
{
    d->revertAction(true);
}

void ScreenplayTreatmentEdit::redo()
{
    d->revertAction(false);
}

void ScreenplayTreatmentEdit::addParagraph(TextParagraphType _type)
{
    //
    // При позиционировании курсора в невидимый блок, Qt откидывает прокрутку в самый верх, поэтому
    // перед редактированием документа запомним прокрутку, а после завершения редактирования
    // восстановим значения полос прокрутки
    //
    const auto verticalScrollValue = verticalScrollBar()->value();
    const auto horizontalScrollValue = horizontalScrollBar()->value();

    BusinessLayer::TextCursor cursor = textCursor();
    QString blockEndMime;
    //
    // Если добавление происходит в момент нахождения курсора в блоке бита
    //
    if (TextBlockStyle::forBlock(cursor) == TextParagraphType::BeatHeading) {
        //
        // Если курсор в начале блока, то просто переносим всё содержимое блока вперёд
        //
        if (cursor.positionInBlock() == 0) {
            //
            // ... ничего не делаем
            //
        }
        //
        // В противном случае
        //
        else {
            //
            // Если в середине блока, то вырежем контент идущий до конца блока
            //
            if (cursor.positionInBlock() < cursor.block().text().length() - 1) {
                //
                // Выделяем до конца, плюс захватываем начало следующего блока
                //
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                const auto selection = cursor.selectionInterval();
                blockEndMime = d->document.mimeFromSelection(selection.from, selection.to);
                //
                // Возвращаемся в конец предыдущего блока, чтобы удалять только контент заголовка
                // бита
                //
                cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
                cursor.removeSelectedText();
            }
            //
            // ... и передвигаем курсор для добавления нового блока
            //
            while (cursor.block().next().isValid() && !cursor.block().next().isVisible()) {
                moveCursor(QTextCursor::NextBlock);
                moveCursor(QTextCursor::EndOfBlock);
                cursor = textCursor();
            }
        }
    }

    //
    // Если добавляется бит
    //
    if (_type == TextParagraphType::BeatHeading) {
        //
        // ... и если это разрыв текущего бита, то вставим сохранённый в буфере контент
        //
        if (!blockEndMime.isEmpty()) {
            constexpr int invalidPosition = -1;
            auto removeCharacterAtPosition = invalidPosition;
            if (cursor.block().text().isEmpty()) {
                removeCharacterAtPosition = cursor.position();
                cursor.insertText(" ");
            }

            d->document.insertFromMime(cursor.position(), blockEndMime);

            if (removeCharacterAtPosition != invalidPosition) {
                cursor.setPosition(removeCharacterAtPosition);
                cursor.deleteChar();
            }
        }
        //
        // ... если вставка нового бита, позиционируем его в конце сцены, либо между другими битами
        //
        else {
            //
            // ... передвигаем курсор
            //
            while (cursor.block().next().isValid() && !cursor.block().next().isVisible()) {
                moveCursor(QTextCursor::NextBlock);
                moveCursor(QTextCursor::EndOfBlock);
                cursor = textCursor();
            }
            //
            // ... и добавляем блок бита
            //
            d->document.addParagraph(_type, textCursor());
        }
    }
    //
    // Все остальные блоки просто добавляются в текст
    //
    else {

        //
        // Если изменяется заголовок изолированного элемента, то снимаем изоляцию на время
        // операции, а после изолируем предшествующий текущему элемент, либо его родителя
        //
        const QSet<TextParagraphType> headerTypes = {
            TextParagraphType::SceneHeading,    TextParagraphType::BeatHeading,
            TextParagraphType::SequenceHeading, TextParagraphType::SequenceFooter,
            TextParagraphType::ActHeading,      TextParagraphType::ActFooter,
        };

        const auto currentTypeIsHeader = headerTypes.contains(currentParagraphType());
        const auto targetTypeIsHeader = headerTypes.contains(_type);
        const auto needReisolate = (currentTypeIsHeader || targetTypeIsHeader)
            && d->document.visibleTopLeveLItem().isValid();
        if (needReisolate) {
            d->document.setVisibleTopLevelItem({});
        }

        d->document.addParagraph(_type, textCursor());

        //
        // ... при необходимости восстанавливаем режим изоляции
        //
        if (needReisolate) {
            d->document.setVisibleTopLevelItem(d->document.itemIndex(textCursor().block()));
        }
    }

    emit paragraphTypeChanged();

    //
    // Восстанавливаем значения полос прокрутки
    //
    verticalScrollBar()->setValue(verticalScrollValue);
    horizontalScrollBar()->setValue(horizontalScrollValue);
}

void ScreenplayTreatmentEdit::setCurrentParagraphType(TextParagraphType _type)
{
    const auto currentType = currentParagraphType();
    if (currentType == _type) {
        return;
    }

    //
    // Если изменяется заголовок изолированного элемента, то снимаем изоляцию на время
    // операции, а после изолируем предшествующий текущему элемент, либо его родителя
    //
    const QSet<TextParagraphType> headerTypes = {
        TextParagraphType::SceneHeading,    TextParagraphType::BeatHeading,
        TextParagraphType::SequenceHeading, TextParagraphType::SequenceFooter,
        TextParagraphType::ActHeading,      TextParagraphType::ActFooter,
    };

    const auto currentTypeIsHeader = headerTypes.contains(currentParagraphType());
    const auto targetTypeIsHeader = headerTypes.contains(_type);
    const auto needReisolate = (currentTypeIsHeader || targetTypeIsHeader)
        && d->document.visibleTopLeveLItem().isValid();
    if (needReisolate) {
        d->document.setVisibleTopLevelItem({});
    }

    //
    // Собственно применяем тип блока
    //
    d->document.setParagraphType(_type, textCursor());

    //
    // ... при необходимости восстанавливаем режим изоляции
    //
    if (needReisolate) {
        d->document.setVisibleTopLevelItem(d->document.itemIndex(textCursor().block()));
    }

    //
    // Если вставили папку, то нужно перейти к предыдущему блоку (из футера к хидеру)
    //
    if (_type == TextParagraphType::ActHeading || _type == TextParagraphType::SequenceHeading) {
        moveCursor(QTextCursor::PreviousBlock);
    }

    emit paragraphTypeChanged();
}

BusinessLayer::TextParagraphType ScreenplayTreatmentEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

QModelIndex ScreenplayTreatmentEdit::currentModelIndex() const
{
    if (d->model == nullptr || d->document.isEditTransactionActive()) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto screenplayBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(screenplayBlockData->item());
}

void ScreenplayTreatmentEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.model() != d->model) {
        return;
    }

    BusinessLayer::TextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));
    ensureCursorVisible(textCursor);
}

int ScreenplayTreatmentEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void ScreenplayTreatmentEdit::addReviewMark(const QColor& _textColor,
                                            const QColor& _backgroundColor, const QString& _comment)
{
    BusinessLayer::TextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, cursor);
}

void ScreenplayTreatmentEdit::setCursors(const QVector<Domain::CursorInfo>& _cursors)
{
    d->pendingCollaboratorsCursorInfo = _cursors;
    d->collaboratorCursorInfoUpdateDebouncer.orderWork();
}

void ScreenplayTreatmentEdit::keyPressEvent(QKeyEvent* _event)
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

        updateEnteredText(_event);
    }

    //
    // Обработка
    //
    if (!_event->isAccepted()) {
        handler->handle(_event);
        updateTypewriterScroll();
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

bool ScreenplayTreatmentEdit::keyPressEventReimpl(QKeyEvent* _event)
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
        BusinessLayer::TextCursor cursor = textCursor();
        if (cursor.hasSelection()) {
            copy();
            cursor.removeCharacters(this);
            d->model->saveChanges();
        }
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

void ScreenplayTreatmentEdit::paintEvent(QPaintEvent* _event)
{
    ScriptTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight = viewport()->width();
    const qreal spaceBetweenSceneNumberAndText = 10 * Ui::DesignSystem::scaleFactor();
    ;
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
        bool isLastBlockSceneHeadingWithNumberAtRight = false;

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
                        palette().text().color(), Ui::DesignSystem::inactiveItemOpacity()));

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

                    //
                    // Для заголовка сцены, если он не задан, рисуем название сцены
                    //
                    if (blockType == BusinessLayer::TextParagraphType::SceneHeading) {
                        const auto title = d->document.groupTitle(block);
                        if (!title.isEmpty()) {
                            painter.setFont(block.charFormat().font());
                            const QPoint topLeft
                                = QPoint(textLeft + leftDelta + spaceBetweenSceneNumberAndText,
                                         cursorR.top());
                            const QPoint bottomRight
                                = QPoint(textRight + leftDelta - spaceBetweenSceneNumberAndText,
                                         cursorR.bottom());
                            const QRect rect(topLeft, bottomRight);
                            painter.drawText(rect, block.blockFormat().alignment(),
                                             painter.fontMetrics().elidedText(title, Qt::ElideRight,
                                                                              rect.width()));
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
                }

                //
                // Нарисуем цвет бита
                //
                if (blockType == TextParagraphType::BeatHeading) {
                    const auto beatColor = d->document.itemColor(block);
                    //
                    // В поэпизоднике рисуем только имеющие цвета биты
                    //
                    if (beatColor.isValid()) {
                        setPainterPen(beatColor.isValid() ? beatColor : palette().text().color());
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
                        painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, u8"\U000F09DE");
                    }
                }
            }

            lastSceneBlockBottom = cursorREnd.bottom();

            block = block.next();
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
}

ContextMenu* ScreenplayTreatmentEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
{
    //
    // Сначала нужно создать контекстное меню в базовом классе, т.к. в этот момент может
    // измениться курсор, который установлен в текстовом редакторе, и использовать его
    //
    auto menu = ScriptTextEdit::createContextMenu(_position, _parent);
    if (isReadOnly() || hasSpellingMenu(_position)) {
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
                &ScreenplayTreatmentEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this,
                &ScreenplayTreatmentEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &ScreenplayTreatmentEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this,
            &ScreenplayTreatmentEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(bookmarkAction);
    menu->setActions(actions);

    return menu;
}

bool ScreenplayTreatmentEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* ScreenplayTreatmentEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    BusinessLayer::TextCursor cursor = textCursor();
    auto selection = cursor.selectionInterval();
    //
    // Расширим выделение, чтобы туда попало содержимое битов
    //
    cursor.setPosition(selection.to);
    if (BusinessLayer::TextBlockStyle::forBlock(cursor)
        == BusinessLayer::TextParagraphType::BeatHeading) {
        do {
            cursor.movePosition(QTextCursor::NextBlock);
            cursor.movePosition(QTextCursor::EndOfBlock);

            if (BusinessLayer::TextBlockStyle::forBlock(cursor)
                == BusinessLayer::TextParagraphType::BeatHeading) {
                cursor.movePosition(QTextCursor::PreviousBlock);
                cursor.movePosition(QTextCursor::EndOfBlock);
                break;
            }
        } while (!cursor.atEnd());
        cursor.setPosition(selection.from, QTextCursor::KeepAnchor);
    }
    selection = cursor.selectionInterval();

    //
    // Сформируем в текстовом виде, для вставки наружу
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
        } while (cursor.position() < selection.to && !cursor.atEnd()
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

void ScreenplayTreatmentEdit::insertFromMimeData(const QMimeData* _source)
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
    // Вручную разделяем параграфы, если курсор в середине блока
    //
    if (!cursor.block().text().isEmpty() && cursor.positionInBlock() > 0
        && cursor.positionInBlock() < cursor.block().length()) {
        const auto lastPosition = cursor.position();
        addParagraph(BusinessLayer::TextParagraphType::BeatHeading);
        cursor.setPosition(lastPosition);
    }

    //
    // Переместим курсор в конец бита
    //
    const int invalidPosition = -1;
    int removeCharacterAtPosition = invalidPosition;
    if (BusinessLayer::TextBlockStyle::forBlock(cursor)
        == BusinessLayer::TextParagraphType::BeatHeading) {
        do {
            cursor.movePosition(QTextCursor::NextBlock);
            if (BusinessLayer::TextBlockStyle::forBlock(cursor)
                == BusinessLayer::TextParagraphType::BeatHeading) {
                cursor.movePosition(QTextCursor::PreviousBlock);
                break;
            }
            cursor.movePosition(QTextCursor::EndOfBlock);
        } while (!cursor.atEnd());

        //
        // Если текст блока пуст, то поместим туда пробел, чтобы вставка майм-данных не
        // проглатила этот блок
        //
        if (cursor.block().text().isEmpty()) {
            removeCharacterAtPosition = cursor.position();
            cursor.insertText(" ");
        } else {
            cursor.movePosition(QTextCursor::EndOfBlock);
        }
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
    if (_source->formats().contains(d->model->mimeTypes().constFirst())) {
        textToInsert = _source->data(d->model->mimeTypes().constFirst());
    }
    //
    // Если простой текст
    //
    else if (_source->hasText()) {
        //
        // Если простой текст, то вставляем его, импортировав с фонтана
        // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
        //       не воспринимался как титульная страница
        //
        BusinessLayer::ScreenplayFountainImporter fountainImporter;
        textToInsert = fountainImporter.importScreenplay("\n" + _source->text()).text;
    }

    //
    // Преобразовываем все текстовые блоки в биты
    //
    textToInsert = BusinessLayer::ScreenplayTextMimeHandler::convertTextBlocksToBeats(textToInsert);

    //
    // Собственно вставка данных
    //
    auto cursorPosition = d->document.insertFromMime(cursor.position(), textToInsert);

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

void ScreenplayTreatmentEdit::dropEvent(QDropEvent* _event)
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

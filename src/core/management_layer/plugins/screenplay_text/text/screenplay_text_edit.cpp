#include "screenplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/screenplay/text/screenplay_text_corrector.h>
#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/screenplay/screenplay_export_options.h>
#include <business_layer/export/screenplay/screenplay_fountain_exporter.h>
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
#include <QDir>
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

namespace {
const QLatin1String kMarkdownMimeType("text/markdown");
}

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

    /**
     * @brief Можно ли разделить параграф, в котором установлен курсор
     */
    bool canSplitParagraph(const BusinessLayer::TextCursor& _cursor) const;


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
    document.setTreatmentDocument(false);
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
        auto position = document.itemEndPosition(itemIndex);
        if (changeCursor.position >= 0) {
            position += changeCursor.position;
        } else if (item->type() == BusinessLayer::TextModelItemType::Text) {
            const auto textItem = static_cast<BusinessLayer::TextModelTextItem*>(item);
            position += textItem->text().length();
        }
        auto cursor = q->textCursor();
        cursor.setPosition(position);
        q->setTextCursorAndKeepScrollBars(cursor);
        q->ensureCursorVisible();

        //
        // При отмене/повторе последнего действия позиция курсора могла и не поменяться,
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

bool ScreenplayTextEdit::Implementation::canSplitParagraph(
    const BusinessLayer::TextCursor& _cursor) const
{
    //
    // FIXME: Проверять все параграфы попадающие в выделение
    //
    const auto blockType = TextBlockStyle::forBlock(_cursor.block());
    return blockType != TextParagraphType::SceneHeading
        && blockType != TextParagraphType::SceneHeadingShadow
        && blockType != TextParagraphType::BeatHeading
        && blockType != TextParagraphType::BeatHeadingShadow
        && blockType != TextParagraphType::ActHeading && blockType != TextParagraphType::ActFooter
        && blockType != TextParagraphType::SequenceHeading
        && blockType != TextParagraphType::SequenceFooter;
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

void ScreenplayTextEdit::setBeatsVisible(bool _visible)
{
    d->document.setBeatsVisible(_visible);
}

void ScreenplayTextEdit::addParagraph(TextParagraphType _type)
{
    BusinessLayer::TextCursor cursor = textCursor();

    //
    // При попытке вставки папки или сцены в таблицу, подменяем тип на описание действия
    //
    if (_type == TextParagraphType::ActHeading || _type == TextParagraphType::SequenceHeading
        || _type == TextParagraphType::SceneHeading) {
        if (cursor.inTable()) {
            _type = TextParagraphType::Action;
        }
    }

    //
    // При добавлении сцены/папки/акта в моменте, когда скрыты биты, нужно добавлять новый блок
    // после всех скрытых блоков, идущих за текущим
    //
    if (cursor.atBlockEnd() && !d->document.isBeatsVisible()
        && (_type == TextParagraphType::SceneHeading || _type == TextParagraphType::SequenceFooter
            || _type == TextParagraphType::ActHeading)) {
        while (cursor.block().next().isValid() && !cursor.block().next().isVisible()) {
            moveCursor(QTextCursor::NextBlock);
            moveCursor(QTextCursor::EndOfBlock);
            cursor = textCursor();
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
    QString blockEndMime;

    //
    // Добавляем дополнительную логику в кейсе, когда биты скрыты
    //
    if (!d->document.isBeatsVisible()) {
        const QSet<TextParagraphType> headerTypes = {
            TextParagraphType::SceneHeading,
            TextParagraphType::BeatHeading,
            TextParagraphType::SequenceHeading,
            TextParagraphType::ActHeading,
        };

        const auto currentTypeIsHeader = headerTypes.contains(currentParagraphType());
        const auto targetTypeIsHeader = headerTypes.contains(_type);
        //
        // При изменении блока на сцену/папку/акт, нужно поставить этот блок и остальные, идущие за
        // ним видимые блоки до заголовка, после всех скрытых блоков, идущих за ними
        //
        if (!currentTypeIsHeader && targetTypeIsHeader) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            while (cursor.block().next().isValid() && cursor.block().next().isVisible()
                   && !headerTypes.contains(TextBlockStyle::forBlock(cursor.block().next()))) {
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            }

            //
            // Если дошли до бита в конце текущего элемента
            //
            if (!cursor.atEnd() && !cursor.block().next().isVisible()) {
                //
                // ... то вырезаем весь выделенный контент
                //
                const auto selection = cursor.selectionInterval();
                blockEndMime = d->document.mimeFromSelection(selection.from, selection.to);
                //
                // ... удалим выделение и оставшийся перенос строки
                //
                cursor.removeSelectedText();
                cursor.deletePreviousChar();
                //
                // ... и передвигаем курсор для вставки вырезанной части
                //
                cursor = textCursor();
                while (cursor.block().next().isValid() && !cursor.block().next().isVisible()) {
                    moveCursor(QTextCursor::NextBlock);
                    moveCursor(QTextCursor::EndOfBlock);
                    cursor = textCursor();
                }
                //
                // ... добавляем блок и вставляем данные из буфера обмена
                //
                addParagraph(_type);
                const auto currentBlockPosition = cursor.position();
                d->document.insertFromMime(cursor.position(), blockEndMime);
                cursor.setPosition(currentBlockPosition);
                setTextCursorForced(cursor);
            }
        }
        //
        // А если блок меняется со сцены/папки/акта на другой, то его и его содержимое потенциально
        // нужно перенести над невидимыми блоками идущими перед ним
        //
        else if (currentTypeIsHeader && !targetTypeIsHeader && cursor.block().previous().isValid()
                 && !cursor.block().previous().isVisible()) {
            cursor.movePosition(QTextCursor::StartOfBlock);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            while (cursor.block().next().isValid() && cursor.block().next().isVisible()
                   && !headerTypes.contains(TextBlockStyle::forBlock(cursor.block().next()))) {
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            }

            //
            // Когда дошли до границы видимости текущего элемента
            //
            // ... вырезаем весь выделенный контент
            //
            const auto selection = cursor.selectionInterval();
            blockEndMime = d->document.mimeFromSelection(selection.from, selection.to);
            //
            // ... удалим выделение и оставшийся перенос строки
            //
            cursor.removeSelectedText();
            cursor.deletePreviousChar();
            //
            // ... и передвигаем курсор для вставки вырезанной части
            //
            cursor = textCursor();
            while (cursor.block().isValid() && !cursor.block().isVisible()) {
                moveCursor(QTextCursor::PreviousBlock);
                moveCursor(QTextCursor::EndOfBlock);
                cursor = textCursor();
            }
            //
            // ... добавляем блок и вставляем данные из буфера обмена
            //
            addParagraph(_type);
            const auto currentBlockPosition = cursor.position();
            d->document.insertFromMime(cursor.position(), blockEndMime);
            cursor.setPosition(currentBlockPosition);
            setTextCursorForced(cursor);
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
    if (!_index.isValid() || _index.model() != d->model) {
        return;
    }

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
    // ... разделить/сшить параграф
    //
    else if (_event->key() == Qt::Key_D && _event->modifiers().testFlag(Qt::ControlModifier)) {
        BusinessLayer::TextCursor cursor = textCursor();
        if (d->canSplitParagraph(cursor)) {
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
        }
    }
    //
    // Обрабатываем в базовом классе
    //
    else {
        isEventHandled = ScriptTextEdit::keyPressEventReimpl(_event);
    }

    return isEventHandled;
}

void ScreenplayTextEdit::paintEvent(QPaintEvent* _event)
{
    ScriptTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = Ui::DesignSystem::card().shadowMargins().left();
    const qreal pageRight = viewport()->width() - Ui::DesignSystem::card().shadowMargins().right()
        - Ui::DesignSystem::layout().px8();
    const qreal spaceBetweenSceneNumberAndText = DesignSystem::layout().px24();
    const qreal textLeft = pageLeft - (isLeftToRight ? 0 : horizontalScrollBar()->maximum())
        + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight + (isLeftToRight ? horizontalScrollBar()->maximum() : 0)
        - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScrollBar()->value();
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
        int previousSceneBlockBottom = 0;
        int lastSceneBlockBottom = 0;
        QVector<QColor> lastSceneColors;
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
            // Запоминаем информацию о бите
            //
            if (blockType == TextParagraphType::BeatHeading) {
                lastBeat.isPainted = false;
                lastBeat.text = block.text();
                lastBeat.color = d->document.itemColor(block);
                if (!lastBeat.color.isValid()) {
                    lastBeat.color = palette().text().color();
                }
            }

            //
            // Пропускаем невидимые блоки
            //
            if (!block.isVisible()) {
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
            switch (blockType) {
            case TextParagraphType::SceneHeading:
            case TextParagraphType::SequenceHeading:
            case TextParagraphType::ActHeading:
            case TextParagraphType::SequenceFooter:
            case TextParagraphType::ActFooter: {
                previousSceneBlockBottom = lastSceneBlockBottom;
                lastSceneBlockBottom = cursorR.top();
                lastSceneColors = d->document.itemColors(block);
                lastBeat = {};
                break;
            }
            default: {
                break;
            }
            }

            //
            // Нарисуем цвета сцены
            //
            if (!lastSceneColors.isEmpty()) {
                const QPointF topLeft(isLeftToRight ? pageRight - DesignSystem::layout().px4()
                                                    : pageLeft + leftDelta,
                                      lastSceneBlockBottom - verticalMargin);
                const QPointF bottomRight(
                    isLeftToRight ? pageRight : pageLeft + leftDelta + DesignSystem::layout().px4(),
                    cursorREnd.bottom() + verticalMargin);
                QRectF rect(topLeft, bottomRight);
                for (const auto& color : std::as_const(lastSceneColors)) {
                    if (!color.isValid()) {
                        continue;
                    }

                    auto colorRect = rect;
                    if (color != lastSceneColors.constLast()) {
                        colorRect.setTop(previousSceneBlockBottom);
                    }

                    painter.setPen(Qt::NoPen);
                    painter.setBrush(color);
                    painter.drawRect(colorRect);
                    rect.moveLeft(rect.left() - DesignSystem::layout().px12());
                }
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
                                    + DesignSystem::layout().px16()
                                          : textRight + leftDelta - spaceBetweenSceneNumberAndText,
                            cursorR.top());
                        const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                                  cursorREnd.bottom());
                        rect = QRectF(topLeft, bottomRight);
                    } else if (cursor.inTable() && !cursor.inFirstColumn()) {
                        QPointF topLeft(isLeftToRight ? splitterX - spaceBetweenSceneNumberAndText
                                                + DesignSystem::layout().px16()
                                                      : textRight + leftDelta
                                                - spaceBetweenSceneNumberAndText,
                                        cursorR.top());
                        const QPointF bottomRight(topLeft.x() + DesignSystem::layout().px4(),
                                                  cursorREnd.bottom());
                        rect = QRectF(topLeft, bottomRight);
                    } else {
                        QPointF topLeft(
                            isLeftToRight ? textLeft + leftDelta + DesignSystem::layout().px8()
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
                && cursorR.top() < viewportGeometry.bottom()
                // ... и блок не является декорацией
                && !block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {

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
                    QPointF topLeft(isLeftToRight ? (pageLeft + leftDelta)
                                                  : (textRight + leftDelta),
                                    cursorR.top());
                    QPointF bottomRight(isLeftToRight ? (textLeft + leftDelta)
                                                      : (pageRight + leftDelta),
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
                if (blockType != TextParagraphType::PageSplitter
                    && block.text().simplified().isEmpty()) {
                    //
                    // Определить область, в которой должен быть отрисован текст блока
                    //
                    auto textRect = [textLeft, textRight, leftDelta, spaceBetweenSceneNumberAndText,
                                     cursorR] {
                        const QPoint topLeft
                            = QPoint(textLeft + leftDelta + spaceBetweenSceneNumberAndText
                                         - Ui::DesignSystem::card().shadowMargins().left(),
                                     cursorR.top());
                        const QPoint bottomRight
                            = QPoint(textRight + leftDelta - spaceBetweenSceneNumberAndText
                                         + Ui::DesignSystem::card().shadowMargins().right(),
                                     cursorR.bottom());
                        return QRect(topLeft, bottomRight);
                    };

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
                        const auto rect = textRect();
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
                    // Для заголовка сцены, если он не задан, рисуем название сцены
                    //
                    if (blockType == BusinessLayer::TextParagraphType::SceneHeading) {
                        const auto title = d->document.groupTitle(block);
                        if (!title.isEmpty()) {
                            painter.setFont(block.charFormat().font());
                            const auto rect = textRect();
                            painter.drawText(rect, block.blockFormat().alignment(),
                                             painter.fontMetrics().elidedText(title, Qt::ElideRight,
                                                                              rect.width()));
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
                        const auto rect = textRect();
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
                                QPointF topLeft(isLeftToRight ? (pageLeft + leftDelta)
                                                              : (textRight + leftDelta),
                                                cursorR.top());
                                QPointF bottomRight(isLeftToRight ? textLeft + leftDelta
                                                                  : pageRight + leftDelta,
                                                    cursorR.bottom());
                                QRectF rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignRight | Qt::AlignTop, sceneNumber);
                            }
                            if (d->showSceneNumberOnRight) {
                                QPointF topLeft(isLeftToRight ? (textRight + leftDelta)
                                                              : (pageLeft - leftDelta),
                                                cursorR.top());
                                QPointF bottomRight(isLeftToRight ? pageRight
                                                                  : textLeft - leftDelta,
                                                    cursorR.bottom());
                                QRectF rect(topLeft, bottomRight);
                                painter.drawText(rect, Qt::AlignLeft | Qt::AlignTop, sceneNumber);
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
                    painter.drawText(rect, Qt::AlignRight | Qt::AlignTop,
                                     blockType == BusinessLayer::TextParagraphType::BeatHeading
                                         ? u8"\U000F09DE"
                                         : u8"\U000F0AEF");
                }

                //
                // Прорисовка префикса/постфикса для блока текста
                //
                if ((block.charFormat().hasProperty(TextBlockStyle::PropertyPrefix)
                     || block.charFormat().hasProperty(TextBlockStyle::PropertyPostfix))) {
                    setPainterPen(palette().text().color());
                    painter.setFont(block.charFormat().font());

                    //
                    // Из-за того, что при смешивании RTL и LTR текста курсор в параграфе может
                    // находиться в разных местах, проходим каждый символ абзаца и выбираем крайние
                    // положения текста по углам в соответствии с направлением
                    //
                    auto decorationCursor = cursor;
                    decorationCursor.setPosition(block.position());
                    auto decorationCursorR = cursorRect(decorationCursor);
                    auto prefixTopLeft = decorationCursorR.topLeft();
                    auto postfixTopRight = decorationCursorR.topRight();
                    while (!decorationCursor.atBlockEnd()) {
                        decorationCursor.movePosition(BusinessLayer::TextCursor::NextCharacter);
                        decorationCursorR = cursorRect(decorationCursor);
                        if (prefixTopLeft.x() > decorationCursorR.left()
                            || (block.text().isRightToLeft()
                                && prefixTopLeft.y() < decorationCursorR.top())) {
                            prefixTopLeft = decorationCursorR.topLeft();
                        }
                        if (postfixTopRight.x() < decorationCursorR.right()
                            || (!block.text().isRightToLeft()
                                && postfixTopRight.y() < decorationCursorR.top())) {
                            postfixTopRight = decorationCursorR.topRight();
                        }
                    }

                    const auto prefix
                        = block.charFormat().stringProperty(TextBlockStyle::PropertyPrefix);
                    const QRectF prefixRect(
                        prefixTopLeft.x() - TextHelper::fineTextWidthF(prefix, painter.font()),
                        prefixTopLeft.y(), TextHelper::fineTextWidthF(prefix, painter.font()),
                        decorationCursorR.height());
                    painter.drawText(prefixRect,
                                     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextForceLeftToRight,
                                     prefix);

                    const auto postfix
                        = block.charFormat().stringProperty(TextBlockStyle::PropertyPostfix);
                    const QRectF postfixRect(postfixTopRight.x(), postfixTopRight.y(),
                                             TextHelper::fineTextWidthF(postfix, painter.font()),
                                             decorationCursorR.height());
                    painter.drawText(postfixRect,
                                     Qt::AlignLeft | Qt::AlignVCenter | Qt::TextForceLeftToRight,
                                     postfix);
                }
            }

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
    if (isReadOnly() || hasSpellingMenu(_position)) {
        return menu;
    }

    const BusinessLayer::TextCursor cursor = textCursor();

    auto splitAction = new QAction(this);
    splitAction->setSeparator(true);
    splitAction->setWhatsThis(QKeySequence("Ctrl+D").toString(QKeySequence::NativeText));
    splitAction->setIconText(u8"\U000F1917");
    if (cursor.inTable()) {
        splitAction->setText(tr("Merge paragraph"));
    } else {
        splitAction->setText(tr("Split paragraph"));

        //
        // Запрещаем разделять некоторые блоки
        //
        splitAction->setEnabled(d->canSplitParagraph(cursor));
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

    //
    // Генерация текста сценария
    //
    auto generateAction = new QAction(this);
    generateAction->setSeparator(true);
    generateAction->setText(tr("Generate text"));
    generateAction->setIconText(u8"\U000F0068");
    connect(generateAction, &QAction::triggered, this,
            [this] { emit generateTextRequested(textCursor().selectedText()); });

    auto actions = menu->actions().toVector();
    actions.prepend(splitAction);
    actions.prepend(bookmarkAction);
    actions.append(generateAction);
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
    //
    {
        QByteArray text;
        auto cursor = textCursor();
        cursor.setPosition(selection.from);
        do {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

            //
            // Для текстового представления не копируем невидимые блоки с содержанием текста
            // сцен т.к. пользователи этого не ожидают
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
    // Добавим фонтан
    //
    {
        //
        // Подготавливаем опции для экспорта в фонтан
        //
        BusinessLayer::ScreenplayExportOptions options;
        options.filePath = QDir::temp().absoluteFilePath("clipboard.fountain");
        options.includeTiltePage = false;
        options.includeSynopsis = false;
        options.showScenesNumbers = d->model->informationModel()->showSceneNumbers();
        //
        // ... сохраняем в формате фонтана
        //
        BusinessLayer::ScreenplayFountainExporter().exportTo(d->model, selection.from, selection.to,
                                                             options);
        //
        // ... читаем сохранённый экспорт из файла
        //
        QFile file(options.filePath);
        QByteArray text;
        if (file.open(QIODevice::ReadOnly)) {
            text = file.readAll();
            file.close();
        }

        if (!text.isEmpty()) {
            mimeData->setData(kMarkdownMimeType, text);
        }
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
        mimeData->setData(d->model->mimeTypes().constFirst(), mime.toUtf8());
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
    else if (_source->hasFormat(kMarkdownMimeType) || _source->hasText()) {
        const auto text = _source->hasFormat(kMarkdownMimeType) ? _source->data(kMarkdownMimeType)
                                                                : _source->text();

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

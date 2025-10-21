#include "novel_outline_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/novel/text/novel_text_corrector.h>
#include <business_layer/document/novel/text/novel_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/import/novel/novel_markdown_importer.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/novel/novel_dictionaries_model.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/model/novel/text/novel_text_block_parser.h>
#include <business_layer/model/novel/text/novel_text_mime_handler.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/novel/text/novel_text_model_text_item.h>
#include <business_layer/templates/novel_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QAction>
#include <QCoreApplication>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QTextTable>
#include <QTimer>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;

namespace Ui {

namespace {
const QSet<BusinessLayer::TextParagraphType> kEndOfBeatTypes = {
    BusinessLayer::TextParagraphType::BeatHeading,
    BusinessLayer::TextParagraphType::SceneHeading,
    BusinessLayer::TextParagraphType::ChapterHeading,
    BusinessLayer::TextParagraphType::ChapterFooter,
    BusinessLayer::TextParagraphType::PartHeading,
    BusinessLayer::TextParagraphType::PartFooter,
};
}

class NovelOutlineEdit::Implementation
{
public:
    explicit Implementation(NovelOutlineEdit* _q);

    /**
     * @brief Текущий шаблон документа
     */
    const BusinessLayer::NovelTemplate& novelTemplate() const;

    /**
     * @brief Отменить/повторить последнее действие
     */
    void revertAction(bool previous);

    /**
     * @brief Получить текстовый элемент в текущем курсоре
     */
    BusinessLayer::TextModelItem* currentItem() const;

    /**
     * @brief Обновить редакторскую заметку в заданном интервале
     */
    void updateReviewMark(QKeyEvent* _event, int _from, int _to);


    NovelOutlineEdit* q = nullptr;

    QPointer<BusinessLayer::NovelTextModel> model;
    BusinessLayer::NovelTextDocument document;

    bool showSceneNumber = false;
    bool showSceneNumberOnLeft = false;
    bool showSceneNumberOnRight = false;
    bool showDialogueNumber = false;

    struct {
        bool isEnabled = false;
        QColor textColor;
        QColor backgroundColor;
        bool isRevision = false;
        bool isTrackChanges = false;
    } autoReviewMode;

    struct {
        int inBlock = 0;
        int inDocument = 0;
        int blockLength = 0;
    } lastPosition;
};

NovelOutlineEdit::Implementation::Implementation(NovelOutlineEdit* _q)
    : q(_q)
{
    document.setOutlineDocument(true);
}

const BusinessLayer::NovelTemplate& NovelOutlineEdit::Implementation::novelTemplate() const
{
    const auto currentTemplateId
        = model && model->informationModel() ? model->informationModel()->templateId() : "";
    return TemplatesFacade::novelTemplate(currentTemplateId);
}

void NovelOutlineEdit::Implementation::revertAction(bool previous)
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

BusinessLayer::TextModelItem* NovelOutlineEdit::Implementation::currentItem() const
{
    if (model == nullptr) {
        return nullptr;
    }

    auto userData = q->textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto novelBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return novelBlockData->item();
}

void NovelOutlineEdit::Implementation::updateReviewMark(QKeyEvent* _event, int _from, int _to)
{
    if (!autoReviewMode.isEnabled) {
        return;
    }

    //
    // Если включён режим автоматического добавления редакторских заметок
    // ... и текст добавляется с клавиатуры и это не шорткат
    // ... или вставляется из буфера обмена
    // ... и позиция курсора изменилась после обработки события
    //
    if ((_event == nullptr
         || ((_event->modifiers().testFlag(Qt::NoModifier)
              || _event->modifiers().testFlag(Qt::ShiftModifier))
             && !_event->text().isEmpty())
         || _event == QKeySequence::Paste)
        && _from < _to) {
        //
        // То автоматически добавим редакторскую заметку
        //
        const auto lastCursor = q->textCursor();
        auto cursor = lastCursor;
        cursor.setPosition(_from);
        cursor.setPosition(_to, QTextCursor::KeepAnchor);
        q->setTextCursor(cursor);
        q->addReviewMark(autoReviewMode.textColor, autoReviewMode.backgroundColor, {},
                         autoReviewMode.isRevision, autoReviewMode.isTrackChanges, false);
        q->setTextCursor(lastCursor);
    }
}


// ****


NovelOutlineEdit::NovelOutlineEdit(QWidget* _parent)
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
            &NovelOutlineEdit::updateCollaboratorsCursors);
    connect(this, &NovelOutlineEdit::completed, this,
            [this](const QModelIndex& _index, int _from, int _to) {
                Q_UNUSED(_index)
                d->updateReviewMark(nullptr, _from, _to);
            });
}

NovelOutlineEdit::~NovelOutlineEdit() = default;

void NovelOutlineEdit::setCorrectionOptions(bool _needToCorrectPageBreaks)
{
    d->document.setCorrectionOptions(_needToCorrectPageBreaks);
}

void NovelOutlineEdit::initWithModel(BusinessLayer::NovelTextModel* _model)
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
        const auto& currentTemplate = d->novelTemplate();
        setPageFormat(currentTemplate.pageSizeId());
        setPageMarginsMm(currentTemplate.pageMargins());
        setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
        setShowPageNumberAtFirstPage(currentTemplate.isFirstPageNumberVisible());
    }

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний
    // изменений
    //
    d->document.setModel(d->model);

    //
    // Отслеживаем некоторые события модели
    //
    if (d->model && d->model->informationModel()) {
        setHeader(d->model->informationModel()->header());
        setFooter(d->model->informationModel()->footer());

        connect(d->model, &BusinessLayer::NovelTextModel::dataChanged, this,
                qOverload<>(&NovelOutlineEdit::update));
        connect(d->model->informationModel(), &BusinessLayer::NovelInformationModel::headerChanged,
                this, &NovelOutlineEdit::setHeader);
        connect(d->model->informationModel(), &BusinessLayer::NovelInformationModel::footerChanged,
                this, &NovelOutlineEdit::setFooter);
        //
        // Корректируем позицию курсора при совместной работе
        //
        connect(d->model, &BusinessLayer::NovelTextModel::changesAboutToBeApplied, this, [this] {
            d->lastPosition.inBlock = textCursor().positionInBlock();
            d->lastPosition.inDocument = textCursor().position();
            d->lastPosition.blockLength = textCursor().block().length();
        });
        connect(d->model, &BusinessLayer::NovelTextModel::changesApplied, this,
                [this](const BusinessLayer::ChangeCursor& _changeCursor) {
                    //
                    // Если курсор сместился в конец блока, значит изменение затрагивало данный блок
                    //
                    auto cursor = textCursor();
                    if (d->lastPosition.inBlock != cursor.positionInBlock()
                        && cursor.positionInBlock() == cursor.block().length() - 1) {
                        //
                        // ... если изменение было за курсором, то восстановим предыдущую позицию
                        //
                        if (_changeCursor.position > d->lastPosition.inBlock) {
                            cursor.setPosition(d->lastPosition.inDocument);
                        }
                        //
                        // ... если изменение было перед, то корректируем позицию с учётом изменения
                        //     количества символов в текущем абзаце
                        //
                        else {
                            cursor.setPosition(d->lastPosition.inDocument + cursor.block().length()
                                               - d->lastPosition.blockLength);
                        }
                        setTextCursorAndKeepScrollBars(cursor);
                    }
                });
    }

    emit cursorPositionChanged();
}

void NovelOutlineEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);
}

const BusinessLayer::NovelTemplate& NovelOutlineEdit::novelTemplate() const
{
    return d->novelTemplate();
}

QAbstractItemModel* NovelOutlineEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersList();
}

void NovelOutlineEdit::createCharacter(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createCharacter(_name);
}

QAbstractItemModel* NovelOutlineEdit::locations() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->locationsModel();
}

void NovelOutlineEdit::createLocation(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createLocation(_name);
}

void NovelOutlineEdit::undo()
{
    d->revertAction(true);
}

void NovelOutlineEdit::redo()
{
    d->revertAction(false);
}

void NovelOutlineEdit::addParagraph(TextParagraphType _type)
{
    //
    // Если добавляется бит, то позиционируем его в конце сцены, либо между другими битами
    //
    if (_type == TextParagraphType::BeatHeading) {
        //
        // Если это не вставка бита после бита
        //
        if (currentParagraphType() != TextParagraphType::BeatHeading) {
            BusinessLayer::TextCursor cursor = textCursor();
            //
            // ... передвигаем курсор
            //
            while (cursor.block().next().isValid() && !cursor.block().next().isVisible()
                   && !cursor.atEnd()) {
                moveCursor(QTextCursor::NextBlock);
                moveCursor(QTextCursor::EndOfBlock);
                cursor = textCursor();
            }
        }
        //
        // ... и добавляем блок бита
        //
        d->document.addParagraph(_type, textCursor());
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
}

void NovelOutlineEdit::setCurrentParagraphType(TextParagraphType _type)
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
        TextParagraphType::SceneHeading,   TextParagraphType::BeatHeading,
        TextParagraphType::ChapterHeading, TextParagraphType::ChapterFooter,
        TextParagraphType::PartHeading,    TextParagraphType::PartFooter,
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
    if (_type == TextParagraphType::PartHeading || _type == TextParagraphType::ChapterHeading) {
        moveCursor(QTextCursor::PreviousBlock);
    }

    emit paragraphTypeChanged();
}

BusinessLayer::TextParagraphType NovelOutlineEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

QModelIndex NovelOutlineEdit::currentModelIndex() const
{
    if (d->model == nullptr || d->document.isEditTransactionActive()) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto novelBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(novelBlockData->item());
}

void NovelOutlineEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.model() != d->model) {
        return;
    }

    BusinessLayer::TextCursor textCursor(document());
    textCursor.setPosition(d->document.itemStartPosition(_index));
    ensureCursorVisible(textCursor);
}

void NovelOutlineEdit::setVisibleTopLevelItemIndex(const QModelIndex& _index)
{
    d->document.setVisibleTopLevelItem(_index);
}

int NovelOutlineEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void NovelOutlineEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                     const QString& _comment, bool _isRevision, bool _isAddition,
                                     bool _isRemoval)
{
    BusinessLayer::TextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, _isRevision, _isAddition,
                              _isRemoval, cursor);
}

void NovelOutlineEdit::setAutoReviewModeEnabled(bool _enabled)
{
    d->autoReviewMode.isEnabled = _enabled;
}

void NovelOutlineEdit::setAutoReviewMode(const QColor& _textColor, const QColor& _backgroundColor,
                                         bool _isRevision, bool _isTrackChanges)
{
    d->autoReviewMode.textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    d->autoReviewMode.backgroundColor = _backgroundColor;
    d->autoReviewMode.isRevision = _isRevision;
    d->autoReviewMode.isTrackChanges = _isTrackChanges;
}

void NovelOutlineEdit::keyPressEvent(QKeyEvent* _event)
{
    if (isReadOnly()) {
        ScriptTextEdit::keyPressEvent(_event);
        return;
    }

    //
    // Если активен режим отслеживания изменений, то обработаем случаи удаления текста,
    // чтобы вместо удаления, текст лишь помечался удалённым
    //
    do {
        if (BusinessLayer::TextCursor cursor = textCursor(); d->autoReviewMode.isEnabled
            && d->autoReviewMode.isTrackChanges
            && (_event->key() == Qt::Key_Delete || _event->key() == Qt::Key_Backspace
                || (!_event->text().isEmpty() && cursor.hasSelection()))) {
            //
            // ... если был удалён один символ, выделим его
            //
            if ((_event->key() == Qt::Key_Delete || _event->key() == Qt::Key_Backspace)
                && !cursor.hasSelection()) {
                cursor.movePosition(_event->key() == Qt::Key_Delete
                                        ? QTextCursor::NextCharacter
                                        : QTextCursor::PreviousCharacter,
                                    QTextCursor::KeepAnchor, 1);
                setTextCursor(cursor);
            }

            //
            // ... если в выделении находится добавленный текст в режиме отслеживания изменений,
            //     то удалим его нормальным способом, а не будем помечать, как удалённый
            //
            const auto selectionInterval = cursor.selectionInterval();
            auto block = cursor.block();
            bool isAdditionSelected = true;
            while (isAdditionSelected && block.isValid()
                   && block.position() <= selectionInterval.to) {
                const auto blockTextFormats = block.textFormats();
                for (const auto& textFormat : blockTextFormats) {
                    //
                    // Пропускаем форматы до выделения
                    //
                    if (textFormat.start + textFormat.length
                        <= selectionInterval.from - block.position()) {
                        continue;
                    }
                    //
                    // Игнорируем форматы после выделения
                    //
                    if (textFormat.start >= selectionInterval.to - block.position()) {
                        break;
                    }

                    //
                    // Если это не добавление, то прерываем поиск
                    //
                    if (textFormat.format.property(TextBlockStyle::PropertyCommentsIsAddition)
                        != QStringList{ "true" }) {
                        isAdditionSelected = false;
                        break;
                    }
                }

                block = block.next();
            }
            if (isAdditionSelected) {
                break;
            }

            //
            // ... если же в выделении был обычный текст, то добавляем ему соответствующую разметку
            //
            addReviewMark({}, ColorHelper::removedTextBackgroundColor(), {}, false, false, true);
            cursor.setPosition(_event->key() == Qt::Key_Backspace ? cursor.selectionInterval().from
                                                                  : cursor.selectionInterval().to);
            setTextCursor(cursor);

            //
            // ... если это было удаление, то прекращаем дальнейшую обработку
            //
            if (_event->key() == Qt::Key_Delete || _event->key() == Qt::Key_Backspace) {
                _event->accept();
                return;
            }
        }
    }
    once;

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
        const int positionBeforeEventHandling = textCursor().position();
        if (keyPressEventReimpl(_event)) {
            _event->accept();
        } else {
            ScriptTextEdit::keyPressEvent(_event);
            _event->ignore();
        }
        const int positionAfterEventHandling = textCursor().position();

        updateEnteredText(_event);
        d->updateReviewMark(_event, positionBeforeEventHandling, positionAfterEventHandling);
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

bool NovelOutlineEdit::keyPressEventReimpl(QKeyEvent* _event)
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
        if (_event == QKeySequence::Cut) {
            d->model->saveChanges();
        }
    }

    return isEventHandled;
}

void NovelOutlineEdit::paintEvent(QPaintEvent* _event)
{
    ScriptTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight = viewport()->width();
    const qreal spaceBetweenSceneNumberAndText = 10 * DesignSystem::scaleFactor();
    ;
    const qreal textLeft = pageLeft - (isLeftToRight ? 0 : horizontalScrollMaximum())
        + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight + (isLeftToRight ? horizontalScrollMaximum() : 0)
        - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScroll();
    //    int colorRectWidth = 0;
    qreal verticalMargin = 0;
    const qreal splitterX = leftDelta + textLeft
        + (textRight - textLeft) * d->novelTemplate().leftHalfOfPageWidthPercents() / 100;


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
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::PartHeading
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

        auto setPainterPen = [&painter, &block, this](const QColor& _color) {
            painter.setPen(ColorHelper::transparent(
                _color,
                1.0
                    - (isFocusCurrentParagraph() && block != textCursor().block()
                           ? DesignSystem::inactiveTextOpacity()
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
                || blockType == TextParagraphType::ChapterHeading
                || blockType == TextParagraphType::PartHeading) {
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
                    QPointF topLeft(isLeftToRight ? (pageLeft + leftDelta
                                                     + DesignSystem::card().shadowMargins().left())
                                                  : (textRight + leftDelta),
                                    cursorR.top());
                    QPointF bottomRight(isLeftToRight
                                            ? textLeft + leftDelta
                                            : (pageRight + leftDelta
                                               - DesignSystem::card().shadowMargins().right()),
                                        cursorR.bottom());
                    QRectF rect(topLeft, bottomRight);
                    const auto yDelta = DesignSystem::layout().px(32) - rect.height() / 2.0;
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
                                     ColorHelper::transparent(bookmark.color,
                                                              DesignSystem::elevationEndOpacity()));
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
                    setPainterPen(ColorHelper::transparent(palette().text().color(),
                                                           DesignSystem::inactiveItemOpacity()));

                    //
                    // Для пустого футера рисуем плейсхолдер
                    //
                    if (blockType == TextParagraphType::PartFooter
                        || blockType == TextParagraphType::ChapterFooter) {
                        painter.setFont(block.charFormat().font());

                        //
                        // Ищем открывающий блок папки
                        //
                        auto headerBlock = block.previous();
                        int openedFolders = 0;
                        while (headerBlock.isValid()) {
                            const auto headerBlockType = TextBlockStyle::forBlock(headerBlock);
                            if (headerBlockType == TextParagraphType::PartHeading
                                || headerBlockType == TextParagraphType::ChapterHeading) {
                                if (openedFolders > 0) {
                                    --openedFolders;
                                } else {
                                    break;
                                }
                            } else if (headerBlockType == TextParagraphType::PartFooter
                                       || headerBlockType == TextParagraphType::ChapterFooter) {
                                ++openedFolders;
                            }

                            headerBlock = headerBlock.previous();
                        }

                        //
                        // Определим область для отрисовки плейсхолдера
                        //
                        const auto placeholderText = QString("%1 %2").arg(
                            QCoreApplication::translate("KeyProcessingLayer::FolderFooterHandler",
                                                        "End of"),
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
                    if (blockType == TextParagraphType::PartHeading
                        || blockType == TextParagraphType::ChapterHeading) {
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
                                         blockType == TextParagraphType::PartHeading
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
                    // Прорисовка ревизий
                    //
                    {
                        //
                        // Собираем ревизии для отображения
                        //
                        QVector<QPair<QRectF, QColor>> revisionMarks;
                        for (const auto& format : block.textFormats()) {
                            if (const auto revision
                                = format.format.property(TextBlockStyle::PropertyCommentsIsRevision)
                                      .toStringList();
                                !revision.isEmpty() && revision.constFirst() == "true") {
                                auto revisionCursor = cursor;
                                revisionCursor.setPosition(revisionCursor.block().position()
                                                           + format.start);
                                do {
                                    if (revisionCursor.positionInBlock() != format.start) {
                                        revisionCursor.movePosition(QTextCursor::NextCharacter);
                                    }
                                    const auto revisionCursorR = cursorRect(revisionCursor);
                                    QPointF topLeft(isLeftToRight ? (textRight + leftDelta)
                                                                  : (pageLeft - leftDelta),
                                                    revisionCursorR.top());
                                    QPointF bottomRight(isLeftToRight ? pageRight
                                                                      : textLeft - leftDelta,
                                                        revisionCursorR.bottom());
                                    const QRectF rect(topLeft, bottomRight);
                                    const auto revisionColor = format.format.foreground().color();
                                    //
                                    // ... первая звёздочка, или звёздочка на следующей строке
                                    //
                                    if (revisionMarks.isEmpty()
                                        || revisionMarks.constLast().first != rect) {
                                        revisionMarks.append({ rect, revisionColor });
                                    }
                                    //
                                    // ... звёздочка на той же строке - проверяем уровень
                                    //
                                    else if (ColorHelper::revisionLevel(
                                                 revisionMarks.constLast().second)
                                             < ColorHelper::revisionLevel(revisionColor)) {
                                        revisionMarks.last().second = revisionColor;
                                    }
                                    if (!revisionCursor.movePosition(QTextCursor::EndOfLine)) {
                                        revisionCursor.movePosition(QTextCursor::EndOfBlock);
                                    }
                                } while (revisionCursor.positionInBlock()
                                             < (format.start + format.length)
                                         && !revisionCursor.atBlockEnd());
                            }
                        }

                        painter.setFont(cursor.charFormat().font());
                        for (const auto& reviewMark : std::as_const(revisionMarks)) {
                            setPainterPen(reviewMark.second);
                            painter.drawText(reviewMark.first, Qt::AlignHCenter | Qt::AlignTop,
                                             "*");
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
    }

    //
    // Курсоры соавторов
    //
    painter.setClipRect(QRectF(), Qt::NoClip);
    paintCollaboratorsCursors(painter, d->model->document()->uuid(), topBlock, bottomBlock);
}

ContextMenu* NovelOutlineEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
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
        connect(createBookmark, &QAction::triggered, this, &NovelOutlineEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this, &NovelOutlineEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &NovelOutlineEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this, &NovelOutlineEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(bookmarkAction);
    menu->setActions(actions);

    return menu;
}

bool NovelOutlineEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* NovelOutlineEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    BusinessLayer::TextCursor cursor = textCursor();
    auto selection = cursor.selectionInterval();
    //
    // При необходимости расширим выделение, чтобы туда попало содержимое битов
    //
    cursor.setPosition(selection.from);
    cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
    if (cursor.atBlockEnd()
        && BusinessLayer::TextBlockStyle::forCursor(cursor)
            == BusinessLayer::TextParagraphType::BeatHeading) {
        do {
            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);

            if (kEndOfBeatTypes.contains(BusinessLayer::TextBlockStyle::forCursor(cursor))) {
                cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                break;
            }
        } while (!cursor.atEnd());
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

void NovelOutlineEdit::insertFromMimeData(const QMimeData* _source)
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
    // Если вставляются данные в сценарном формате
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
        BusinessLayer::NovelMarkdownImporter markdownImporter;
        textToInsert = markdownImporter.novelText("\n" + _source->text()).text;
    }

    //
    // Преобразовываем все текстовые блоки в биты
    //
    textToInsert = BusinessLayer::NovelTextMimeHandler::convertTextBlocksToBeats(textToInsert);

    //
    // Собственно вставка данных
    //
    auto cursorPosition = d->document.insertFromMime(cursor.position(), textToInsert);

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

void NovelOutlineEdit::dropEvent(QDropEvent* _event)
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

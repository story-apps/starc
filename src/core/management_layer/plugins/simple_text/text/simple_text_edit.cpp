#include "simple_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/export/simple_text/simple_text_markdown_exporter.h>
#include <business_layer/import/text/simple_text_markdown_importer.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/audioplay_synopsis_model.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/comic_book_synopsis_model.h>
#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/model/novel/novel_synopsis_model.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_synopsis_model.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/stageplay_synopsis_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QAction>
#include <QCoreApplication>
#include <QDir>
#include <QLocale>
#include <QMimeData>
#include <QPainter>
#include <QPointer>
#include <QTextTable>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextCursor;
using BusinessLayer::TextParagraphType;

namespace Ui {

namespace {
const QLatin1String kMarkdownMimeType("text/markdown");
}

class SimpleTextEdit::Implementation
{
public:
    explicit Implementation(SimpleTextEdit* _q);

    /**
     * @brief Текущий шаблон документа
     */
    const BusinessLayer::TextTemplate& textTemplate() const;

    /**
     * @brief Отменить/повторить последнее действие
     */
    void revertAction(bool previous);

    /**
     * @brief Обновить редакторскую заметку в заданном интервале
     */
    void updateReviewMark(QKeyEvent* _event, int _from, int _to);


    SimpleTextEdit* q = nullptr;

    QPointer<BusinessLayer::SimpleTextModel> model;
    BusinessLayer::SimpleTextDocument document;

    struct {
        bool isEnabled = false;
        QColor textColor;
        QColor backgroundColor;
        bool isRevision = false;
        bool isTrackChanges = false;
    } autoReviewMode;
};

SimpleTextEdit::Implementation::Implementation(SimpleTextEdit* _q)
    : q(_q)
{
}

const BusinessLayer::TextTemplate& SimpleTextEdit::Implementation::textTemplate() const
{
    return TemplatesFacade::textTemplate(model);
}

void SimpleTextEdit::Implementation::revertAction(bool previous)
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

void SimpleTextEdit::Implementation::updateReviewMark(QKeyEvent* _event, int _from, int _to)
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


SimpleTextEdit::SimpleTextEdit(QWidget* _parent)
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
            &SimpleTextEdit::updateCollaboratorsCursors);
    connect(this, &SimpleTextEdit::completed, this,
            [this](const QModelIndex& _index, int _from, int _to) {
                Q_UNUSED(_index)
                d->updateReviewMark(nullptr, _from, _to);
            });
}

SimpleTextEdit::~SimpleTextEdit() = default;

void SimpleTextEdit::setCorrectionOptions(bool _needToCorrectPageBreaks)
{
    d->document.setCorrectionOptions(_needToCorrectPageBreaks);
}

void SimpleTextEdit::initWithModel(BusinessLayer::SimpleTextModel* _model)
{
    if (d->model) {
        d->model->disconnect(this);

        if (auto synopsisModel = qobject_cast<BusinessLayer::ScreenplaySynopsisModel*>(d->model)) {
            synopsisModel->informationModel()->disconnect(this);
        } else if (auto synopsisModel
                   = qobject_cast<BusinessLayer::ComicBookSynopsisModel*>(d->model)) {
            synopsisModel->informationModel()->disconnect(this);
        } else if (auto synopsisModel
                   = qobject_cast<BusinessLayer::AudioplaySynopsisModel*>(d->model)) {
            synopsisModel->informationModel()->disconnect(this);
        } else if (auto synopsisModel
                   = qobject_cast<BusinessLayer::StageplaySynopsisModel*>(d->model)) {
            synopsisModel->informationModel()->disconnect(this);
        } else if (auto synopsisModel
                   = qobject_cast<BusinessLayer::NovelSynopsisModel*>(d->model)) {
            synopsisModel->informationModel()->disconnect(this);
        }
    }

    d->model = _model;

    //
    // Сбрасываем модель, чтобы не вылезали изменения документа при изменении параметров страницы
    //
    d->document.setModel(nullptr);

    //
    // Сценарий
    //
    if (auto synopsisModel = qobject_cast<BusinessLayer::ScreenplaySynopsisModel*>(d->model)) {
        auto updateHeader
            = [this, synopsisModel] { setHeader(synopsisModel->informationModel()->header()); };
        updateHeader();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::headerChanged, this, updateHeader);

        auto updateFooter
            = [this, synopsisModel] { setFooter(synopsisModel->informationModel()->footer()); };
        updateFooter();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::footerChanged, this, updateFooter);

        connect(synopsisModel->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::templateIdChanged, this,
                &SimpleTextEdit::reinit);
    }
    //
    // Комикс
    //
    else if (auto synopsisModel = qobject_cast<BusinessLayer::ComicBookSynopsisModel*>(d->model)) {
        auto updateHeader
            = [this, synopsisModel] { setHeader(synopsisModel->informationModel()->header()); };
        updateHeader();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::headerChanged, this, updateHeader);

        auto updateFooter
            = [this, synopsisModel] { setFooter(synopsisModel->informationModel()->footer()); };
        updateFooter();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::footerChanged, this, updateFooter);

        connect(synopsisModel->informationModel(),
                &BusinessLayer::ComicBookInformationModel::templateIdChanged, this,
                &SimpleTextEdit::reinit);
    }
    //
    // Аудиопостановка
    //
    else if (auto synopsisModel = qobject_cast<BusinessLayer::AudioplaySynopsisModel*>(d->model)) {
        auto updateHeader
            = [this, synopsisModel] { setHeader(synopsisModel->informationModel()->header()); };
        updateHeader();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::headerChanged, this, updateHeader);

        auto updateFooter
            = [this, synopsisModel] { setFooter(synopsisModel->informationModel()->footer()); };
        updateFooter();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::footerChanged, this, updateFooter);

        connect(synopsisModel->informationModel(),
                &BusinessLayer::AudioplayInformationModel::templateIdChanged, this,
                &SimpleTextEdit::reinit);
    }
    //
    // Пьеса
    //
    else if (auto synopsisModel = qobject_cast<BusinessLayer::StageplaySynopsisModel*>(d->model)) {
        auto updateHeader
            = [this, synopsisModel] { setHeader(synopsisModel->informationModel()->header()); };
        updateHeader();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::headerChanged, this, updateHeader);

        auto updateFooter
            = [this, synopsisModel] { setFooter(synopsisModel->informationModel()->footer()); };
        updateFooter();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::footerChanged, this, updateFooter);

        connect(synopsisModel->informationModel(),
                &BusinessLayer::StageplayInformationModel::templateIdChanged, this,
                &SimpleTextEdit::reinit);
    }
    //
    // Роман
    //
    else if (auto synopsisModel = qobject_cast<BusinessLayer::NovelSynopsisModel*>(d->model)) {
        auto updateHeader
            = [this, synopsisModel] { setHeader(synopsisModel->informationModel()->header()); };
        updateHeader();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::NovelInformationModel::headerChanged, this, updateHeader);

        auto updateFooter
            = [this, synopsisModel] { setFooter(synopsisModel->informationModel()->footer()); };
        updateFooter();
        connect(synopsisModel->informationModel(),
                &BusinessLayer::NovelInformationModel::footerChanged, this, updateFooter);

        connect(synopsisModel->informationModel(),
                &BusinessLayer::NovelInformationModel::templateIdChanged, this,
                &SimpleTextEdit::reinit);
    }

    //
    // Настроим редактор в соответствии с параметрами шаблона
    //
    const auto currentTemplate = TemplatesFacade::textTemplate(_model);
    setPageFormat(currentTemplate.pageSizeId());
    setPageMarginsMm(currentTemplate.pageMargins());
    setPageNumbersAlignment(currentTemplate.pageNumbersAlignment());
    setShowPageNumberAtFirstPage(currentTemplate.isFirstPageNumberVisible());

    //
    // Документ нужно формировать только после того, как редактор настроен, чтобы избежать лишний
    // изменений
    //
    d->document.setModel(d->model);

    if (d->model) {
        connect(d->model, &BusinessLayer::SimpleTextModel::dataChanged, this,
                qOverload<>(&SimpleTextEdit::update));
    }

    emit cursorPositionChanged();
}

void SimpleTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);
}

const BusinessLayer::TextTemplate& SimpleTextEdit::textTemplate() const
{
    return d->textTemplate();
}

void SimpleTextEdit::undo()
{
    d->revertAction(true);
}

void SimpleTextEdit::redo()
{
    d->revertAction(false);
}

void SimpleTextEdit::addParagraph(BusinessLayer::TextParagraphType _type)
{
    //
    // Если изменяется заголовок изолированного элемента, то снимаем изоляцию на время
    // операции, а после изолируем предшествующий текущему элемент, либо его родителя
    //
    const QSet<TextParagraphType> headerTypes = {
        TextParagraphType::ChapterHeading1, TextParagraphType::ChapterHeading2,
        TextParagraphType::ChapterHeading3, TextParagraphType::ChapterHeading4,
        TextParagraphType::ChapterHeading5, TextParagraphType::ChapterHeading6,
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

    emit paragraphTypeChanged();
}

void SimpleTextEdit::setCurrentParagraphType(BusinessLayer::TextParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    //
    // Если изменяется заголовок изолированного элемента, то снимаем изоляцию на время
    // операции, а после изолируем предшествующий текущему элемент, либо его родителя
    //
    const QSet<TextParagraphType> headerTypes = {
        TextParagraphType::ChapterHeading1, TextParagraphType::ChapterHeading2,
        TextParagraphType::ChapterHeading3, TextParagraphType::ChapterHeading4,
        TextParagraphType::ChapterHeading5, TextParagraphType::ChapterHeading6,
    };

    const auto currentTypeIsHeader = headerTypes.contains(currentParagraphType());
    const auto targetTypeIsHeader = headerTypes.contains(_type);
    const auto needReisolate = (currentTypeIsHeader || targetTypeIsHeader)
        && d->document.visibleTopLeveLItem().isValid();
    if (needReisolate) {
        d->document.setVisibleTopLevelItem({});
    }

    d->document.setParagraphType(_type, textCursor());

    //
    // ... при необходимости восстанавливаем режим изоляции
    //
    if (needReisolate) {
        d->document.setVisibleTopLevelItem(d->document.itemIndex(textCursor().block()));
    }

    emit paragraphTypeChanged();
}

BusinessLayer::TextParagraphType SimpleTextEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

QModelIndex SimpleTextEdit::currentModelIndex() const
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

void SimpleTextEdit::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid() || _index.model() != d->model || _index == currentModelIndex()) {
        return;
    }

    const auto cursorPosition = d->document.itemStartPosition(_index);
    if (cursorPosition == -1) {
        return;
    }

    BusinessLayer::TextCursor textCursor(document());
    textCursor.setPosition(cursorPosition);
    ensureCursorVisible(textCursor);
}

void SimpleTextEdit::setVisibleTopLevelItemIndex(const QModelIndex& _index)
{
    d->document.setVisibleTopLevelItem(_index);
}

int SimpleTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void SimpleTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                   const QString& _comment, bool _isRevision, bool _isAddition,
                                   bool _isRemoval)
{
    QTextCursor cursor(textCursor());
    if (!cursor.hasSelection()) {
        return;
    }

    d->document.addReviewMark(_textColor, _backgroundColor, _comment, _isRevision, _isAddition,
                              _isRemoval, cursor);
}

void SimpleTextEdit::setAutoReviewModeEnabled(bool _enabled)
{
    d->autoReviewMode.isEnabled = _enabled;
}

void SimpleTextEdit::setAutoReviewMode(const QColor& _textColor, const QColor& _backgroundColor,
                                       bool _isRevision, bool _isTrackChanges)
{
    d->autoReviewMode.textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    d->autoReviewMode.backgroundColor = _backgroundColor;
    d->autoReviewMode.isRevision = _isRevision;
    d->autoReviewMode.isTrackChanges = _isTrackChanges;
}

void SimpleTextEdit::keyPressEvent(QKeyEvent* _event)
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

bool SimpleTextEdit::keyPressEventReimpl(QKeyEvent* _event)
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

void SimpleTextEdit::paintEvent(QPaintEvent* _event)
{
    ScriptTextEdit::paintEvent(_event);

    //
    // Определить область прорисовки по краям от текста
    //
    const bool isLeftToRight = QLocale().textDirection() == Qt::LeftToRight;
    const qreal pageLeft = 0;
    const qreal pageRight = viewport()->width() - DesignSystem::layout().px8();
    const qreal spaceBetweenSceneNumberAndText = DesignSystem::layout().px(10);
    const qreal textLeft = pageLeft - (isLeftToRight ? 0 : horizontalScrollMaximum())
        + document()->rootFrame()->frameFormat().leftMargin() - spaceBetweenSceneNumberAndText;
    const qreal textRight = pageRight + (isLeftToRight ? horizontalScrollMaximum() : 0)
        - document()->rootFrame()->frameFormat().rightMargin() + spaceBetweenSceneNumberAndText;
    const qreal leftDelta = (isLeftToRight ? -1 : 1) * horizontalScroll();
    qreal verticalMargin = 0;


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
    while (TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading1
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading2
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading3
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading4
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading5
           && TextBlockStyle::forBlock(topBlock) != TextParagraphType::ChapterHeading6
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
    // Прорисовка дополнительных элементов редактора
    //
    QPainter painter(viewport());
    {
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
            bool isLastBlockSceneHeadingWithNumberAtRight = false;
            //
            auto setPainterPen = [&painter, &block, this](const QColor& _color) {
                painter.setPen(ColorHelper::transparent(
                    _color,
                    1.0
                        - (isFocusCurrentParagraph() && block != textCursor().block()
                               ? DesignSystem::inactiveTextOpacity()
                               : 0.0)));
            };

            QTextCursor cursor(document());
            while (block.isValid() && block != bottomBlock) {
                //
                // Стиль текущего блока
                //
                const auto blockType = TextBlockStyle::forBlock(block);

                cursor.setPosition(block.position());
                const QRect cursorR = cursorRect(cursor);
                cursor.movePosition(QTextCursor::EndOfBlock);
                const QRect cursorREnd = cursorRect(cursor);
                //
                verticalMargin = cursorR.height() / 2;

                //
                // Определим цвет главы
                //

                switch (blockType) {
                case TextParagraphType::ChapterHeading1:
                case TextParagraphType::ChapterHeading2:
                case TextParagraphType::ChapterHeading3:
                case TextParagraphType::ChapterHeading4:
                case TextParagraphType::ChapterHeading5:
                case TextParagraphType::ChapterHeading6: {
                    previousSceneBlockBottom = lastSceneBlockBottom;
                    lastSceneBlockBottom = cursorR.top();
                    lastSceneColors = d->document.itemColors(block);
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
                    const QPointF topLeft(isLeftToRight
                                              ? pageRight + leftDelta - DesignSystem::layout().px4()
                                              : pageLeft + leftDelta,
                                          lastSceneBlockBottom - verticalMargin);
                    const QPointF bottomRight(isLeftToRight ? pageRight + leftDelta
                                                            : pageLeft + leftDelta
                                                      + DesignSystem::layout().px4(),
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
                                         ColorHelper::transparent(
                                             bookmark.color, DesignSystem::elevationEndOpacity()));
                        painter.drawText(rect, Qt::AlignCenter, u8"\U000F00C0");
                    }

                    //
                    // Прорисовка декораций пустой строки
                    //
                    if (block.text().simplified().isEmpty()) {
                        //
                        // Настроим цвет
                        //
                        setPainterPen(ColorHelper::transparent(
                            palette().text().color(), DesignSystem::inactiveItemOpacity()));

                        //
                        // Рисуем индикатор пустой строки
                        //
                        painter.setFont(block.charFormat().font());
                        const QString emptyLineMark = "» ";
                        //
                        // Определим область для отрисовки и выведем символ в редактор
                        //
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
                    // Прорисовка декораций непустых строк
                    //
                    else {

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
                                    = format.format
                                          .property(TextBlockStyle::PropertyCommentsIsRevision)
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
                                        const auto revisionColor
                                            = format.format.foreground().color();
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
                }

                block = block.next();
            }
        }
    }

    //
    // Курсоры соавторов
    //
    painter.setClipRect(QRectF(), Qt::NoClip);
    paintCollaboratorsCursors(painter, d->model->document()->uuid(), topBlock, bottomBlock);
}

ContextMenu* SimpleTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
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
        connect(createBookmark, &QAction::triggered, this, &SimpleTextEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this, &SimpleTextEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &SimpleTextEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this, &SimpleTextEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(bookmarkAction);
    menu->setActions(actions);

    return menu;
}

bool SimpleTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* SimpleTextEdit::createMimeDataFromSelection() const
{
    if (!textCursor().hasSelection()) {
        return {};
    }

    QMimeData* mimeData = new QMimeData;
    TextCursor cursor = textCursor();
    const auto selection = cursor.selectionInterval();

    //
    // Сформируем в текстовом виде, для вставки наружу
    //
    {
        QString text;
        auto cursor = textCursor();
        cursor.setPosition(selection.from);
        do {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (cursor.position() > selection.to) {
                cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
            }
            if (!text.isEmpty()) {
                text.append("\r\n\r\n");
            }
            const auto selectedText = cursor.selectedText().replace(QChar::LineSeparator, "\r\n");
            text.append(cursor.blockCharFormat().fontCapitalization() == QFont::AllUppercase
                            ? TextHelper::smartToUpper(selectedText)
                            : selectedText);
        } while (cursor.position() < textCursor().selectionEnd() && !cursor.atEnd()
                 && cursor.movePosition(QTextCursor::NextBlock));

        mimeData->setData("text/plain", text.toUtf8());
    }

    //
    // Добавим markdown
    //
    {
        //
        // Подготавливаем опции для экспорта в markdown
        //
        BusinessLayer::ExportOptions options;
        options.filePath = QDir::temp().absoluteFilePath("clipboard.md");
        options.includeTitlePage = false;
        options.includeReviewMarks = false;
        //
        // ... сохраняем в формате markdown
        //
        BusinessLayer::SimpleTextMarkdownExporter().exportTo(d->model, selection.from, selection.to,
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
        mimeData->setData(d->model->mimeTypes().first(),
                          d->document.mimeFromSelection(selection.from, selection.to).toUtf8());
    }

    return mimeData;
}

void SimpleTextEdit::insertFromMimeData(const QMimeData* _source)
{
    if (isReadOnly()) {
        return;
    }

    //
    // Удаляем выделенный текст
    //
    TextCursor cursor = textCursor();
    if (cursor.hasSelection()) {
        cursor.removeSelectedText();
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
    else if (_source->hasFormat(kMarkdownMimeType) || _source->hasText()) {
        const auto text = _source->hasFormat(kMarkdownMimeType) ? _source->data(kMarkdownMimeType)
                                                                : _source->text();

        //
        // ... если строк несколько, то вставляем его, импортировав с фонтана
        // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
        //       не воспринимался как титульная страница
        //
        BusinessLayer::SimpleTextMarkdownImporter markdownImporter;
        textToInsert = markdownImporter.importSimpleText(text).text;
    }

    //
    // Собственно вставка данных
    //
    auto cursorPosition = d->document.insertFromMime(textCursor().position(), textToInsert);

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

void SimpleTextEdit::dropEvent(QDropEvent* _event)
{
    //
    // Если в момент вставки было выделение
    //
    if (textCursor().hasSelection()) {
        QTextCursor cursor = textCursor();
        //
        // ... и это перемещение содержимого внутри редактора
        //
        if (_event->source() == this) {
            //
            // ... то удалим выделенный текст
            //
            cursor.removeSelectedText();
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

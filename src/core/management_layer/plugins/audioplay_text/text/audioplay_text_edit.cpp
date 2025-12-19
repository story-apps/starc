#include "audioplay_text_edit.h"

#include "handlers/key_press_handler_facade.h"

#include <business_layer/document/audioplay/text/audioplay_text_corrector.h>
#include <business_layer/document/audioplay/text/audioplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/audioplay/audioplay_export_options.h>
#include <business_layer/export/audioplay/audioplay_fountain_exporter.h>
#include <business_layer/import/audioplay/audioplay_fountain_importer.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_block_parser.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/model_helper.h>
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
#include <QTimer>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;

namespace Ui {

namespace {
const QLatin1String kMarkdownMimeType("text/markdown");
}

class AudioplayTextEdit::Implementation
{
public:
    explicit Implementation(AudioplayTextEdit* _q);

    /**
     * @brief Текущий шаблон документа
     */
    const BusinessLayer::AudioplayTemplate& audioplayTemplate() const;

    /**
     * @brief Отменить/повторить последнее действие
     */
    void revertAction(bool previous);

    /**
     * @brief Обновить редакторскую заметку в заданном интервале
     */
    void updateReviewMark(QKeyEvent* _event, int _from, int _to);


    AudioplayTextEdit* q = nullptr;

    QPointer<BusinessLayer::AudioplayTextModel> model;
    BusinessLayer::AudioplayTextDocument document;

    bool showBlockNumbers = false;
    bool continueBlockNumber = false;

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

AudioplayTextEdit::Implementation::Implementation(AudioplayTextEdit* _q)
    : q(_q)
{
}

const BusinessLayer::AudioplayTemplate& AudioplayTextEdit::Implementation::audioplayTemplate() const
{
    const auto currentTemplateId
        = model && model->informationModel() ? model->informationModel()->templateId() : "";
    return TemplatesFacade::audioplayTemplate(currentTemplateId);
}

void AudioplayTextEdit::Implementation::revertAction(bool previous)
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

void AudioplayTextEdit::Implementation::updateReviewMark(QKeyEvent* _event, int _from, int _to)
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


AudioplayTextEdit::AudioplayTextEdit(QWidget* _parent)
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
            &AudioplayTextEdit::updateCollaboratorsCursors);
    connect(this, &AudioplayTextEdit::completed, this,
            [this](const QModelIndex& _index, int _from, int _to) {
                Q_UNUSED(_index)
                d->updateReviewMark(nullptr, _from, _to);
            });
}

AudioplayTextEdit::~AudioplayTextEdit() = default;

void AudioplayTextEdit::setShowBlockNumbers(bool _show, bool _continue)
{
    d->showBlockNumbers = _show;
    d->continueBlockNumber = _continue;
    update();
}

void AudioplayTextEdit::setCorrectionOptions(bool _needToCorrectPageBreaks)
{
    d->document.setCorrectionOptions(_needToCorrectPageBreaks);
}

void AudioplayTextEdit::initWithModel(BusinessLayer::AudioplayTextModel* _model)
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
        const auto& currentTemplate = d->audioplayTemplate();
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

        connect(d->model, &BusinessLayer::AudioplayTextModel::dataChanged, this,
                qOverload<>(&AudioplayTextEdit::update));
        connect(d->model->informationModel(),
                &BusinessLayer::AudioplayInformationModel::headerChanged, this,
                &AudioplayTextEdit::setHeader);
        connect(d->model->informationModel(),
                &BusinessLayer::AudioplayInformationModel::footerChanged, this,
                &AudioplayTextEdit::setFooter);
        //
        // Корректируем позицию курсора при совместной работе
        //
        connect(d->model, &BusinessLayer::AudioplayTextModel::changesAboutToBeApplied, this,
                [this] {
                    d->lastPosition.inBlock = textCursor().positionInBlock();
                    d->lastPosition.inDocument = textCursor().position();
                    d->lastPosition.blockLength = textCursor().block().length();
                });
        connect(d->model, &BusinessLayer::AudioplayTextModel::changesApplied, this,
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

void AudioplayTextEdit::reinit()
{
    //
    // Перенастроим всё, что зависит от шаблона
    //
    initWithModel(d->model);

    //
    // Пересчитаем всё, что считается во время выполнения
    //
    if (d->model != nullptr) {
        d->model->recalculateCounters();
        d->model->updateRuntimeDictionaries();
    }
}

const BusinessLayer::AudioplayTemplate& AudioplayTextEdit::audioplayTemplate() const
{
    return d->audioplayTemplate();
}

QAbstractItemModel* AudioplayTextEdit::characters() const
{
    if (d->model == nullptr) {
        return nullptr;
    }

    return d->model->charactersList();
}

void AudioplayTextEdit::createCharacter(const QString& _name)
{
    if (d->model == nullptr) {
        return;
    }

    d->model->createCharacter(_name);
}

void AudioplayTextEdit::undo()
{
    d->revertAction(true);
}

void AudioplayTextEdit::redo()
{
    d->revertAction(false);
}

void AudioplayTextEdit::addParagraph(BusinessLayer::TextParagraphType _type)
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
    // Вставляем параграф на уровне модели
    //
    d->document.addParagraph(_type, textCursor());

    //
    // ... при необходимости восстанавливаем режим изоляции
    //
    if (needReisolate) {
        d->document.setVisibleTopLevelItem(d->document.itemIndex(textCursor().block()));
    }

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

    //
    // Вставляем вырезанные данные
    //
    if (!mimeDataToMove.isEmpty()) {
        d->document.insertFromMime(textCursor().position(), mimeDataToMove);
    }

    emit paragraphTypeChanged();
}

void AudioplayTextEdit::setCurrentParagraphType(TextParagraphType _type)
{
    if (currentParagraphType() == _type) {
        return;
    }

    BusinessLayer::TextCursor cursor = textCursor();

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
    // Меняем тип блока на персонажа
    //
    if (_type == TextParagraphType::Character) {
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
    else if (_type == TextParagraphType::Dialogue) {
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
        // Если текущий блок в табилце, ничего не делаем
        //
        else {
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

BusinessLayer::TextParagraphType AudioplayTextEdit::currentParagraphType() const
{
    return TextBlockStyle::forBlock(textCursor().block());
}

QModelIndex AudioplayTextEdit::currentModelIndex() const
{
    if (d->model == nullptr || d->document.isEditTransactionActive()) {
        return {};
    }

    auto userData = textCursor().block().userData();
    if (userData == nullptr) {
        return {};
    }

    auto audioplayBlockData = static_cast<BusinessLayer::TextBlockData*>(userData);
    return d->model->indexForItem(audioplayBlockData->item());
}

void AudioplayTextEdit::setCurrentModelIndex(const QModelIndex& _index)
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

    //
    // В кейсе с битами мы попадаем на невидимый блок, но интересует нас следующий за ним -
    // первый параграф бита
    //
    if (!textCursor.block().isVisible()) {
        textCursor.movePosition(BusinessLayer::TextCursor::NextBlock);
    }

    ensureCursorVisible(textCursor);
}

void AudioplayTextEdit::setVisibleTopLevelItemIndex(const QModelIndex& _index)
{
    d->document.setVisibleTopLevelItem(_index);
}

int AudioplayTextEdit::positionForModelIndex(const QModelIndex& _index)
{
    return d->document.itemStartPosition(_index);
}

void AudioplayTextEdit::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
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

void AudioplayTextEdit::setAutoReviewModeEnabled(bool _enabled)
{
    d->autoReviewMode.isEnabled = _enabled;
}

void AudioplayTextEdit::setAutoReviewMode(const QColor& _textColor, const QColor& _backgroundColor,
                                          bool _isRevision, bool _isTrackChanges)
{
    d->autoReviewMode.textColor
        = _textColor.isValid() ? _textColor : ColorHelper::contrasted(_backgroundColor);
    d->autoReviewMode.backgroundColor = _backgroundColor;
    d->autoReviewMode.isRevision = _isRevision;
    d->autoReviewMode.isTrackChanges = _isTrackChanges;
}

void AudioplayTextEdit::keyPressEvent(QKeyEvent* _event)
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

bool AudioplayTextEdit::keyPressEventReimpl(QKeyEvent* _event)
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

void AudioplayTextEdit::paintEvent(QPaintEvent* _event)
{
    if (d->model.isNull()) {
        return;
    }

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
    const qreal splitterX = leftDelta + textLeft
        + (textRight - textLeft) * d->audioplayTemplate().leftHalfOfPageWidthPercents() / 100;


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
        int previousSceneBlockBottom = 0;
        int lastSceneBlockBottom = 0;
        QVector<QColor> lastSceneColors;
        QColor lastCharacterColor;
        int lastCharacterColorWithNumberRectBottom = 0;

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
            switch (blockType) {
            case TextParagraphType::SceneHeading:
            case TextParagraphType::SequenceHeading:
            case TextParagraphType::ActHeading:
            case TextParagraphType::SequenceFooter:
            case TextParagraphType::ActFooter: {
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
                const QPointF bottomRight(isLeftToRight
                                              ? pageRight + leftDelta
                                              : pageLeft + leftDelta + DesignSystem::layout().px4(),
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
                && d->model->charactersList() != nullptr) {
                lastCharacterColor = QColor();
                lastCharacterColorWithNumberRectBottom = 0;
                const QString characterName
                    = BusinessLayer::AudioplayCharacterParser::name(block.text());
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
                if (d->audioplayTemplate()
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
                // Прорисовка тайтлов блоков
                //
                const auto& blockStyle = d->audioplayTemplate().paragraphStyle(blockType);
                if (blockStyle.isTitleVisible()) {
                    setPainterPen(palette().text().color());
                    painter.setFont(cursor.charFormat().font());

                    //
                    // Определим область для отрисовки (отступы используем от стиля персонажа)
                    //
                    const auto& characterStyle
                        = d->audioplayTemplate().paragraphStyle(TextParagraphType::Character);
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
                    setPainterPen(ColorHelper::transparent(palette().text().color(),
                                                           DesignSystem::inactiveItemOpacity()));

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
                        const auto placeholderText = QString("%1: %2").arg(
                            blockType == TextParagraphType::ActFooter ? tr("End of act")
                                                                      : tr("End of folder"),
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
                            / 2.0;
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
                            if (d->audioplayTemplate()
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
            }

            block = block.next();
        }
    }

    //
    // Курсоры соавторов
    //
    painter.setClipRect(QRectF(), Qt::NoClip);
    paintCollaboratorsCursors(painter, d->model->document()->uuid(), topBlock, bottomBlock);
}

ContextMenu* AudioplayTextEdit::createContextMenu(const QPoint& _position, QWidget* _parent)
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
                &AudioplayTextEdit::addBookmarkRequested);
    } else {
        auto editBookmark = new QAction(bookmarkAction);
        editBookmark->setText(tr("Edit"));
        editBookmark->setIconText(u8"\U000F03EB");
        connect(editBookmark, &QAction::triggered, this, &AudioplayTextEdit::editBookmarkRequested);
        //
        auto removeBookmark = new QAction(bookmarkAction);
        removeBookmark->setText(tr("Remove"));
        removeBookmark->setIconText(u8"\U000F01B4");
        connect(removeBookmark, &QAction::triggered, this,
                &AudioplayTextEdit::removeBookmarkRequested);
    }
    //
    auto showBookmarks = new QAction(bookmarkAction);
    showBookmarks->setText(tr("Show/hide list"));
    showBookmarks->setIconText(u8"\U000F0E16");
    connect(showBookmarks, &QAction::triggered, this, &AudioplayTextEdit::showBookmarksRequested);

    auto actions = menu->actions().toVector();
    actions.first()->setSeparator(true);
    actions.prepend(bookmarkAction);
    menu->setActions(actions);

    return menu;
}

bool AudioplayTextEdit::canInsertFromMimeData(const QMimeData* _source) const
{
    return _source->formats().contains(d->model->mimeTypes().first()) || _source->hasText();
}

QMimeData* AudioplayTextEdit::createMimeDataFromSelection() const
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
        BusinessLayer::AudioplayExportOptions options;
        options.filePath = QDir::temp().absoluteFilePath("clipboard.fountain");
        options.includeTitlePage = false;
        options.includeSynopsis = false;
        //        options.showScenesNumbers = d->model->informationModel()->showSceneNumbers();
        //
        // ... сохраняем в формате фонтана
        //
        BusinessLayer::AudioplayFountainExporter().exportTo(d->model, selection.from, selection.to,
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

void AudioplayTextEdit::insertFromMimeData(const QMimeData* _source)
{
    using namespace BusinessLayer;

    if (isReadOnly()) {
        return;
    }

    //
    // Удаляем выделенный текст
    //
    TextCursor cursor = textCursor();
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
    else if (_source->hasFormat(kMarkdownMimeType) || _source->hasText()) {
        const auto text = _source->hasFormat(kMarkdownMimeType) ? _source->data(kMarkdownMimeType)
                                                                : _source->text();

        //
        // ... если строк несколько, то вставляем его, импортировав с фонтана
        // NOTE: Перед текстом нужно обязательно добавить перенос строки, чтобы он
        //       не воспринимался как титульная страница
        //
        AudioplayFountainImporter fountainImporter;
        textToInsert
            = fountainImporter.audioplayText("\n" + TextHelper::removeControlCharacters(text)).text;
    }

    //
    // Если курсор установлен в таблице
    //
    if (cursor.inTable()) {
        const auto mimeInfo = ModelHelper::isMimeHasJustOneBlock(textToInsert);
        const auto isMimeContainsJustOneBlock = mimeInfo.first;
        const auto isMimeContainsFolderOrSequence = mimeInfo.second;
        //
        // ... если вставляется несколько блоков
        //
        if (!isMimeContainsJustOneBlock || isMimeContainsFolderOrSequence) {
            bool isTableEmpty = true;
            while (TextBlockStyle::forCursor(cursor) != TextParagraphType::PageSplitter) {
                cursor.movePosition(TextCursor::PreviousBlock);
            }
            const auto tableBeginningPosition = cursor.position();
            cursor.movePosition(TextCursor::NextBlock);
            while (TextBlockStyle::forCursor(cursor) != TextParagraphType::PageSplitter) {
                if (!cursor.block().text().isEmpty()) {
                    isTableEmpty = false;
                    break;
                }
                cursor.movePosition(TextCursor::NextBlock);
            }

            //
            // ... если таблица пуста, то удалим её
            //
            if (isTableEmpty) {
                cursor.setPosition(tableBeginningPosition, TextCursor::KeepAnchor);
                cursor.removeSelectedText();
                setCurrentParagraphType(TextParagraphType::Cue);
            }
            //
            // ... а если не пуста, то выходим из таблицы и будем производить вставку после неё
            //
            else {
                while (TextBlockStyle::forCursor(cursor) != TextParagraphType::PageSplitter) {
                    cursor.movePosition(TextCursor::NextBlock);
                }
                setTextCursor(cursor);
                addParagraph(TextParagraphType::Cue);
            }
        }
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

void AudioplayTextEdit::dropEvent(QDropEvent* _event)
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

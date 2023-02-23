#include "text_document.h"

#include "abstract_text_corrector.h"
#include "text_block_data.h"
#include "text_cursor.h"

#include <business_layer/model/text/text_model.h>
#include <business_layer/model/text/text_model_folder_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/model/text/text_model_splitter_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>
#include <utils/tools/debouncer.h>

#include <QDateTime>
#include <QPointer>
#include <QScopedValueRollback>
#include <QTextTable>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;


namespace BusinessLayer {

enum class DocumentState { Undefined, Loading, Changing, Correcting, Ready };


class TextDocument::Implementation
{
public:
    explicit Implementation(TextDocument* _document);

    /**
     * @brief Получить шаблон оформления текущего документа
     */
    const TextTemplate& documentTemplate() const;

    /**
     * @brief Получить элемент блока
     */
    TextModelItem* itemFor(const QTextBlock& _block) const;
    TextModelItem* itemFor(const TextCursor& _cursor) const;

    /**
     * @brief Скорректировать позиции элементов на заданную дистанцию
     */
    void correctPositionsToItems(std::map<int, TextModelItem*>::iterator _from, int _distance);
    void correctPositionsToItems(int _fromPosition, int _distance);

    /**
     * @brief Считать содержимое элмента модели с заданным индексом
     *        и вставить считанные данные в текущее положение курсора
     */
    void readModelItemContent(int _itemRow, const QModelIndex& _parent, TextCursor& _cursor,
                              bool& _isFirstParagraph);

    /**
     * @brief Считать содержимое вложенных в заданный индекс элементов
     *        и вставить считанные данные в текущее положение курсора
     */
    void readModelItemsContent(const QModelIndex& _parent, TextCursor& _cursor,
                               bool& _isFirstParagraph);

    /**
     * @brief Скорректировать документ, если это возможно
     */
    void tryToCorrectDocument();


    /**
     * @brief Владелец
     */
    TextDocument* q = nullptr;

    /**
     * @brief Текущее состояние документа
     */
    DocumentState state = DocumentState::Undefined;
    QPointer<BusinessLayer::TextModel> model;
    bool canChangeModel = true;
    std::map<int, TextModelItem*> positionsToItems;
    QScopedPointer<AbstractTextCorrector> corrector;

    /**
     * @brief Дебаунсер для корректировки текста после изменений в модели
     * @note Тут нужен именно дебаунсинг, т.к. некоторые изменения в модели могут быть
     *       несгруппированными, поэтому чтобы на каждое изменение не делать корректировки
     *       группируем их в конце очереди событий
     */
    Debouncer modelChangeCorrectionDebouncer;
};

TextDocument::Implementation::Implementation(TextDocument* _document)
    : q(_document)
    , modelChangeCorrectionDebouncer(0)
{
}

const TextTemplate& TextDocument::Implementation::documentTemplate() const
{
    return TemplatesFacade::textTemplate(model);
}

TextModelItem* TextDocument::Implementation::itemFor(const QTextBlock& _block) const
{
    if (_block.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_block.userData());
    if (blockData == nullptr) {
        return {};
    }

    return blockData->item();
}

TextModelItem* TextDocument::Implementation::itemFor(const TextCursor& _cursor) const
{
    return itemFor(_cursor.block());
}

void TextDocument::Implementation::correctPositionsToItems(
    std::map<int, TextModelItem*>::iterator _from, int _distance)
{
    if (_from == positionsToItems.end()) {
        return;
    }

    if (_distance > 0) {
        auto reversed = [](std::map<int, TextModelItem*>::iterator iter) {
            return std::prev(std::make_reverse_iterator(iter));
        };
        for (auto iter = positionsToItems.rbegin(); iter != std::make_reverse_iterator(_from);
             ++iter) {
            auto itemToUpdate = positionsToItems.extract(iter->first);
            itemToUpdate.key() = itemToUpdate.key() + _distance;
            iter = reversed(positionsToItems.insert(std::move(itemToUpdate)).position);
        }
    } else if (_distance < 0) {
        for (auto iter = _from; iter != positionsToItems.end(); ++iter) {
            auto itemToUpdate = positionsToItems.extract(iter->first);
            itemToUpdate.key() = itemToUpdate.key() + _distance;
            iter = positionsToItems.insert(std::move(itemToUpdate)).position;
        }
    }
}

void TextDocument::Implementation::correctPositionsToItems(int _fromPosition, int _distance)
{
    correctPositionsToItems(positionsToItems.lower_bound(_fromPosition), _distance);
}

void TextDocument::Implementation::readModelItemContent(int _itemRow, const QModelIndex& _parent,
                                                        TextCursor& _cursor,
                                                        bool& _isFirstParagraph)
{
    const auto itemIndex = model->index(_itemRow, 0, _parent);
    const auto item = model->itemForIndex(itemIndex);
    switch (item->type()) {
    case TextModelItemType::Folder: {
        break;
    }

    case TextModelItemType::Group: {
        break;
    }

    case TextModelItemType::Splitter: {
        const auto splitterItem = static_cast<TextModelSplitterItem*>(item);
        switch (splitterItem->splitterType()) {
        case TextModelSplitterItemType::Start: {
            //
            // Если это не первый абзац, вставим блок для него
            //
            if (!_isFirstParagraph) {
                _cursor.insertBlock();
            }
            //
            // ... в противном же случае, новый блок нет необходимости вставлять
            //
            else {
                _isFirstParagraph = false;
            }

            //
            // Скорректируем позиции последующих элементов
            //
            correctPositionsToItems(_cursor.position(), 4);

            //
            // Запомним позицию разделителя
            //
            positionsToItems.emplace(_cursor.position(), splitterItem);

            //
            // Назначим блоку перед таблицей формат PageSplitter
            //
            auto insertPageSplitter = [&_cursor, this] {
                const auto style
                    = documentTemplate().paragraphStyle(TextParagraphType::PageSplitter);
                _cursor.setBlockFormat(style.blockFormat());
                _cursor.setBlockCharFormat(style.charFormat());
                _cursor.setCharFormat(style.charFormat());
            };
            insertPageSplitter();
            //
            // ... и сохраним данные блока
            //
            auto blockData = new TextBlockData(splitterItem);
            _cursor.block().setUserData(blockData);

            //
            // Вставляем таблицу
            //
            q->insertTable(_cursor);

            //
            // Назначим блоку после таблицы формат PageSplitter
            //
            insertPageSplitter();

            //
            // После вставки таблицы нужно завершить транзакцию изменения документа,
            // чтобы корректно считывались таблицы в положении курсора
            // FIXME: у больших документов это приводит к долгой работе, особенно в маке
            //
            _cursor.restartEditBlock();

            //
            // Помещаем курсор в первую ячейку для дальнейшего наполнения
            //
            _cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, 2);
            //
            // ... и помечаем, что вставлять новый блок нет необходимости
            //
            _isFirstParagraph = true;

            break;
        }

        case TextModelSplitterItemType::End: {
            //
            // Блок завершения таблицы уже был вставлен, при вставке начала и самой таблицы,
            // поэтому тут лишь сохраняем его в карту позиций элементов
            //
            _cursor.movePosition(QTextCursor::NextBlock);
            positionsToItems.emplace(_cursor.position(), splitterItem);
            //
            // ... и сохраняем в блоке информацию об элементе
            //
            auto blockData = new TextBlockData(splitterItem);
            _cursor.block().setUserData(blockData);

            break;
        }

        default:
            break;
        }

        break;
    }

    case TextModelItemType::Text: {
        const auto textItem = static_cast<TextModelTextItem*>(item);

        //
        // Если новый блок должен быть в другой колонке
        //
        if (_cursor.inTable() && _cursor.inFirstColumn()) {
            //
            // Если флаг размещения в таблице ещё не задан, то оставляем в той колонке,
            // в которой работаем в данный момент
            //
            if (textItem->isInFirstColumn().value_or(true) == false) {
                //
                // Переходим к следующей колонке
                //
                _cursor.movePosition(QTextCursor::NextBlock);
                //
                // ... и помечаем, что вставлять новый блок нет необходимости
                //
                _isFirstParagraph = true;
            }
        }

        //
        // Если вставляемый элемент должен быть в таблице, а курсор стоит на разделителе
        //
        if (auto item = itemFor(_cursor);
            item != nullptr && item->type() == TextModelItemType::Splitter) {
            const auto splitterItem = static_cast<TextModelSplitterItem*>(item);
            if (splitterItem->splitterType() == TextModelSplitterItemType::Start
                && textItem->isInFirstColumn().has_value()) {
                //
                // Зайдём внутрь таблицы
                //
                _cursor.movePosition(TextCursor::NextBlock);
                //
                // ... и пометим, что вставлять новый блок нет нужны
                //
                _isFirstParagraph = true;
            }
        }

        //
        // При корректировке положений блоков нужно учитывать перенос строки
        //
        int additionalDistance = 1;
        //
        // Если это не первый абзац, вставим блок для него
        //
        if (!_isFirstParagraph) {
            _cursor.insertBlock();
        }
        //
        // ... в противном же случае, новый блок нет необходимости вставлять
        //
        else {
            _isFirstParagraph = false;
            additionalDistance = 0;
        }

        //
        // Запомним позицию элемента
        //
        correctPositionsToItems(_cursor.position(), textItem->text().length() + additionalDistance);
        positionsToItems.emplace(_cursor.position(), textItem);

        //
        // Установим стиль блока
        //
        const auto currentStyle = documentTemplate().paragraphStyle(textItem->paragraphType());
        _cursor.setBlockFormat(currentStyle.blockFormat(_cursor.inTable()));
        _cursor.setBlockCharFormat(currentStyle.charFormat());
        _cursor.setCharFormat(currentStyle.charFormat());

        //
        // Для докараций, добавим дополнительные флаги
        //
        auto decorationFormat = _cursor.block().blockFormat();
        if (textItem->isCorrection()) {
            decorationFormat.setProperty(TextBlockStyle::PropertyIsCorrection, true);
            decorationFormat.setProperty(PageTextEdit::PropertyDontShowCursor, true);
        }
        if (textItem->isCorrectionContinued()) {
            decorationFormat.setProperty(TextBlockStyle::PropertyIsCorrectionContinued, true);
            decorationFormat.setTopMargin(0);
        }
        if (textItem->isBreakCorrectionStart()) {
            decorationFormat.setProperty(TextBlockStyle::PropertyIsBreakCorrectionStart, true);
        }
        if (textItem->isBreakCorrectionEnd()) {
            decorationFormat.setProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd, true);
        }
        _cursor.setBlockFormat(decorationFormat);

        //
        // ... выравнивание
        //
        if (textItem->alignment().has_value()) {
            auto blockFormat = _cursor.blockFormat();
            blockFormat.setAlignment(textItem->alignment().value());
            _cursor.setBlockFormat(blockFormat);
        }

        //
        // Вставим текст абзаца
        //
        const auto textToInsert = TextHelper::fromHtmlEscaped(textItem->text());
        _cursor.insertText(textToInsert);

        //
        // Вставим данные блока
        //
        auto blockData = new TextBlockData(textItem);
        _cursor.block().setUserData(blockData);

        //
        // Вставим форматирование
        //
        auto formatCursor = _cursor;
        for (const auto& format : textItem->formats()) {
            formatCursor.setPosition(formatCursor.block().position() + format.from);
            formatCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                      format.length);
            formatCursor.mergeCharFormat(format.charFormat());
        }

        //
        // Вставим редакторские заметки
        //
        auto reviewCursor = _cursor;
        for (const auto& reviewMark : textItem->reviewMarks()) {
            reviewCursor.setPosition(reviewCursor.block().position() + reviewMark.from);
            reviewCursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                      reviewMark.length);
            reviewCursor.mergeCharFormat(reviewMark.charFormat());
        }

        break;
    }

    default: {
        Q_ASSERT(false);
        break;
    }
    }
}

void TextDocument::Implementation::readModelItemsContent(const QModelIndex& _parent,
                                                         TextCursor& _cursor,
                                                         bool& _isFirstParagraph)
{
    for (int itemRow = 0; itemRow < model->rowCount(_parent); ++itemRow) {
        readModelItemContent(itemRow, _parent, _cursor, _isFirstParagraph);

        //
        // Считываем информацию о детях
        //
        const auto itemIndex = model->index(itemRow, 0, _parent);
        readModelItemsContent(itemIndex, _cursor, _isFirstParagraph);
    }
}

void TextDocument::Implementation::tryToCorrectDocument()
{
    if (state != DocumentState::Ready || model.isNull() || corrector.isNull()) {
        return;
    }

    QScopedValueRollback temporatryState(state, DocumentState::Correcting);
    corrector->makePlannedCorrection(model->contentHash());
}


// ****


TextDocument::TextDocument(QObject* _parent)
    : QTextDocument(_parent)
    , d(new Implementation(this))
{
    connect(this, &TextDocument::contentsChange, this, &TextDocument::updateModelOnContentChange);
    connect(this, &TextDocument::contentsChanged, this, [this] { d->tryToCorrectDocument(); });
    connect(&d->modelChangeCorrectionDebouncer, &Debouncer::gotWork, this,
            [this] { d->tryToCorrectDocument(); });
}

TextDocument::~TextDocument() = default;

void TextDocument::setModel(BusinessLayer::TextModel* _model, bool _canChangeModel)
{
    d->state = DocumentState::Loading;

    if (d->model) {
        d->model->disconnect(this);
    }

    d->model = _model;
    d->canChangeModel = _canChangeModel;
    d->positionsToItems.clear();

    //
    // Сбрасываем корректор
    //
    if (d->corrector) {
        d->corrector->clear();
    }

    //
    // Аккуратно очищаем текст, чтобы не сломать форматирование самого документа
    //
    TextCursor cursor(this);
    cursor.beginEditBlock();
    cursor.select(QTextCursor::Document);
    cursor.deleteChar();
    cursor.block().setUserData(nullptr);
    cursor.endEditBlock();

    if (d->model == nullptr) {
        d->state = DocumentState::Ready;
        return;
    }

    if (d->corrector) {
        d->corrector->setTemplateId(d->documentTemplate().id());
    }

    //
    // Обновим шрифт документа, в моменте когда текста нет
    //
    const auto templateDefaultFont = d->documentTemplate().defaultFont();
    if (defaultFont() != templateDefaultFont) {
        setDefaultFont(templateDefaultFont);
    }

    //
    // Начинаем операцию вставки
    //
    cursor.beginEditBlock();

    //
    // Последовательно формируем текст документа
    //
    bool isFirstParagraph = true;
    d->readModelItemsContent({}, cursor, isFirstParagraph);

    //
    // Завершаем операцию
    //
    cursor.endEditBlock();

    //
    // Настроим соединения
    //
    connect(d->model, &TextModel::modelAboutToBeReset, this, [this] {
        //
        // При сбросе модели, делаем финт ушами
        // - сначала отключаемся от модели
        // - а потом переустанавливаем модель по сигналу завершения сброса
        //

        auto model = d->model;
        setModel(nullptr);

        connect(model, &TextModel::modelReset, this, [this, model] {
            setModel(model);
            processModelReset();
        });
    });
    connect(
        d->model, &TextModel::dataChanged, this,
        [this](const QModelIndex& _topLeft, const QModelIndex& _bottomRight,
               const QVector<int>& _roles) {
            if (d->state != DocumentState::Ready) {
                return;
            }

            if (_roles.size() == 1 && _roles.constFirst() == TextModelTextItem::TextNumberRole) {
                return;
            }

            Q_ASSERT(_topLeft == _bottomRight);

            QScopedValueRollback temporatryState(d->state, DocumentState::Changing);
            d->modelChangeCorrectionDebouncer.orderWork();

            const auto position = itemStartPosition(_topLeft);
            if (position < 0) {
                return;
            }

            const auto item = d->model->itemForIndex(_topLeft);
            if (item->type() != TextModelItemType::Text) {
                return;
            }

            const auto textItem = static_cast<TextModelTextItem*>(item);

            TextCursor cursor(this);
            cursor.setPosition(position);
            cursor.beginEditBlock();

            //
            // Если элемент расположен там, где и должен быть, обновим его
            //
            if (textItem->isInFirstColumn().has_value() == cursor.inTable()) {
                //
                // ... тип параграфа
                //
                if (TextBlockStyle::forBlock(cursor.block()) != textItem->paragraphType()) {
                    applyParagraphType(textItem->paragraphType(), cursor);
                }
                //
                // ... выравнивание
                //
                if (textItem->alignment().has_value()
                    && cursor.blockFormat().alignment() != textItem->alignment()) {
                    //
                    // ... если пользователь задал выравнивание
                    //
                    auto blockFormat = cursor.blockFormat();
                    blockFormat.setAlignment(textItem->alignment().value());
                    cursor.setBlockFormat(blockFormat);
                } else if (!textItem->alignment().has_value()
                           && cursor.blockFormat().alignment()
                               != d->documentTemplate()
                                      .paragraphStyle(textItem->paragraphType())
                                      .align()) {
                    //
                    // ... если выравнивание должно быть, как в стиле
                    //
                    auto blockFormat = cursor.blockFormat();
                    blockFormat.setAlignment(
                        d->documentTemplate().paragraphStyle(textItem->paragraphType()).align());
                    cursor.setBlockFormat(blockFormat);
                }
                //
                // ... текст
                //
                if (cursor.block().text() != textItem->text()) {
                    //
                    // Корректируем позиции всех элментов идущих за обновляемым
                    //
                    const auto distanse
                        = textItem->text().length() - cursor.block().text().length();
                    d->correctPositionsToItems(position + 1, distanse);

                    //
                    // TODO: Сделать более умный алгоритм, который заменяет только изменённые части
                    // текста
                    //
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    cursor.insertText(textItem->text());
                }
                //
                // ... редакторские заметки и форматирование
                //
                {
                    //
                    // TODO: придумать, как не перезаписывать форматирование каждый раз
                    //

                    //
                    // Сбросим текущее форматирование
                    //
                    cursor.movePosition(QTextCursor::StartOfBlock);
                    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                    const auto blockType = TextBlockStyle::forBlock(cursor.block());
                    const auto blockStyle = d->documentTemplate().paragraphStyle(blockType);
                    cursor.setBlockCharFormat(blockStyle.charFormat());
                    cursor.setCharFormat(blockStyle.charFormat());

                    //
                    // Применяем форматирование из редакторских заметок элемента
                    //
                    for (const auto& reviewMark : textItem->reviewMarks()) {
                        cursor.movePosition(QTextCursor::StartOfBlock);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                                            reviewMark.from);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                            reviewMark.length);
                        cursor.mergeCharFormat(reviewMark.charFormat());
                    }
                    for (const auto& format : textItem->formats()) {
                        cursor.movePosition(QTextCursor::StartOfBlock);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor,
                                            format.from);
                        cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor,
                                            format.length);
                        cursor.mergeCharFormat(format.charFormat());
                    }
                }
                //
                // ... разрыв
                //
                if (cursor.blockFormat().boolProperty(
                        TextBlockStyle::PropertyIsBreakCorrectionStart)
                    && !textItem->isBreakCorrectionStart()) {
                    auto clearFormat = cursor.blockFormat();
                    clearFormat.clearProperty(TextBlockStyle::PropertyIsBreakCorrectionStart);
                    cursor.setBlockFormat(clearFormat);
                    //
                    // ... в обратную сторону не делаем, т.к. это реализует сам корректор текста
                    //
                }
            }
            //
            // А если нужно скорректировать позицию элемента
            //
            else {
                //
                // ... то удалим его старую версию
                //
                cursor.movePosition(TextCursor::EndOfBlock, TextCursor::KeepAnchor);
                const auto positionsCorrectionDelta = cursor.selectedText().length();
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }
                //
                // ... удаляем блок в карте
                //
                d->positionsToItems.erase(cursor.position());
                //
                // ... и корректируем позиции элементов
                //
                d->correctPositionsToItems(cursor.position(), -1 * positionsCorrectionDelta);
                //
                // ... удалим сам блок и при этом нужно сохранить данные блока и его формат
                //
                auto block = cursor.block().previous();
                TextBlockData* blockData = nullptr;
                if (block.userData() != nullptr) {
                    blockData = new TextBlockData(static_cast<TextBlockData*>(block.userData()));
                }
                const auto blockFormat = cursor.block().previous().blockFormat();
                cursor.deletePreviousChar();
                cursor.block().setUserData(blockData);
                cursor.setBlockFormat(blockFormat);
                d->correctPositionsToItems(cursor.position(), -1);

                //
                // ... и вставим новую
                //
                if (*textItem->isInFirstColumn()) {
                    while (!cursor.inTable() || !cursor.inFirstColumn()) {
                        cursor.movePosition(TextCursor::PreviousBlock);
                        cursor.movePosition(TextCursor::EndOfBlock);
                    }
                } else {
                    while (!cursor.inTable()) {
                        cursor.movePosition(TextCursor::PreviousBlock);
                        cursor.movePosition(TextCursor::EndOfBlock);
                    }
                }
                bool isFirstParagraph
                    = TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::Undefined;
                d->readModelItemContent(_topLeft.row(), _topLeft.parent(), cursor,
                                        isFirstParagraph);
            }

            cursor.endEditBlock();
        });
    connect(d->model, &TextModel::rowsInserted, this,
            [this](const QModelIndex& _parent, int _from, int _to) {
                if (d->state != DocumentState::Ready) {
                    return;
                }

                QScopedValueRollback temporatryState(d->state, DocumentState::Changing);
                d->modelChangeCorrectionDebouncer.orderWork();

                //
                // Игнорируем добавление пустых сцен и папок
                //
                const auto item = d->model->itemForIndex(d->model->index(_from, 0, _parent));
                if ((item->type() == TextModelItemType::Folder
                     || item->type() == TextModelItemType::Group)
                    && !item->hasChildren()) {
                    return;
                }

                //
                // Определим позицию курсора откуда нужно начинать вставку
                //
                QModelIndex cursorItemIndex;
                if (_from > 0) {
                    cursorItemIndex = d->model->index(_from - 1, 0, _parent);

                    //
                    // В кейсе, когда вставляется новая папка/группа перед уже существующей и
                    // существующую нужно перенести после неё, добавляем дополнительное условие на
                    // определение позиции, т.к. у новой папки/группы ещё нет элементов и мы не
                    // знаем о её позиции, поэтому берём предыдущую, либо смотрим в конец общего
                    // родителя
                    //
                    const auto cursorItem = d->model->itemForIndex(cursorItemIndex);
                    if ((cursorItem->type() == TextModelItemType::Folder
                         || cursorItem->type() == TextModelItemType::Group)
                        && !cursorItem->hasChildren()) {
                        if (_from > 1) {
                            cursorItemIndex = d->model->index(_from - 2, 0, _parent);
                        } else {
                            cursorItemIndex
                                = d->model->index(_parent.row() - 1, 0, _parent.parent());
                        }
                    }
                } else {
                    cursorItemIndex = d->model->index(_parent.row() - 1, 0, _parent.parent());
                }
                //
                bool isFirstParagraph = !cursorItemIndex.isValid();
                const int cursorPosition = isFirstParagraph ? 0 : itemEndPosition(cursorItemIndex);
                if (cursorPosition < 0) {
                    return;
                }

                //
                // Собственно вставляем контент
                //
                TextCursor cursor(this);
                cursor.beginEditBlock();

                cursor.setPosition(cursorPosition);
                if (isFirstParagraph) {
                    //
                    // Если первый параграф и это не вставка блока в самого себя (а такое возможно
                    // как минимум в кейсе, когда в сценарии два блока - заголовок сцены и действие
                    // и заголовок сцены меняется на бит, что приводит как бы к удалению блоков из
                    // сцены и их вставке в бит), то нужно перенести блок со своими данными дальше
                    //
                    if (begin().userData() != nullptr
                        && static_cast<TextBlockData*>(begin().userData())->item() != item) {
                        TextBlockData* blockData = nullptr;
                        auto block = cursor.block();
                        if (block.userData() != nullptr) {
                            blockData
                                = new TextBlockData(static_cast<TextBlockData*>(block.userData()));
                            block.setUserData(nullptr);
                        }
                        cursor.insertBlock();
                        cursor.block().setUserData(blockData);
                        //
                        // И вернуться назад, для вставки данныхшт
                        //
                        cursor.movePosition(QTextCursor::PreviousBlock);
                        //
                        // Корректируем позиции всех блоков на один символ
                        //
                        d->correctPositionsToItems(0, 1);
                    }
                } else {
                    cursor.movePosition(QTextCursor::EndOfBlock);
                }

                for (int itemRow = _from; itemRow <= _to; ++itemRow) {
                    d->readModelItemContent(itemRow, _parent, cursor, isFirstParagraph);

                    //
                    // Считываем информацию о детях
                    //
                    const auto itemIndex = d->model->index(itemRow, 0, _parent);
                    d->readModelItemsContent(itemIndex, cursor, isFirstParagraph);
                }

                cursor.endEditBlock();
            });
    connect(
        d->model, &TextModel::rowsAboutToBeRemoved, this,
        [this](const QModelIndex& _parent, int _from, int _to) {
            if (d->state != DocumentState::Ready) {
                return;
            }

            QScopedValueRollback temporatryState(d->state, DocumentState::Changing);
            d->modelChangeCorrectionDebouncer.orderWork();

            const QModelIndex fromIndex = d->model->index(_from, 0, _parent);
            if (!fromIndex.isValid()) {
                return;
            }
            auto fromPosition = itemStartPosition(fromIndex);
            if (fromPosition < 0) {
                return;
            }

            //
            // Если происходит удаление разделителя таблицы, то удаляем таблицу,
            // а все блоки переносим за таблицу
            //
            auto item = d->model->itemForIndex(fromIndex);
            if (item->type() == TextModelItemType::Splitter) {
                auto splitterItem = static_cast<TextModelSplitterItem*>(item);

                //
                // ... убираем таблицу, если она ещё не была удалена
                //
                TextCursor cursor(this);
                cursor.setPosition(fromPosition);
                if (TextBlockStyle::forBlock(cursor.block()) == TextParagraphType::PageSplitter) {
                    int row = _from + 1;
                    if (splitterItem->splitterType() == TextModelSplitterItemType::End) {
                        //
                        // ... идём в начало таблицы
                        //
                        do {
                            cursor.movePosition(TextCursor::PreviousBlock);
                            --row;
                        } while (TextBlockStyle::forBlock(cursor)
                                 != TextParagraphType::PageSplitter);
                        fromPosition = cursor.position();
                    }
                    //
                    // ... заходим в таблицу
                    //
                    cursor.movePosition(TextCursor::NextBlock);

                    //
                    // Берём второй курсор, куда будем переносить блоки
                    //
                    auto insertCursor = cursor;
                    while (TextBlockStyle::forBlock(insertCursor.block())
                           != TextParagraphType::PageSplitter) {
                        insertCursor.movePosition(TextCursor::NextBlock);
                        insertCursor.movePosition(TextCursor::EndOfBlock);
                    }
                    bool isFirstParagraph = false;

                    //
                    // Идём по таблице
                    //
                    while (TextBlockStyle::forBlock(cursor.block())
                           != TextParagraphType::PageSplitter) {
                        //
                        // ... удаляем блок в таблице
                        //
                        cursor.movePosition(TextCursor::EndOfBlock, TextCursor::KeepAnchor);
                        const auto positionsCorrectionDelta = cursor.selectedText().length();
                        if (cursor.hasSelection()) {
                            cursor.deleteChar();
                        }
                        //
                        // ... удаляем блок в карте
                        //
                        d->positionsToItems.erase(cursor.position());
                        //
                        // ... и корректируем позиции элементов
                        //
                        d->correctPositionsToItems(cursor.position(),
                                                   -1 * positionsCorrectionDelta);

                        //
                        // ... если всё ещё в одной колонке, то удаляем текущий пустой блок
                        //
                        const auto currentBlockInFirstColumnAttribute = cursor.inFirstColumn();
                        if (cursor.movePosition(TextCursor::NextBlock, TextCursor::KeepAnchor)) {
                            if (currentBlockInFirstColumnAttribute == cursor.inFirstColumn()) {
                                cursor.deleteChar();
                                d->correctPositionsToItems(cursor.position(), -1);
                            }
                            //
                            // ... если перешли в другую колонку, то сбросим выделение
                            //
                            else {
                                cursor.clearSelection();
                            }
                        }
                        //
                        // ... если дошли до конца таблицы, то переходим в следующий блок
                        //
                        else {
                            cursor.movePosition(TextCursor::NextBlock);
                        }

                        //
                        // ... вставляем этот же блок после таблицы
                        //
                        d->readModelItemContent(row++, fromIndex.parent(), insertCursor,
                                                isFirstParagraph);
                    }

                    //
                    // Удаляем саму таблицу
                    //
                    cursor.movePosition(QTextCursor::NextBlock);
                    //
                    // ... и при этом нужно сохранить данные блока и его формат
                    //
                    TextBlockData* blockData = nullptr;
                    Q_ASSERT(cursor.block().userData());
                    if (cursor.block().userData() != nullptr) {
                        blockData = new TextBlockData(
                            static_cast<TextBlockData*>(cursor.block().userData()));
                    }
                    const auto blockFormat = cursor.block().blockFormat();
                    //
                    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
                    cursor.deletePreviousChar();
                    //
                    cursor.block().setUserData(blockData);
                    cursor.setBlockFormat(blockFormat);

                    //
                    // Корректируем карту позиций блоков
                    //
                    auto fromIter = d->positionsToItems.lower_bound(fromPosition);
                    auto endIter = std::next(fromIter, 2);
                    d->positionsToItems.erase(fromIter, endIter);

                    //
                    // Корректируем позиции элементов
                    //
                    d->correctPositionsToItems(cursor.position(), -4);
                }

                return;
            }

            //
            // Если удаляется изолированный элемент, то сбросим изоляцию
            //
            if (d->corrector->visibleTopLevelItem() == item) {
                d->corrector->setVisibleTopLevelItem(nullptr);
            }

            const QModelIndex toIndex = d->model->index(_to, 0, _parent);
            if (!toIndex.isValid()) {
                return;
            }
            const auto toPosition = itemEndPosition(toIndex);
            if (toPosition < 0) {
                return;
            }

            TextCursor cursor(this);
            cursor.setPosition(fromPosition);
            cursor.setPosition(toPosition, QTextCursor::KeepAnchor);
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (!cursor.hasSelection() && !cursor.block().text().isEmpty()) {
                return;
            }

            //
            // Корректируем карту позиций элементов
            //
            auto fromIter = d->positionsToItems.lower_bound(cursor.selectionInterval().from);
            auto endIter = d->positionsToItems.lower_bound(cursor.selectionInterval().to);
            //
            // ... если удаление заканчивается на пустом абзаце, берём следующий за ним элемент
            //
            if (cursor.block().text().isEmpty()) {
                endIter = std::next(endIter);
            }
            //
            // ... определим дистанцию занимаемую удаляемыми элементами
            //
            const auto distance = endIter->first - fromIter->first;
            //
            // ... удаляем сами элементы из карты
            //
            d->positionsToItems.erase(fromIter, endIter);
            //
            // ... корректируем позиции остальных элементов
            //
            d->correctPositionsToItems(cursor.selectionInterval().to, -1 * distance);

            cursor.beginEditBlock();
            if (cursor.hasSelection()) {
                cursor.deleteChar();
            }

            //
            // Если это самый первый блок, то нужно удалить на один символ больше, чтобы удалить
            // сам блок
            //
            if (fromPosition == 0) {
                //
                // Если это не конец документа, удалим перенос строки
                //
                if (!cursor.atEnd()) {
                    cursor.deleteChar();
                }
                //
                // А если конец (документ остаётся пустым), то затрём данные блока
                //
                else {
                    cursor.block().setUserData(nullptr);
                }
            }
            //
            // Если это не самый первый блок
            //
            else if (fromPosition > 0) {
                //
                // ... если это верхний блок в какой-либо из колонок таблицы,
                //     то берём на один символ вперёд
                //
                const bool isFirstBlockOfFirstColumn = cursor.inFirstColumn()
                    && TextBlockStyle::forBlock(cursor.block().previous())
                        == TextParagraphType::PageSplitter;
                auto previousBlockCursor = cursor;
                previousBlockCursor.movePosition(QTextCursor::PreviousBlock);
                const bool isFirstBlockOfSecondColumn
                    = !cursor.inFirstColumn() && previousBlockCursor.inFirstColumn();
                if (isFirstBlockOfFirstColumn || isFirstBlockOfSecondColumn) {
                    cursor.deleteChar();
                }
                //
                // ... в остальных случаях берём на один символ назад, чтобы удалить сам блок
                //
                else {
                    //
                    // ... и при этом нужно сохранить данные блока и его формат
                    //
                    TextBlockData* blockData = nullptr;
                    auto block = cursor.block().previous();
                    if (block.userData() != nullptr) {
                        blockData
                            = new TextBlockData(static_cast<TextBlockData*>(block.userData()));
                    }
                    const auto blockFormat = cursor.block().previous().blockFormat();
                    cursor.deletePreviousChar();
                    cursor.block().setUserData(blockData);
                    cursor.setBlockFormat(blockFormat);
                }
            }
            cursor.endEditBlock();
        });
    //
    // Группируем массовые изменения, чтобы не мелькать пользователю перед глазами
    //
    connect(d->model, &TextModel::rowsAboutToBeChanged, this,
            [this] { TextCursor(this).beginEditBlock(); });
    connect(d->model, &TextModel::rowsChanged, this, [this] {
        //
        // Завершаем групповое изменение, но при этом обходим стороной корректировки документа,
        // т.к. всё это происходило в модели и документ уже находится в синхронизированном с
        // моделью состоянии
        //
        d->state = DocumentState::Changing;
        TextCursor(this).endEditBlock();
        d->state = DocumentState::Ready;

        //
        // После того, как все изменения были выполнены, пробуем скорректировать документ
        //
        d->tryToCorrectDocument();
    });

    d->state = DocumentState::Ready;

    //
    // Корректируем документ после загрузки
    //
    if (d->corrector != nullptr) {
        d->corrector->planCorrection(0, 0, characterCount());
        d->tryToCorrectDocument();
    }
}

TextModel* TextDocument::model() const
{
    return d->model;
}

void TextDocument::setCorrectionOptions(const QStringList& _options)
{
    if (d->corrector) {
        d->corrector->setCorrectionOptions(_options);
    }
}

QModelIndex TextDocument::visibleTopLeveLItem() const
{
    if (!d->corrector) {
        return {};
    }

    return d->model->indexForItem(d->corrector->visibleTopLevelItem());
}

void TextDocument::setVisibleTopLevelItem(const QModelIndex& _index)
{
    if (!d->corrector) {
        return;
    }

    //
    // Сюда может прилететь любой индекс, но сделать видимой мы должны как минимум сцену,
    // или группу, а так же папку
    //
    auto item = _index.isValid() ? d->model->itemForIndex(_index) : nullptr;
    if (item != nullptr && item->type() != TextModelItemType::Folder) {
        if (item->type() == TextModelItemType::Text
            || item->type() == TextModelItemType::Splitter) {
            item = item->parent();
        }
        //
        // ... изолировать можно любой элемент, кроме битов
        //
        if (item != nullptr && item->type() == TextModelItemType::Group) {
            auto groupItem = static_cast<TextModelGroupItem*>(item);
            if (groupItem->groupType() == TextGroupType::Beat) {
                item = item->parent();
            }
        }
    }

    const auto previousVisibleTopLevelItem = d->corrector->visibleTopLevelItem();
    d->corrector->setVisibleTopLevelItem(item);

    //
    // Уведомляем об изменении декорации вручную, т.к. данные иконки не влияют на содержимое модели
    // по-сути и их изменение не приведёт к срабатыванию механизмов оповещения модели об изменениях
    //
    auto updateItem = [this](TextModelItem* _item) {
        if (_item == nullptr) {
            return;
        }

        const auto itemIndex = d->model->indexForItem(_item);
        emit d->model->dataChanged(itemIndex, itemIndex, { Qt::DecorationRole });
    };
    updateItem(previousVisibleTopLevelItem);
    updateItem(item);
}

int TextDocument::itemPosition(const QModelIndex& _index, bool _fromStart)
{
    auto item = d->model->itemForIndex(_index);
    if (item == nullptr) {
        return -1;
    }

    while (item->childCount() > 0) {
        item = item->childAt(_fromStart ? 0 : item->childCount() - 1);
    }
    //
    // FIXME: оптимизировать это место
    //
    for (const auto& [key, value] : d->positionsToItems) {
        if (value == item) {
            return key;
        }
    }

    return -1;
}

int TextDocument::itemStartPosition(const QModelIndex& _index)
{
    return itemPosition(_index, true);
}

int TextDocument::itemEndPosition(const QModelIndex& _index)
{
    return itemPosition(_index, false);
}

QModelIndex TextDocument::itemIndex(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    return d->model->indexForItem(blockData->item());
}

QColor TextDocument::itemColor(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr) {
        return {};
    }
    QColor color;
    if (itemParent->type() == TextModelItemType::Folder) {
        const auto folderItem = static_cast<const TextModelFolderItem*>(itemParent);
        color = folderItem->color();
    } else if (itemParent->type() == TextModelItemType::Group) {
        const auto groupItem = static_cast<const TextModelGroupItem*>(itemParent);
        color = groupItem->color();
    }

    return color;
}

QVector<QColor> TextDocument::itemColors(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr) {
        return {};
    }

    QVector<QColor> colors;
    while (itemParent != nullptr) {
        QColor itemParentColor;
        if (itemParent->type() == TextModelItemType::Folder) {
            const auto folderItem = static_cast<const TextModelFolderItem*>(itemParent);
            itemParentColor = folderItem->color();
        } else if (itemParent->type() == TextModelItemType::Group) {
            const auto groupItem = static_cast<const TextModelGroupItem*>(itemParent);
            itemParentColor = groupItem->color();
        }

        colors.prepend(itemParentColor);

        itemParent = itemParent->parent();
    }
    return colors;
}

QString TextDocument::groupTitle(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr) {
        return {};
    }
    QString title;
    if (itemParent->type() == TextModelItemType::Group) {
        const auto groupItem = static_cast<const TextModelGroupItem*>(itemParent);
        title = groupItem->title();
    }

    return title;
}

QString TextDocument::mimeFromSelection(int _fromPosition, int _toPosition) const
{
    const auto fromBlock = findBlock(_fromPosition);
    if (fromBlock.userData() == nullptr) {
        return {};
    }
    auto fromBlockData = static_cast<TextBlockData*>(fromBlock.userData());
    if (fromBlockData == nullptr) {
        return {};
    }
    const auto fromItemIndex = d->model->indexForItem(fromBlockData->item());
    const auto fromPositionInBlock = _fromPosition - fromBlock.position();

    const auto toBlock = findBlock(_toPosition);
    if (toBlock.userData() == nullptr) {
        return {};
    }
    auto toBlockData = static_cast<TextBlockData*>(toBlock.userData());
    if (toBlockData == nullptr) {
        return {};
    }
    const auto toItemIndex = d->model->indexForItem(toBlockData->item());
    const auto toPositionInBlock = _toPosition - toBlock.position();

    const bool clearUuid = true;
    return d->model->mimeFromSelection(fromItemIndex, fromPositionInBlock, toItemIndex,
                                       toPositionInBlock, clearUuid);
}

int TextDocument::insertFromMime(int _position, const QString& _mimeData)
{
    constexpr auto invalidPosition = -1;
    const auto block = findBlock(_position);
    if (block.userData() == nullptr) {
        return invalidPosition;
    }

    auto blockData = static_cast<TextBlockData*>(block.userData());
    if (blockData == nullptr) {
        return invalidPosition;
    }

    const auto itemIndex = d->model->indexForItem(blockData->item());
    const auto positionInBlock = _position - block.position();
    const auto insertedMimeLength = d->model->insertFromMime(itemIndex, positionInBlock, _mimeData);
    if (insertedMimeLength <= 0) {
        return invalidPosition;
    }

    return _position + insertedMimeLength;
}

void TextDocument::addParagraph(BusinessLayer::TextParagraphType _type, TextCursor _cursor)
{
    _cursor.beginEditBlock();

    //
    // Если параграф целиком переносится (энтер нажат перед всем текстом блока),
    // необходимо перенести данные блока с текущего на следующий
    //
    if (_cursor.block().text().leftRef(_cursor.positionInBlock()).isEmpty()
        && !_cursor.block().text().isEmpty()) {
        TextBlockData* blockData = nullptr;
        auto block = _cursor.block();
        if (block.userData() != nullptr) {
            blockData = new TextBlockData(static_cast<TextBlockData*>(block.userData()));
            block.setUserData(nullptr);
        }

        //
        // Вставим блок
        //
        _cursor.insertBlock();

        //
        // Перенесём данные блока
        //
        _cursor.block().setUserData(blockData);

        //
        // Перейдём к предыдущему абзацу
        //
        _cursor.movePosition(QTextCursor::PreviousBlock);
        //
        // ...и применим стиль к нему
        //
        applyParagraphType(_type, _cursor);
    }
    //
    // Вставляем новый блок после текущего
    //
    else {
        //
        // Вставим блок
        //
        _cursor.insertBlock();

        //
        // Применим стиль к новому блоку
        //
        applyParagraphType(_type, _cursor);
    }

    _cursor.endEditBlock();
}

void TextDocument::setParagraphType(BusinessLayer::TextParagraphType _type,
                                    const TextCursor& _cursor)
{
    const auto currentParagraphType = TextBlockStyle::forBlock(_cursor.block());
    if (currentParagraphType == _type) {
        return;
    }

    //
    // Нельзя сменить стиль конечных элементов папок
    //
    if (currentParagraphType == TextParagraphType::ActFooter
        || currentParagraphType == TextParagraphType::SequenceFooter
        || currentParagraphType == TextParagraphType::PartFooter
        || currentParagraphType == TextParagraphType::ChapterFooter) {
        return;
    }

    auto cursor = _cursor;
    cursor.beginEditBlock();

    //
    // Первым делом очищаем пользовательские данные
    //
    cursor.block().setUserData(nullptr);

    //
    // Обработаем предшествующий установленный стиль
    //
    cleanParagraphType(_cursor);

    //
    // Применим новый стиль к блоку
    //
    applyParagraphType(_type, _cursor);

    cursor.endEditBlock();
}

void TextDocument::cleanParagraphType(const TextCursor& _cursor)
{
    const auto oldBlockType = TextBlockStyle::forBlock(_cursor.block());
    if (oldBlockType != TextParagraphType::ActHeading
        && oldBlockType != TextParagraphType::SequenceHeading
        && oldBlockType != TextParagraphType::PartHeading
        && oldBlockType != TextParagraphType::ChapterHeading) {
        return;
    }

    auto cursor = _cursor;

    //
    // Удалить завершающий блок папки
    //
    cursor.movePosition(QTextCursor::NextBlock);

    // ... открытые группы на пути поиска необходимого для обновления блока
    int openedGroups = 0;
    bool isFooterUpdated = false;
    do {
        const auto currentType = TextBlockStyle::forBlock(cursor.block());
        if (currentType == TextParagraphType::ActFooter
            || currentType == TextParagraphType::SequenceFooter
            || currentType == TextParagraphType::PartFooter
            || currentType == TextParagraphType::ChapterFooter) {
            if (openedGroups == 0) {
                cursor.movePosition(QTextCursor::PreviousBlock);
                cursor.movePosition(QTextCursor::EndOfBlock);
                cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
                cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
                cursor.deleteChar();

                isFooterUpdated = true;
            } else {
                --openedGroups;
            }
        } else if (currentType == oldBlockType) {
            //
            // Встретилась новая папка
            //
            ++openedGroups;
        }

        if (cursor.atEnd()) {
            break;
        }

        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.movePosition(QTextCursor::NextBlock);
    } while (!isFooterUpdated);
}

void TextDocument::applyParagraphType(BusinessLayer::TextParagraphType _type,
                                      const TextCursor& _cursor)
{
    auto cursor = _cursor;
    cursor.beginEditBlock();

    const auto newBlockStyle = d->documentTemplate().paragraphStyle(_type);

    //
    // Обновим стили
    //
    cursor.setBlockCharFormat(newBlockStyle.charFormat());
    cursor.setBlockFormat(newBlockStyle.blockFormat(cursor.inTable()));

    //
    // Применим стиль текста ко всему блоку, выделив его,
    // т.к. в блоке могут находиться фрагменты в другом стиле
    // + сохраняем форматирование выделений
    //
    {
        cursor.movePosition(QTextCursor::StartOfBlock);

        //
        // Если в блоке есть выделения, обновляем цвет только тех частей, которые не входят в
        // выделения
        //
        QTextBlock currentBlock = cursor.block();
        if (!currentBlock.textFormats().isEmpty()) {
            const auto formats = currentBlock.textFormats();
            for (const auto& range : formats) {
                if (range.format.boolProperty(TextBlockStyle::PropertyIsReviewMark)) {
                    continue;
                }
                cursor.setPosition(currentBlock.position() + range.start);
                cursor.setPosition(cursor.position() + range.length, QTextCursor::KeepAnchor);
                cursor.mergeCharFormat(newBlockStyle.charFormat());
            }
        }
        //
        // Если выделений нет, обновляем блок целиком
        //
        else {
            cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(newBlockStyle.charFormat());
        }

        cursor.clearSelection();
    }

    //
    // Для заголовка папки нужно создать завершение
    //
    if (_type == TextParagraphType::ActHeading || _type == TextParagraphType::SequenceHeading
        || _type == TextParagraphType::PartHeading || _type == TextParagraphType::ChapterHeading) {
        const auto footerStyle = d->documentTemplate().paragraphStyle([_type] {
            switch (_type) {
            default:
            case TextParagraphType::ActHeading: {
                return TextParagraphType::ActFooter;
            }
            case TextParagraphType::SequenceHeading: {
                return TextParagraphType::SequenceFooter;
            }
            case TextParagraphType::PartHeading: {
                return TextParagraphType::PartFooter;
            }
            case TextParagraphType::ChapterHeading: {
                return TextParagraphType::ChapterFooter;
            }
            }
        }());

        //
        // Вставляем закрывающий блок после заголовка
        //
        cursor.insertBlock();
        cursor.setBlockCharFormat(footerStyle.charFormat());
        cursor.setBlockFormat(footerStyle.blockFormat(cursor.inTable()));
    }

    cursor.endEditBlock();
}

void TextDocument::splitParagraph(const TextCursor& _cursor)
{
    //
    // Получим курсор для блока, который хочет разделить пользователь
    //
    TextCursor cursor = _cursor;
    cursor.beginEditBlock();

    //
    // Сохраним текущий формат блока
    //
    const auto lastBlockType = TextBlockStyle::forBlock(findBlock(cursor.selectionInterval().from));
    //
    // Вырезаем выделение, захватывая блоки целиком
    //
    if (cursor.hasSelection()) {
        const auto selection = cursor.selectionInterval();
        cursor.setPosition(selection.from);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.setPosition(selection.to, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    //
    // ... либо только текущий блок
    //
    else {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    //
    // ... проставляем вырезаемым элементам флаг, что они будут теперь внутри таблицы
    //
    {
        auto block = findBlock(cursor.selectionStart());
        while (block.isValid() && block.position() < cursor.selectionEnd()) {
            auto item = d->positionsToItems[block.position()];
            if (item->type() == TextModelItemType::Text) {
                auto textItem = static_cast<TextModelTextItem*>(item);
                textItem->setInFirstColumn(true);
            }

            block = block.next();
        }
    }
    //
    // ... собственно вырезаем
    //
    const QString mime
        = mimeFromSelection(cursor.selectionInterval().from, cursor.selectionInterval().to);
    cursor.removeSelectedText();

    //
    // Назначим блоку перед таблицей формат PageSplitter
    //
    setParagraphType(TextParagraphType::PageSplitter, cursor);

    //
    // Вставляем таблицу
    //
    insertTable(cursor);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, 2);
    cursor.restartEditBlock();

    //
    // Применяем сохранённый формат блока каждой из колонок
    //
    setParagraphType(lastBlockType, cursor);
    cursor.movePosition(QTextCursor::NextBlock);
    setParagraphType(lastBlockType, cursor);
    cursor.movePosition(QTextCursor::NextBlock);

    //
    // Назначим блоку после таблицы формат PageSplitter
    //
    setParagraphType(TextParagraphType::PageSplitter, cursor);

    //
    // Завершаем редактирование
    //
    cursor.endEditBlock();
    //
    // ... и только после этого вставляем текст в первую колонку
    //
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, 2);
    insertFromMime(cursor.position(), mime);
}

void TextDocument::mergeParagraph(const TextCursor& _cursor)
{
    //
    // Получим курсор для блока, из которого пользователь хочет убрать разделение
    //
    TextCursor cursor = _cursor;
    cursor.movePosition(QTextCursor::StartOfBlock);
    const auto sourceBlockType = TextBlockStyle::forBlock(cursor);
    cursor.beginEditBlock();

    //
    // Идём до начала таблицы
    //
    while (TextBlockStyle::forBlock(cursor) != TextParagraphType::PageSplitter) {
        cursor.movePosition(QTextCursor::PreviousBlock);
    }

    //
    // Проставить элементам блоков в таблице флаг, что они будут теперь вне таблицы
    //
    {
        auto updateCursor = cursor;
        updateCursor.movePosition(QTextCursor::NextBlock);
        while (!updateCursor.atEnd() && updateCursor.inTable()) {
            auto item = d->positionsToItems[updateCursor.position()];
            if (item->type() == TextModelItemType::Text) {
                auto textItem = static_cast<TextModelTextItem*>(item);
                textItem->setInFirstColumn({});
            }

            updateCursor.movePosition(QTextCursor::EndOfBlock);
            updateCursor.movePosition(QTextCursor::NextBlock);
        }
    }

    //
    // Выделяем и сохраняем текст из первой ячейки
    //
    cursor.movePosition(QTextCursor::NextBlock);
    QTextTable* table = cursor.currentTable();
    while (table->cellAt(cursor).column() == 0) {
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
    }
    cursor.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor);
    const QString firstColumnData = cursor.selectedText().isEmpty()
        ? QString()
        : mimeFromSelection(cursor.selectionInterval().from, cursor.selectionInterval().to);
    cursor.removeSelectedText();

    //
    // Выделяем и сохраняем текст из второй ячейки
    //
    cursor.movePosition(QTextCursor::NextBlock);
    while (cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor))
        ;
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    const QString secondColumnData = cursor.selectedText().isEmpty()
        ? QString()
        : mimeFromSelection(cursor.selectionInterval().from, cursor.selectionInterval().to);
    cursor.removeSelectedText();

    //
    // Удаляем таблицу
    //
    // NOTE: Делается это только таким костылём, как удалить таблицу по-человечески я не нашёл...
    //
    // ... page splitter после таблицы
    cursor.movePosition(QTextCursor::NextBlock);
    // ... page splitter перед таблицей
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
    cursor.block().setUserData(nullptr);
    cursor.endEditBlock();

    //
    // Вставляем текст из удалённых ячеек
    //
    const int insertPosition = cursor.position();
    //
    // Если таблица была пуста, то создаём пустой блок, с типом исходного блока, где был курсор
    //
    if (firstColumnData.isEmpty() && secondColumnData.isEmpty()) {
        const auto blockStyle = d->documentTemplate().paragraphStyle(sourceBlockType);
        cursor.insertBlock(blockStyle.blockFormat(), blockStyle.charFormat());
    }
    //
    // В противном случае, вставляем тексты обеих колонок сохраняя порядок следования текста
    //
    else {
        if (!secondColumnData.isEmpty()) {
            cursor.insertBlock({});
            insertFromMime(cursor.position(), secondColumnData);
            cursor.setPosition(insertPosition);
        }
        if (!firstColumnData.isEmpty()) {
            cursor.insertBlock({});
            insertFromMime(cursor.position(), firstColumnData);
        }
    }

    //
    // Удаляем оставшийся от таблицы блок
    //
    cursor.setPosition(insertPosition);
    cursor.deleteChar();
}

void TextDocument::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                 const QString& _comment, const TextCursor& _cursor)
{
    TextModelTextItem::ReviewMark reviewMark;
    if (_textColor.isValid()) {
        reviewMark.textColor = _textColor;
    }
    if (_backgroundColor.isValid()) {
        reviewMark.backgroundColor = _backgroundColor;
    }
    reviewMark.comments.append({ DataStorageLayer::StorageFacade::settingsStorage()->accountName(),
                                 DataStorageLayer::StorageFacade::settingsStorage()->accountEmail(),
                                 QDateTime::currentDateTime().toString(Qt::ISODate), _comment });

    auto cursor = _cursor;
    cursor.mergeCharFormat(reviewMark.charFormat());
}

TextModelTextItem::Bookmark TextDocument::bookmark(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    const auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    const auto item = blockData->item();
    if (item == nullptr || item->type() != TextModelItemType::Text) {
        return {};
    }

    const auto textItem = static_cast<const TextModelTextItem*>(item);
    return textItem->bookmark().value_or(TextModelTextItem::Bookmark());
}

bool TextDocument::canChangeModel() const
{
    return d->canChangeModel;
}

void TextDocument::setCorrector(AbstractTextCorrector* _corrector)
{
    d->corrector.reset(_corrector);
    connect(this, &TextDocument::contentsChange, d->corrector.data(),
            &AbstractTextCorrector::planCorrection);
}

void TextDocument::processModelReset()
{
}

void TextDocument::updateModelOnContentChange(int _position, int _charsRemoved, int _charsAdded)
{
    if (d->model == nullptr) {
        return;
    }

    if (!d->canChangeModel) {
        return;
    }

    if (d->state != DocumentState::Ready && d->state != DocumentState::Correcting) {
        return;
    }

    QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

    using namespace BusinessLayer;

    auto isFolder = [](TextModelItem* _item) {
        return _item != nullptr && _item->type() == TextModelItemType::Folder;
    };
    auto isGroup = [](TextModelItem* _item) {
        return _item != nullptr && _item->type() == TextModelItemType::Group;
    };
    auto toGroup = [](TextModelItem* _item) -> TextModelGroupItem* {
        return static_cast<TextModelGroupItem*>(_item);
    };
    auto toText = [](TextModelItem* _item) -> TextModelTextItem* {
        return static_cast<TextModelTextItem*>(_item);
    };


    //
    // Удаляем из модели элементы удалённых блоков и корректируем позиции блоков идущих после правки
    //
    {
        //
        // Собираем элементы которые потенциально могут быть удалены
        //
        std::map<TextModelItem*, int> itemsToDelete;
        {
            auto itemsToDeleteIter = d->positionsToItems.lower_bound(_position);
            while (itemsToDeleteIter != d->positionsToItems.end()
                   && itemsToDeleteIter->first <= _position + _charsRemoved) {
                itemsToDelete.emplace(itemsToDeleteIter->second, itemsToDeleteIter->first);
                itemsToDeleteIter = d->positionsToItems.erase(itemsToDeleteIter);
            }

            //
            // Корректируем позиции элементов идущих за изменёнными блоками
            //

            auto itemToUpdateIter = itemsToDeleteIter;

            //
            // Формируем мапу элементов со скорректированными позициями
            //
            std::map<int, TextModelItem*> correctedItems;
            for (auto itemIter = itemToUpdateIter; itemIter != d->positionsToItems.end();
                 ++itemIter) {
                correctedItems.emplace(itemIter->first - _charsRemoved + _charsAdded,
                                       itemIter->second);
            }

            //
            // Удаляем элементы со старыми позициями
            //
            d->positionsToItems.erase(itemToUpdateIter, d->positionsToItems.end());

            //
            // И записываем на их место новые элементы
            //
            d->positionsToItems.merge(correctedItems);
        }

        //
        // Проходим по изменённым блокам и фильтруем элементы, которые не были удалены
        //
        auto block = findBlock(_position);
        while (!itemsToDelete.empty() && block.isValid()
               && block.position() <= _position + std::max(_charsRemoved, _charsAdded)) {
            if (block.userData() != nullptr) {
                const auto blockData = static_cast<TextBlockData*>(block.userData());
                const auto notRemovedItemIter = itemsToDelete.find(blockData->item());
                if (notRemovedItemIter != itemsToDelete.end()) {
                    //
                    // Восстанавливаем позицию блока с учётом смещения
                    //
                    d->positionsToItems.emplace(block.position(), notRemovedItemIter->first);
                    itemsToDelete.erase(notRemovedItemIter);
                }
            }
            block = block.next();
        }

        //
        // Удаляем блоки, которые действительно были удалены из текста
        //

        //
        // Сначала группируем и "сжимаем" блоки
        //
        std::map<int, TextModelItem*> itemsToDeleteSorted;
        for (auto [item, position] : itemsToDelete) {
            itemsToDeleteSorted.emplace(position, item);
        }
        //
        std::map<int, TextModelItem*> itemsToDeleteCompressed;
        int compressionCycle = 0;
        while (!itemsToDeleteSorted.empty()) {
            //
            // Формируем список идущих подряд элементов
            //
            struct ItemToPosition {
                TextModelItem* item;
                int position;
            };
            QVector<ItemToPosition> itemsGroup;
            do {
                const auto beginIter = itemsToDeleteSorted.cbegin();
                const auto firstItem = beginIter->second;
                if (!itemsGroup.isEmpty()) {
                    //
                    // ... прерываем группировку, если у элементов разные родители
                    //
                    const auto lastGroupItem = itemsGroup.constLast().item;
                    const auto lastGroupItemParent = lastGroupItem->parent();
                    if (lastGroupItemParent != firstItem->parent()) {
                        break;
                    }
                    //
                    // ... или если они не идут друг за другом подряд
                    //
                    const auto lastGroupItemRow = lastGroupItemParent->rowOfChild(lastGroupItem);
                    const auto firstItemRow = lastGroupItemParent->rowOfChild(firstItem);
                    if (lastGroupItemRow + 1 != firstItemRow) {
                        break;
                    }
                }

                const auto firstItemPosition = beginIter->first;
                itemsGroup.append({ firstItem, firstItemPosition });
                itemsToDeleteSorted.erase(beginIter);
            } while (!itemsToDeleteSorted.empty());

            //
            // Если элемент удаляется целиком, то его и запишем на удаление,
            // за исключением случая, если это самый верхнеуровневый элемент
            //
            if (itemsGroup.constFirst().item->hasParent()
                && itemsGroup.constFirst().item->parent()->childCount() == itemsGroup.size()
                && itemsGroup.constFirst().item->parent()->hasParent()) {
                itemsToDeleteCompressed.emplace(itemsGroup.constFirst().position,
                                                itemsGroup.constFirst().item->parent());
            }
            //
            // В противном случае будем удалять поэлементно
            //
            else {
                for (const auto& item : itemsGroup) {
                    itemsToDeleteCompressed.emplace(item.position, item.item);
                }
            }

            //
            // Выполняем несколько циклов укорачивания очереди на удаление
            //
            const int maxCompressionCycles = 2;
            if (itemsToDeleteSorted.empty() && compressionCycle < maxCompressionCycles) {
                itemsToDeleteSorted.swap(itemsToDeleteCompressed);
                ++compressionCycle;
            }
        }

        //
        // Удаляем все верхнеуровневые элементы, а так же группы сцен и папок любого уровня
        //
        QVector<TextModelItem*> itemsToDeleteGroup;
        auto removeGroup = [this, &itemsToDeleteGroup] {
            if (itemsToDeleteGroup.isEmpty()) {
                return;
            }

            d->model->removeItems(itemsToDeleteGroup.constFirst(), itemsToDeleteGroup.constLast());

            itemsToDeleteGroup.clear();
        };
        for (auto removeIter = itemsToDeleteCompressed.begin();
             removeIter != itemsToDeleteCompressed.end();) {
            //
            // Будем удалять только если элемент лежит в руте, или является папкой, или сценой
            //
            if (removeIter->second->hasParent() && removeIter->second->parent()->hasParent()
                && removeIter->second->type() != TextModelItemType::Folder
                && removeIter->second->type() != TextModelItemType::Group) {
                removeGroup();
                ++removeIter;
                continue;
            }

            const auto item = removeIter->second;
            if (!itemsToDeleteGroup.isEmpty()) {
                const auto lastItemToDelete = itemsToDeleteGroup.constLast();
                const auto lastItemToDeleteParent = lastItemToDelete->parent();
                if (lastItemToDeleteParent != item->parent()) {
                    removeGroup();
                } else {
                    const auto lastItemToDeleteRow
                        = lastItemToDeleteParent->rowOfChild(lastItemToDelete);
                    const auto itemRow = lastItemToDeleteParent->rowOfChild(item);
                    if (lastItemToDeleteRow + 1 != itemRow) {
                        removeGroup();
                    }
                }
            }

            itemsToDeleteGroup.append(item);

            removeIter = itemsToDeleteCompressed.erase(removeIter);
        }
        removeGroup();

        //
        // Затем удаляем то, что осталось поэлементно
        //
        for (auto removeIter = itemsToDeleteCompressed.rbegin();
             removeIter != itemsToDeleteCompressed.rend(); ++removeIter) {
            auto item = removeIter->second;

            //
            // Если удаляется сцена или папка, нужно удалить соответствующий элемент
            // и перенести элементы к предыдущему группирующему элементу
            //
            bool needToDeleteParent = false;
            if (item->type() == TextModelItemType::Text) {
                const auto textItem = static_cast<TextModelTextItem*>(item);
                //
                // ... т.к. при удалении папки удаляются и заголовок и конец, но удаляются они
                //     последовательно сверху вниз, то удалять непосредственно папку будем,
                //     когда дойдём до обработки именно конца папки
                //
                needToDeleteParent = textItem->paragraphType() == TextParagraphType::ActFooter
                    || textItem->paragraphType() == TextParagraphType::SequenceFooter
                    || textItem->paragraphType() == TextParagraphType::PartFooter
                    || textItem->paragraphType() == TextParagraphType::ChapterFooter
                    || textItem->paragraphType() == TextParagraphType::SceneHeading
                    || textItem->paragraphType() == TextParagraphType::BeatHeading
                    || textItem->paragraphType() == TextParagraphType::PageHeading
                    || textItem->paragraphType() == TextParagraphType::PanelHeading
                    || textItem->paragraphType() == TextParagraphType::ChapterHeading1
                    || textItem->paragraphType() == TextParagraphType::ChapterHeading2
                    || textItem->paragraphType() == TextParagraphType::ChapterHeading3
                    || textItem->paragraphType() == TextParagraphType::ChapterHeading4
                    || textItem->paragraphType() == TextParagraphType::ChapterHeading5
                    || textItem->paragraphType() == TextParagraphType::ChapterHeading6;
            }

            //
            // Запомним родителя и удаляем сам элемент
            //
            auto itemParent = item->parent();
            d->model->removeItem(item);

            //
            // Если необходимо удаляем родительский элемент
            //
            if (needToDeleteParent && itemParent != nullptr) {
                //
                // Определим предыдущий
                //
                TextModelItem* previousItem = nullptr;
                const int itemRow
                    = itemParent->hasParent() ? itemParent->parent()->rowOfChild(itemParent) : 0;
                if (itemRow > 0) {
                    const int previousItemRow = itemRow - 1;
                    previousItem = itemParent->parent()->childAt(previousItemRow);
                }
                //
                // Обработаем кейс, когда блоки находящиеся в папке в самом верху документа
                // нужно вынести из папки -> пробуем вставить их в папку идущую после удаляемой
                // папки
                //
                const bool needInsertInNextFolder = itemParent->type() == TextModelItemType::Folder
                    && itemParent->hasParent() && itemParent->parent()->childCount() > itemRow + 1
                    && itemParent->parent()->childAt(itemRow + 1)->type()
                        == TextModelItemType::Folder;
                if (needInsertInNextFolder) {
                    previousItem = itemParent->parent()->childAt(itemRow + 1);
                }

                //
                // Переносим дочерние элементы на уровень родительского элемента
                //
                TextModelItem* lastMovedItem = nullptr;
                while (itemParent->childCount() > 0) {
                    auto childItem = itemParent->childAt(0);
                    d->model->takeItem(childItem);

                    //
                    // Папки и сцены переносим на один уровень с текущим элементом
                    //
                    if (childItem->type() == TextModelItemType::Folder
                        || childItem->type() == TextModelItemType::Group) {
                        if (lastMovedItem == nullptr
                            || lastMovedItem->parent() != itemParent->parent()) {
                            //
                            // Если переносимый элемент ниже уровня, чем предыдущий,
                            // то вкладываем его внутрь предыдущего
                            //
                            bool moved = false;
                            if (previousItem != nullptr
                                && previousItem->type() == TextModelItemType::Group
                                && childItem->type() == TextModelItemType::Group) {
                                auto previousGroupItem = toGroup(previousItem);
                                auto childGroupItem = toGroup(childItem);
                                if (childGroupItem->level() > previousGroupItem->level()) {
                                    d->model->appendItem(childItem, previousItem);
                                    moved = true;
                                } else if (childGroupItem->level() == previousGroupItem->level()) {
                                    d->model->insertItem(childItem, previousItem);
                                    moved = true;
                                }
                            }
                            //
                            // В противном случае, если перенести не удалось, то ставим
                            // на один уровень с текущим
                            //
                            if (!moved) {
                                d->model->insertItem(childItem, itemParent);
                            }
                        } else {
                            d->model->insertItem(childItem, lastMovedItem);
                        }
                        previousItem = childItem;
                    }
                    //
                    // Все остальные элементы
                    //
                    else {
                        if (lastMovedItem == nullptr) {
                            //
                            // Если удаляемый был в папке, которая была в самом начале
                            // документа, то в начало последующей папки
                            //
                            if (needInsertInNextFolder) {
                                d->model->prependItem(childItem, previousItem);
                            }
                            //
                            // Если перед удаляемым была сцена или папка, то в её конец
                            //
                            else if (previousItem != nullptr
                                     && (previousItem->type() == TextModelItemType::Folder
                                         || previousItem->type() == TextModelItemType::Group)) {
                                d->model->appendItem(childItem, previousItem);
                            }
                            //
                            // Если перед удаляемым внутри родителя нет ни одного элемента, то
                            // вставляем в начало к деду
                            //
                            else if (previousItem == nullptr && itemParent->parent() != nullptr) {
                                d->model->prependItem(childItem, itemParent->parent());
                            }
                            //
                            // Во всех остальных случаях просто кладём на один уровень с
                            // предыдущим элементом
                            //
                            else {
                                d->model->insertItem(childItem, previousItem);
                            }
                        } else {
                            d->model->insertItem(childItem, lastMovedItem);
                        }
                    }

                    lastMovedItem = childItem;
                }

                //
                // Удаляем родителя удалённого элемента
                //
                d->model->removeItem(itemParent);

                //
                // Если после удаляемого элемента есть текстовые элементы, пробуем их встроить в
                // предыдущую сцену
                //
                if (previousItem != nullptr && previousItem->type() == TextModelItemType::Group) {
                    const auto previousItemRow = previousItem->parent()->rowOfChild(previousItem);
                    if (previousItemRow >= 0
                        && previousItemRow < previousItem->parent()->childCount() - 1) {
                        const int nextItemRow = previousItemRow + 1;
                        //
                        // Подготовим элементы для переноса
                        //
                        QVector<TextModelItem*> itemsToMove;
                        for (int itemRow = nextItemRow;
                             itemRow < previousItem->parent()->childCount(); ++itemRow) {
                            const auto nextItem = previousItem->parent()->childAt(itemRow);
                            if (nextItem == nullptr
                                || (nextItem->type() != TextModelItemType::Text
                                    && nextItem->type() != TextModelItemType::Splitter)) {
                                break;
                            }

                            itemsToMove.append(previousItem->parent()->childAt(itemRow));
                        }
                        if (!itemsToMove.isEmpty()) {
                            d->model->takeItems(itemsToMove.constFirst(), itemsToMove.constLast(),
                                                previousItem->parent());
                            d->model->appendItems(itemsToMove, previousItem);
                        }
                    }
                }
            }
        }
    }

    //
    // Идём с позиции начала, до конца добавления
    //
    auto block = findBlock(_position);
    //
    // ... определим элемент модели для предыдущего блока
    //
    auto previousTextItem = [block]() -> TextModelItem* {
        if (!block.isValid()) {
            return nullptr;
        }

        auto previousBlock = block.previous();
        if (!previousBlock.isValid() || previousBlock.userData() == nullptr) {
            return nullptr;
        }

        auto blockData = static_cast<TextBlockData*>(previousBlock.userData());
        return blockData->item();
    }();

    //
    // Информация о таблице, в которой находится блок
    //
    struct TableInfo {
        bool inTable = false;
        bool inFirstColumn = false;
    } tableInfo;
    auto updateTableInfo = [this, &tableInfo](const QTextBlock& _block) {
        TextCursor cursor(this);
        cursor.setPosition(_block.position());
        tableInfo.inTable = cursor.inTable();
        tableInfo.inFirstColumn = cursor.inFirstColumn();
    };
    updateTableInfo(block);

    while (block.isValid() && block.position() <= _position + _charsAdded) {
        const auto paragraphType = TextBlockStyle::forBlock(block);

        //
        // Новый блок
        //
        if (block.userData() == nullptr) {
            //
            // Разделитель
            //
            if (paragraphType == TextParagraphType::PageSplitter) {
                TextCursor cursor(this);
                cursor.setPosition(block.position());
                cursor.movePosition(QTextCursor::NextBlock);
                //
                // Сформируем элемент в зависимости от типа разделителя
                //
                TextModelSplitterItem* splitterItem = nullptr;
                if (cursor.inTable() && !tableInfo.inTable) {
                    tableInfo.inTable = true;
                    tableInfo.inFirstColumn = true;
                    splitterItem = d->model->createSplitterItem();
                    splitterItem->setSplitterType(TextModelSplitterItemType::Start);
                } else {
                    tableInfo = {};
                    splitterItem = d->model->createSplitterItem();
                    splitterItem->setSplitterType(TextModelSplitterItemType::End);
                }
                if (previousTextItem == nullptr) {
                    d->model->prependItem(splitterItem);
                } else {
                    d->model->insertItem(splitterItem, previousTextItem);
                }

                //
                // Запомним информацию о разделителе в блоке
                //
                auto blockData = new TextBlockData(splitterItem);
                block.setUserData(blockData);
                previousTextItem = splitterItem;

                //
                // Запомним новый блок, или обновим старый
                //
                d->positionsToItems.insert_or_assign(block.position(), previousTextItem);

                block = block.next();
                continue;
            } else {
                updateTableInfo(block);
            }

            //
            // Создаём группирующий элемент, если создаётся непосредственно папка, сцена, бит,
            // страница или панель
            //
            TextModelItem* parentItem = nullptr;
            switch (paragraphType) {
            case TextParagraphType::ActHeading: {
                parentItem = d->model->createFolderItem(TextFolderType::Act);
                break;
            }

            case TextParagraphType::SequenceHeading: {
                parentItem = d->model->createFolderItem(TextFolderType::Sequence);
                break;
            }

            case TextParagraphType::PartHeading: {
                parentItem = d->model->createFolderItem(TextFolderType::Part);
                break;
            }

            case TextParagraphType::ChapterHeading: {
                parentItem = d->model->createFolderItem(TextFolderType::Chapter);
                break;
            }

            case TextParagraphType::SceneHeading: {
                parentItem = d->model->createGroupItem(TextGroupType::Scene);
                break;
            }

            case TextParagraphType::BeatHeading: {
                parentItem = d->model->createGroupItem(TextGroupType::Beat);
                break;
            }

            case TextParagraphType::PageHeading: {
                parentItem = d->model->createGroupItem(TextGroupType::Page);
                break;
            }

            case TextParagraphType::PanelHeading: {
                parentItem = d->model->createGroupItem(TextGroupType::Panel);
                break;
            }

            case TextParagraphType::ChapterHeading1: {
                parentItem = d->model->createGroupItem(TextGroupType::Chapter1);
                break;
            }
            case TextParagraphType::ChapterHeading2: {
                parentItem = d->model->createGroupItem(TextGroupType::Chapter2);
                break;
            }
            case TextParagraphType::ChapterHeading3: {
                parentItem = d->model->createGroupItem(TextGroupType::Chapter3);
                break;
            }
            case TextParagraphType::ChapterHeading4: {
                parentItem = d->model->createGroupItem(TextGroupType::Chapter4);
                break;
            }
            case TextParagraphType::ChapterHeading5: {
                parentItem = d->model->createGroupItem(TextGroupType::Chapter5);
                break;
            }
            case TextParagraphType::ChapterHeading6: {
                parentItem = d->model->createGroupItem(TextGroupType::Chapter6);
                break;
            }

            default:
                break;
            }

            //
            // Создаём сам текстовый элемент
            //
            auto textItem = d->model->createTextItem();
            textItem->setCorrection(
                block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection));
            textItem->setCorrectionContinued(
                block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrectionContinued));
            textItem->setBreakCorrectionStart(
                block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionStart));
            textItem->setBreakCorrectionEnd(
                block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd));
            if (tableInfo.inTable) {
                textItem->setInFirstColumn(tableInfo.inFirstColumn);
            } else {
                textItem->setInFirstColumn({});
            }
            textItem->setParagraphType(paragraphType);
            if (d->documentTemplate().paragraphStyle(paragraphType).align()
                != block.blockFormat().alignment()) {
                textItem->setAlignment(block.blockFormat().alignment());
            } else {
                textItem->clearAlignment();
            }
            textItem->setText(block.text());
            textItem->setFormats(block.textFormats());
            textItem->setReviewMarks(block.textFormats());

            //
            // Является ли предыдущий элемент футером папки
            //
            const bool previousItemIsFolderFooter = [previousTextItem] {
                if (!previousTextItem || previousTextItem->type() != TextModelItemType::Text) {
                    return false;
                }

                auto textItem = static_cast<TextModelTextItem*>(previousTextItem);
                return textItem->paragraphType() == TextParagraphType::ActFooter
                    || textItem->paragraphType() == TextParagraphType::SequenceFooter
                    || textItem->paragraphType() == TextParagraphType::PartFooter
                    || textItem->paragraphType() == TextParagraphType::ChapterFooter;
            }();

            //
            // Добавляем элементы в модель
            //
            // ... в случае, когда вставляем внутрь созданной папки, или группы
            //
            if (parentItem != nullptr) {
                //
                // Вставляем родительский элемент
                //
                // ... если перед вставляемым элементом что-то уже есть
                //
                if (previousTextItem != nullptr) {
                    auto previousItemParent = previousTextItem->parent();
                    Q_ASSERT(previousItemParent);

                    //
                    // Если элемент вставляется после другого элемента того же уровня, или после
                    // окончания папки, то вставляем его на том же уровне, что и предыдущий
                    //
                    if (previousItemParent->subtype() == parentItem->subtype()
                        || previousItemIsFolderFooter) {
                        d->model->insertItem(parentItem, previousItemParent);
                    }
                    //
                    // Если вставляется папка на уровне группы, то вставим в родителя более высокого
                    // уровня
                    //
                    else if (isFolder(parentItem) && !isFolder(previousItemParent)) {
                        auto targetParent = previousItemParent;
                        while (targetParent != nullptr
                               && targetParent->parent()->type() != TextModelItemType::Folder) {
                            targetParent = targetParent->parent();
                        };
                        Q_ASSERT(targetParent);
                        d->model->insertItem(parentItem, targetParent);
                    }
                    //
                    // Если вставляется группа в позицию, где находится другая группа более
                    // низкого уровня, то вставляем новую группу в родителя более высокого уровня
                    //
                    else if (isGroup(parentItem) && isGroup(previousItemParent)
                             && toGroup(parentItem)->level()
                                 < toGroup(previousItemParent)->level()) {
                        auto targetParent = previousItemParent->parent();
                        do {
                            if ((isGroup(targetParent)
                                 && toGroup(parentItem)->level() >= toGroup(targetParent)->level())
                                || isFolder(targetParent)) {
                                break;
                            }
                            previousItemParent = targetParent;
                            targetParent = targetParent->parent();
                        } while (targetParent != nullptr);
                        //
                        // Если дошли до элемента с таким же уровнем, то вставим после
                        //
                        if (isGroup(targetParent)
                            && toGroup(parentItem)->level() == toGroup(targetParent)->level()) {
                            d->model->insertItem(parentItem, targetParent);
                        }
                        //
                        // а если элемента с таким же уровнем нет, то вставим внутрь родителя
                        //
                        else {
                            d->model->insertItem(parentItem, previousItemParent);
                        }
                    }
                    //
                    // В противном случае вставляем после предыдущего элемента
                    //
                    else {
                        d->model->insertItem(parentItem, previousTextItem);
                    }
                }
                //
                // ... а если перед вставляемым ничего нет, просто вставим в самое начало
                //
                else {
                    d->model->prependItem(parentItem);
                }

                //
                // Вставляем текстовый элемент в родителя
                //
                d->model->appendItem(textItem, parentItem);

                //
                // Если вставляется группа, то все элементы идущие после неё нужно по возможности
                // положить к ней внутрь
                //
                if (isGroup(parentItem)) {
                    TextModelItem* grandParentItem = nullptr;
                    int startChildIndex = -1;
                    //
                    // Если группа вставлена в самое начало
                    //
                    if (previousTextItem == nullptr) {
                        grandParentItem = parentItem->parent();
                        startChildIndex = grandParentItem->rowOfChild(parentItem);
                    }
                    //
                    // Если группа вставлена на одном уровне с предыдущим
                    //
                    else if (parentItem->parent() == previousTextItem->parent()->parent()) {
                        //
                        // Переносим все элементы идущие за вставленной группой
                        //
                        grandParentItem = previousTextItem->parent();
                        startChildIndex = grandParentItem->rowOfChild(previousTextItem);
                    }
                    //
                    // А если предыдущий элемент на другом уровне
                    //
                    else {
                        //
                        // Переносим элементы идущие после предыдущего текстового
                        //
                        grandParentItem = previousTextItem->parent();
                        startChildIndex = grandParentItem->rowOfChild(previousTextItem);
                    }

                    //
                    // Соберём элементы для переноса
                    //
                    bool firstTry = true;
                    do {
                        //
                        // Элементы для первого прохода уже настроены, поэтому переходим в поиске
                        // на уровень выше только со второго прохода
                        //
                        if (!firstTry) {
                            startChildIndex
                                = grandParentItem->parent()->rowOfChild(grandParentItem);
                            grandParentItem = grandParentItem->parent();
                        } else {
                            firstTry = false;
                        }

                        QVector<TextModelItem*> itemsToMove;
                        //
                        // +1, т.к. начинаем со следующего элемента
                        //
                        for (int childIndex = startChildIndex + 1;
                             childIndex < grandParentItem->childCount(); ++childIndex) {
                            auto grandParentChildItem = grandParentItem->childAt(childIndex);
                            //
                            // В моменте, когда дошли до максимального родителя первоначальный
                            // индекс будет указывать на вставляемый элемент, поэтому пропускаем его
                            //
                            if (grandParentChildItem == parentItem) {
                                continue;
                            }

                            if (grandParentChildItem->type() == TextModelItemType::Text) {
                                const auto grandParentChildTextItem = toText(grandParentChildItem);
                                if (grandParentChildTextItem->paragraphType()
                                        == TextParagraphType::ActFooter
                                    || grandParentChildTextItem->paragraphType()
                                        == TextParagraphType::SequenceFooter
                                    || grandParentChildTextItem->paragraphType()
                                        == TextParagraphType::PartFooter
                                    || grandParentChildTextItem->paragraphType()
                                        == TextParagraphType::ChapterFooter) {
                                    break;
                                }
                            } else if (grandParentChildItem->type() == TextModelItemType::Group) {
                                if (toGroup(parentItem)->level()
                                    >= toGroup(grandParentChildItem)->level()) {
                                    break;
                                }
                            } else if (grandParentChildItem->type() == TextModelItemType::Folder) {
                                break;
                            }

                            itemsToMove.append(grandParentChildItem);
                        }

                        //
                        // ... собственно переносим элементы
                        //
                        if (!itemsToMove.isEmpty()) {
                            d->model->takeItems(itemsToMove.constFirst(), itemsToMove.constLast(),
                                                grandParentItem);
                            d->model->appendItems(itemsToMove, parentItem);
                        }

                        //
                        // Переходим на уровень выше
                        //
                        if (grandParentItem->parent() == nullptr) {
                            break;
                        }
                    } while (grandParentItem != parentItem->parent());
                }
                //
                // А для папки, если она вставляется после сцены, то нужно перенести все текстовые
                // элементы, которые идут после вставленной папки на уровень самой папки
                //
                else if (previousTextItem != nullptr && isGroup(previousTextItem->parent())) {
                    auto grandParentItem = previousTextItem->parent();
                    const int lastItemIndex = grandParentItem->rowOfChild(previousTextItem) + 1;

                    //
                    // Соберём элементы для переноса
                    //
                    QVector<TextModelItem*> itemsToMove;
                    for (int childIndex = lastItemIndex; childIndex < grandParentItem->childCount();
                         ++childIndex) {
                        auto grandParentChildItem = grandParentItem->childAt(childIndex);
                        if (grandParentChildItem->type() != TextModelItemType::Text) {
                            break;
                        }

                        auto grandParentChildTextItem
                            = static_cast<TextModelTextItem*>(grandParentChildItem);
                        if (grandParentChildTextItem->paragraphType()
                                == TextParagraphType::ActFooter
                            || grandParentChildTextItem->paragraphType()
                                == TextParagraphType::SequenceFooter
                            || grandParentChildTextItem->paragraphType()
                                == TextParagraphType::PartFooter
                            || grandParentChildTextItem->paragraphType()
                                == TextParagraphType::ChapterFooter) {
                            break;
                        }

                        itemsToMove.append(grandParentChildItem);
                    }
                    //
                    // ... собственно переносим элементы
                    //
                    if (!itemsToMove.isEmpty()) {
                        d->model->takeItems(itemsToMove.constFirst(), itemsToMove.constLast(),
                                            grandParentItem);
                        d->model->insertItems(itemsToMove, parentItem);
                    }
                }
            }
            //
            // ... в случае, когда добавился просто текст
            //
            else {
                //
                // ... в самое начало документа
                //
                if (previousTextItem == nullptr) {
                    d->model->prependItem(textItem);
                }
                //
                // ... после предыдущего элемента
                //
                else {
                    //
                    // ... если блок вставляется после конца папки, то нужно вынести на уровень с
                    // папкой
                    //
                    if (previousItemIsFolderFooter) {
                        d->model->insertItem(textItem, previousTextItem->parent());
                    }
                    //
                    // ... в противном случае ставим на уровне с предыдущим элементом
                    //
                    else {
                        d->model->insertItem(textItem, previousTextItem);
                    }
                }
            }

            auto blockData = new TextBlockData(textItem);
            block.setUserData(blockData);

            previousTextItem = textItem;
        }
        //
        // Старый блок
        //
        else {
            updateTableInfo(block);
            auto blockData = static_cast<TextBlockData*>(block.userData());
            auto item = blockData->item();

            if (item->type() == TextModelItemType::Text) {
                auto textItem = static_cast<TextModelTextItem*>(item);
                textItem->setCorrection(
                    block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection));
                textItem->setCorrectionContinued(block.blockFormat().boolProperty(
                    TextBlockStyle::PropertyIsCorrectionContinued));
                textItem->setBreakCorrectionStart(block.blockFormat().boolProperty(
                    TextBlockStyle::PropertyIsBreakCorrectionStart));
                textItem->setBreakCorrectionEnd(
                    block.blockFormat().boolProperty(TextBlockStyle::PropertyIsBreakCorrectionEnd));
                if (tableInfo.inTable) {
                    textItem->setInFirstColumn(tableInfo.inFirstColumn);
                } else {
                    textItem->setInFirstColumn({});
                }
                textItem->setParagraphType(paragraphType);
                if (d->documentTemplate().paragraphStyle(paragraphType).align()
                    != block.blockFormat().alignment()) {
                    textItem->setAlignment(block.blockFormat().alignment());
                } else {
                    textItem->clearAlignment();
                }
                textItem->setText(block.text());
                textItem->setFormats(block.textFormats());
                textItem->setReviewMarks(block.textFormats());
            }

            d->model->updateItem(item);
            previousTextItem = item;
        }

        //
        // Запомним новый блок, или обновим старый
        //
        d->positionsToItems.insert_or_assign(block.position(), previousTextItem);

        //
        // Переходим к обработке следующего блока
        //
        block = block.next();
    }
}

void TextDocument::insertTable(const TextCursor& _cursor)
{
    const auto& scriptTemplate = d->documentTemplate();
    const auto pageSplitterWidth = scriptTemplate.pageSplitterWidth();
    //
    // Эта двухпиксельная рамка никак не убирается...
    //
    const int qtTableBorderWidth = 2;
    const qreal tableWidth = pageSize().width() - rootFrame()->frameFormat().leftMargin()
        - rootFrame()->frameFormat().rightMargin() - qtTableBorderWidth * 3 + pageSplitterWidth;
    const qreal leftColumnWidth = tableWidth * scriptTemplate.leftHalfOfPageWidthPercents() / 100.0;
    const qreal rightColumnWidth = tableWidth - leftColumnWidth;
    QTextTableFormat format;
    format.setBorder(0);
    format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    format.setWidth(QTextLength{ QTextLength::FixedLength, tableWidth });
    format.setColumnWidthConstraints({ QTextLength{ QTextLength::FixedLength, leftColumnWidth },
                                       QTextLength{ QTextLength::FixedLength, rightColumnWidth } });
    format.setLeftMargin(-1 * pageSplitterWidth);
    //
    // Делаем негативный отступ, чтобы отыграть место, которое занимает рамка сверху и снизу таблицы
    //
    format.setTopMargin(-1 * qtTableBorderWidth * 2);
    auto cursor = _cursor;
    cursor.insertTable(1, 2, format);
}

} // namespace BusinessLayer

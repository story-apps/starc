#include "text_document.h"

#include "text_block_data.h"
#include "text_cursor.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/screenplay_title_page_model.h>
#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/simple_text/simple_text_model_chapter_item.h>
#include <business_layer/model/simple_text/simple_text_model_text_item.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QDateTime>
#include <QPointer>
#include <QScopedValueRollback>
#include <QTextTable>

using BusinessLayer::TemplatesFacade;
using BusinessLayer::TextBlockStyle;
using BusinessLayer::TextParagraphType;


namespace BusinessLayer {

enum class DocumentState { Undefined, Loading, Changing, Correcting, Ready };


class SimpleTextDocument::Implementation
{
public:
    explicit Implementation(SimpleTextDocument* _document);

    /**
     * @brief Получить шаблон документа
     */
    const TextTemplate& documentTemplate() const;

    /**
     * @brief Скорректировать позиции элементов на заданную дистанцию
     */
    void correctPositionsToItems(std::map<int, BusinessLayer::TextModelItem*>::iterator _from,
                                 int _distance);
    void correctPositionsToItems(int _fromPosition, int _distance);

    /**
     * @brief Считать содержимое элмента модели с заданным индексом
     *        и вставить считанные данные в текущее положение курсора
     */
    void readModelItemContent(int _itemRow, const QModelIndex& _parent, QTextCursor& _cursor,
                              bool& _isFirstParagraph);

    /**
     * @brief Считать содержимое вложенных в заданный индекс элементов
     *        и вставить считанные данные в текущее положение курсора
     */
    void readModelItemsContent(const QModelIndex& _parent, QTextCursor& _cursor,
                               bool& _isFirstParagraph);


    SimpleTextDocument* q = nullptr;

    DocumentState state = DocumentState::Undefined;
    QPointer<BusinessLayer::SimpleTextModel> model;
    bool canChangeModel = true;
    std::map<int, BusinessLayer::TextModelItem*> positionsToItems;
};

SimpleTextDocument::Implementation::Implementation(SimpleTextDocument* _document)
    : q(_document)
{
}

const TextTemplate& SimpleTextDocument::Implementation::documentTemplate() const
{
    if (auto titlePageModel = qobject_cast<BusinessLayer::ScreenplayTitlePageModel*>(model)) {
        return TemplatesFacade::screenplayTitlePageTemplate(
            titlePageModel->informationModel()->templateId());
    }
    return TemplatesFacade::simpleTextTemplate();
}

void SimpleTextDocument::Implementation::correctPositionsToItems(
    std::map<int, BusinessLayer::TextModelItem*>::iterator _from, int _distance)
{
    if (_from == positionsToItems.end()) {
        return;
    }

    if (_distance > 0) {
        auto reversed = [](std::map<int, BusinessLayer::TextModelItem*>::iterator iter) {
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

void SimpleTextDocument::Implementation::correctPositionsToItems(int _fromPosition, int _distance)
{
    correctPositionsToItems(positionsToItems.lower_bound(_fromPosition), _distance);
}

void SimpleTextDocument::Implementation::readModelItemContent(int _itemRow,
                                                              const QModelIndex& _parent,
                                                              QTextCursor& _cursor,
                                                              bool& _isFirstParagraph)
{
    const auto itemIndex = model->index(_itemRow, 0, _parent);
    const auto item = model->itemForIndex(itemIndex);
    switch (item->type()) {
    case TextModelItemType::Group: {
        break;
    }

    case TextModelItemType::Text: {
        const auto textItem = static_cast<SimpleTextModelTextItem*>(item);

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
        _cursor.setBlockFormat(currentStyle.blockFormat());
        _cursor.setBlockCharFormat(currentStyle.charFormat());
        _cursor.setCharFormat(currentStyle.charFormat());

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
        // Обновим высоту блока, если требуется
        //
        qreal blockHeight = 0.0;
        const auto formats = formatCursor.block().textFormats();
        if (formats.isEmpty()) {
            blockHeight = documentTemplate()
                              .paragraphStyle(textItem->paragraphType())
                              .blockFormat()
                              .lineHeight();
        } else {
            for (const auto& format : formats) {
                blockHeight
                    = std::max(blockHeight, TextHelper::fineLineSpacing(format.format.font()));
            }
        }
        if (!qFuzzyCompare(formatCursor.blockFormat().lineHeight(), blockHeight)) {
            auto blockFormat = formatCursor.blockFormat();
            blockFormat.setLineHeight(blockHeight, QTextBlockFormat::FixedHeight);
            formatCursor.setBlockFormat(blockFormat);
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

void SimpleTextDocument::Implementation::readModelItemsContent(const QModelIndex& _parent,
                                                               QTextCursor& _cursor,
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


// ****


SimpleTextDocument::SimpleTextDocument(QObject* _parent)
    : QTextDocument(_parent)
    , d(new Implementation(this))
{
    connect(this, &SimpleTextDocument::contentsChange, this,
            &SimpleTextDocument::updateModelOnContentChange);
}

SimpleTextDocument::~SimpleTextDocument() = default;

void SimpleTextDocument::setModel(BusinessLayer::SimpleTextModel* _model, bool _canChangeModel)
{
    d->state = DocumentState::Loading;

    if (d->model) {
        d->model->disconnect(this);
    }

    d->model = _model;
    d->canChangeModel = _canChangeModel;
    d->positionsToItems.clear();

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

    //
    // Обновим шрифт документа, в моменте когда текста нет
    //
    const auto templateDefaultFont
        = d->documentTemplate().paragraphStyle(TextParagraphType::Text).font();
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
    connect(d->model, &SimpleTextModel::modelReset, this, [this] {
        QSignalBlocker signalBlocker(this);
        setModel(d->model);
    });
    connect(d->model, &SimpleTextModel::dataChanged, this,
            [this](const QModelIndex& _topLeft, const QModelIndex& _bottomRight) {
                if (d->state != DocumentState::Ready) {
                    return;
                }

                Q_ASSERT(_topLeft == _bottomRight);

                QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

                const auto position = itemStartPosition(_topLeft);
                if (position < 0) {
                    return;
                }

                const auto item = d->model->itemForIndex(_topLeft);
                if (item->type() != TextModelItemType::Text) {
                    return;
                }

                const auto textItem = static_cast<SimpleTextModelTextItem*>(item);

                QTextCursor cursor(this);
                cursor.setPosition(position);
                cursor.beginEditBlock();

                //
                // Обновим элемент
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
                // Обновим высоту блока, если требуется
                //
                qreal blockHeight = 0.0;
                const auto formats = cursor.block().textFormats();
                if (formats.isEmpty()) {
                    blockHeight = d->documentTemplate()
                                      .paragraphStyle(textItem->paragraphType())
                                      .blockFormat()
                                      .lineHeight();
                } else {
                    for (const auto& format : formats) {
                        blockHeight = std::max(blockHeight,
                                               TextHelper::fineLineSpacing(format.format.font()));
                    }
                }
                if (!qFuzzyCompare(cursor.blockFormat().lineHeight(), blockHeight)) {
                    auto blockFormat = cursor.blockFormat();
                    blockFormat.setLineHeight(blockHeight, QTextBlockFormat::FixedHeight);
                    cursor.setBlockFormat(blockFormat);
                }

                cursor.endEditBlock();
            });
    connect(
        d->model, &SimpleTextModel::rowsInserted, this,
        [this](const QModelIndex& _parent, int _from, int _to) {
            if (d->state != DocumentState::Ready) {
                return;
            }

            QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

            //
            // Игнорируем добавление пустых глав
            //
            const auto item = d->model->itemForIndex(d->model->index(_from, 0, _parent));
            if (item->type() == TextModelItemType::Group && !item->hasChildren()) {
                return;
            }

            //
            // Определим позицию курсора откуда нужно начинать вставку
            //
            QModelIndex cursorItemIndex;
            if (_from > 0) {
                cursorItemIndex = d->model->index(_from - 1, 0, _parent);

                //
                // В кейсе, когда вставляется новая глава перед уже существующей и существующую
                // нужно перенести после неё, добавляем дополнительное условие на определение
                // позиции, т.к. у новой главы ещё нет элементов и мы не знаем о её позиции,
                // поэтому берём предыдущую, либо смотрим в конец общего родителя
                //
                const auto cursorItem = d->model->itemForIndex(cursorItemIndex);
                if (cursorItem->type() == TextModelItemType::Group && !cursorItem->hasChildren()) {
                    if (_from > 1) {
                        cursorItemIndex = d->model->index(_from - 2, 0, _parent);
                    } else {
                        cursorItemIndex = d->model->index(_parent.row() - 1, 0, _parent.parent());
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
            QTextCursor cursor(this);
            cursor.beginEditBlock();

            cursor.setPosition(cursorPosition);
            if (isFirstParagraph) {
                //
                // Если первый параграф, то нужно перенести блок со своими данными дальше
                //
                TextBlockData* blockData = nullptr;
                auto block = cursor.block();
                if (block.userData() != nullptr) {
                    blockData = new TextBlockData(static_cast<TextBlockData*>(block.userData()));
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
    connect(d->model, &SimpleTextModel::rowsAboutToBeRemoved, this,
            [this](const QModelIndex& _parent, int _from, int _to) {
                if (d->state != DocumentState::Ready) {
                    return;
                }

                QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

                const QModelIndex fromIndex = d->model->index(_from, 0, _parent);
                if (!fromIndex.isValid()) {
                    return;
                }
                auto fromPosition = itemStartPosition(fromIndex);
                if (fromPosition < 0) {
                    return;
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
                const auto selectionInterval = cursor.selectionInterval();
                auto fromIter = d->positionsToItems.lower_bound(selectionInterval.from);
                auto endIter = d->positionsToItems.lower_bound(selectionInterval.to);
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
                d->correctPositionsToItems(selectionInterval.to, -1 * distance);

                cursor.beginEditBlock();
                if (cursor.hasSelection()) {
                    cursor.deleteChar();
                }

                //
                // Если это самый первый блок, то нужно удалить на один символ больше, чтобы удалить
                // сам блок
                //
                if (fromPosition == 0 && toPosition != characterCount()) {
                    cursor.deleteChar();
                }
                //
                // Если это не самый первый блок, то нужно взять на один символ назад, чтобы удалить
                // сам блок
                //
                else if (fromPosition > 0) {
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
                cursor.endEditBlock();
            });
    //
    // Группируем массовые изменения, чтобы не мелькать пользователю перед глазами
    //
    connect(d->model, &SimpleTextModel::rowsAboutToBeChanged, this,
            [this] { QTextCursor(this).beginEditBlock(); });
    connect(d->model, &SimpleTextModel::rowsChanged, this, [this] {
        d->state = DocumentState::Changing;
        QTextCursor(this).endEditBlock();
        d->state = DocumentState::Ready;
    });

    d->state = DocumentState::Ready;
}

int SimpleTextDocument::itemPosition(const QModelIndex& _index, bool _fromStart)
{
    auto item = d->model->itemForIndex(_index);
    if (item == nullptr) {
        return -1;
    }

    while (item->childCount() > 0) {
        item = item->childAt(_fromStart ? 0 : item->childCount() - 1);
    }
    for (const auto& [key, value] : d->positionsToItems) {
        if (value == item) {
            return key;
        }
    }

    return -1;
}

int SimpleTextDocument::itemStartPosition(const QModelIndex& _index)
{
    return itemPosition(_index, true);
}

int SimpleTextDocument::itemEndPosition(const QModelIndex& _index)
{
    return itemPosition(_index, false);
}

QString SimpleTextDocument::chapterNumber(const QTextBlock& _forBlock) const
{
    if (_forBlock.userData() == nullptr) {
        return {};
    }

    auto blockData = static_cast<TextBlockData*>(_forBlock.userData());
    if (blockData == nullptr) {
        return {};
    }

    auto itemParent = blockData->item()->parent();
    if (itemParent == nullptr || itemParent->type() != TextModelItemType::Group) {
        return {};
    }

    auto itemScene = static_cast<SimpleTextModelChapterItem*>(itemParent);
    return itemScene->number()->text;
}

QString SimpleTextDocument::mimeFromSelection(int _fromPosition, int _toPosition) const
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

void SimpleTextDocument::insertFromMime(int _position, const QString& _mimeData)
{
    const auto block = findBlock(_position);
    if (block.userData() == nullptr) {
        return;
    }

    auto blockData = static_cast<TextBlockData*>(block.userData());
    if (blockData == nullptr) {
        return;
    }

    const auto itemIndex = d->model->indexForItem(blockData->item());
    const auto positionInBlock = _position - block.position();
    d->model->insertFromMime(itemIndex, positionInBlock, _mimeData);
}

void SimpleTextDocument::addParagraph(BusinessLayer::TextParagraphType _type, QTextCursor _cursor)
{
    _cursor.beginEditBlock();

    //
    // Если параграф целиком переносится (энтер нажат перед всем текстом блока),
    // необходимо перенести данные блока с текущего на следующий
    //
    if (_cursor.block().text().left(_cursor.positionInBlock()).isEmpty()
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

void SimpleTextDocument::setParagraphType(BusinessLayer::TextParagraphType _type,
                                          const QTextCursor& _cursor)
{
    const auto currentParagraphType = TextBlockStyle::forBlock(_cursor.block());
    if (currentParagraphType == _type) {
        return;
    }

    auto cursor = _cursor;
    cursor.beginEditBlock();

    //
    // Первым делом очищаем пользовательские данные
    //
    cursor.block().setUserData(nullptr);

    //
    // Применим новый стиль к блоку
    //
    applyParagraphType(_type, _cursor);

    cursor.endEditBlock();
}

void SimpleTextDocument::applyParagraphType(BusinessLayer::TextParagraphType _type,
                                            const QTextCursor& _cursor)
{
    auto cursor = _cursor;
    cursor.beginEditBlock();

    const auto newBlockStyle = d->documentTemplate().paragraphStyle(_type);

    //
    // Обновим стили
    //
    cursor.setBlockCharFormat(newBlockStyle.charFormat());
    cursor.setBlockFormat(newBlockStyle.blockFormat());

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

    cursor.endEditBlock();
}

void SimpleTextDocument::addReviewMark(const QColor& _textColor, const QColor& _backgroundColor,
                                       const QString& _comment, const QTextCursor& _cursor)
{
    SimpleTextModelTextItem::ReviewMark reviewMark;
    if (_textColor.isValid()) {
        reviewMark.textColor = _textColor;
    }
    if (_backgroundColor.isValid()) {
        reviewMark.backgroundColor = _backgroundColor;
    }
    reviewMark.comments.append({ DataStorageLayer::StorageFacade::settingsStorage()->accountName(),
                                 QDateTime::currentDateTime().toString(Qt::ISODate), _comment });

    auto cursor = _cursor;
    cursor.mergeCharFormat(reviewMark.charFormat());
}

void SimpleTextDocument::updateModelOnContentChange(int _position, int _charsRemoved,
                                                    int _charsAdded)
{
    if (d->model == nullptr) {
        return;
    }

    if (!d->canChangeModel) {
        return;
    }

    //
    // Обновляем формат первого блока, если там нужен был разрыв страницы, чтобы первый блок
    // документа не перескакивал на вторую страницу
    //
    if (_position == 0
        && begin().blockFormat().pageBreakPolicy() == QTextFormat::PageBreak_AlwaysBefore) {
        TextCursor cursor(this);
        cursor.movePosition(TextCursor::Start);
        auto blockFormat = cursor.blockFormat();
        blockFormat.setPageBreakPolicy(QTextFormat::PageBreak_Auto);
        cursor.setBlockFormat(blockFormat);
    }

    if (d->state != DocumentState::Ready && d->state != DocumentState::Correcting) {
        return;
    }

    QScopedValueRollback temporatryState(d->state, DocumentState::Changing);

    using namespace BusinessLayer;

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
                if (!itemsGroup.isEmpty()
                    && itemsGroup.constLast().item->parent() != firstItem->parent()) {
                    break;
                }

                const auto firstItemPosition = beginIter->first;
                itemsGroup.append({ firstItem, firstItemPosition });
                itemsToDeleteSorted.erase(beginIter);
            } while (!itemsToDeleteSorted.empty());

            //
            // Если элемент удаляется целиком, то его и запишем на удаление,
            // за исключением случая, если это самый верхнеуровневый элемент
            //
            if (itemsGroup.constFirst().item->parent()->childCount() == itemsGroup.size()
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
            if (removeIter->second->parent()->hasParent()
                && removeIter->second->type() != TextModelItemType::Group) {
                removeGroup();
                ++removeIter;
                continue;
            }

            const auto item = removeIter->second;
            if (!itemsToDeleteGroup.isEmpty()
                && itemsToDeleteGroup.constLast()->parent() != item->parent()) {
                removeGroup();
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
            // Если удаляется глава, нужно удалить соответствующий элемент
            // и перенести элементы к предыдущему группирующему элементу
            //
            bool needToDeleteParent = false;
            if (item->type() == TextModelItemType::Text) {
                const auto textItem = static_cast<SimpleTextModelTextItem*>(item);
                needToDeleteParent = textItem->paragraphType() == TextParagraphType::Heading1
                    || textItem->paragraphType() == TextParagraphType::Heading2
                    || textItem->paragraphType() == TextParagraphType::Heading3
                    || textItem->paragraphType() == TextParagraphType::Heading4
                    || textItem->paragraphType() == TextParagraphType::Heading5
                    || textItem->paragraphType() == TextParagraphType::Heading6;
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
                // Переносим дочерние элементы на уровень родительского элемента
                //
                TextModelItem* lastMovedItem = nullptr;
                while (itemParent->childCount() > 0) {
                    auto childItem = itemParent->childAt(0);
                    d->model->takeItem(childItem, itemParent);

                    //
                    // Главы переносим на один уровень с текущим элементом
                    //
                    if (childItem->type() == TextModelItemType::Group) {
                        if (lastMovedItem == nullptr
                            || lastMovedItem->parent() != itemParent->parent()) {
                            d->model->insertItem(childItem, itemParent);
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
                            // Если перед удаляемым была глава, то в её конец
                            //
                            if (previousItem != nullptr
                                && previousItem->type() == TextModelItemType::Group) {
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
                // предыдущую главу
                //
                if (previousItem != nullptr && previousItem->type() == TextModelItemType::Group) {
                    const auto previousItemRow = previousItem->parent()->rowOfChild(previousItem);
                    if (previousItemRow >= 0
                        && previousItemRow < previousItem->parent()->childCount() - 1) {
                        const int nextItemRow = previousItemRow + 1;
                        auto nextItem = previousItem->parent()->childAt(nextItemRow);
                        while (nextItem != nullptr && nextItem->type() == TextModelItemType::Text) {
                            d->model->takeItem(nextItem, nextItem->parent());
                            d->model->appendItem(nextItem, previousItem);
                            nextItem = previousItem->parent()->childAt(nextItemRow);
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
    auto previousItem = [block]() -> TextModelItem* {
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

    while (block.isValid() && block.position() <= _position + _charsAdded) {
        const auto paragraphType = TextBlockStyle::forBlock(block);

        //
        // Новый блок
        //
        if (block.userData() == nullptr) {
            //
            // Создаём группирующий элемент, если создаётся глава
            //
            TextModelItem* parentItem = nullptr;
            switch (paragraphType) {
            case TextParagraphType::Heading1:
            case TextParagraphType::Heading2:
            case TextParagraphType::Heading3:
            case TextParagraphType::Heading4:
            case TextParagraphType::Heading5:
            case TextParagraphType::Heading6: {
                parentItem = d->model->createGroupItem(TextGroupType::Chapter);
                break;
            }

            default:
                break;
            }

            //
            // Создаём сам текстовый элемент
            //
            auto textItem = d->model->createTextItem();
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
            // Добавляем элементы в модель
            //
            // ... в случае, когда вставляем внутрь созданной главы
            //
            if (parentItem != nullptr) {
                const auto parentItemLevel = static_cast<int>(paragraphType);
                //
                // Если перед вставляемым элементом что-то уже есть
                //
                if (previousItem != nullptr) {
                    auto previousTextItemParent = previousItem->parent();
                    Q_ASSERT(previousTextItemParent);
                    Q_ASSERT(previousTextItemParent->type() == TextModelItemType::Group);

                    //
                    // Если элемент вставляется после главы с таким же уровнем,
                    // то вставляем его на том же уровне, что и предыдущий
                    //
                    auto previousChapterItem
                        = static_cast<SimpleTextModelChapterItem*>(previousTextItemParent);
                    if (previousChapterItem->level() == parentItemLevel) {
                        d->model->insertItem(parentItem, previousTextItemParent);
                    }
                    //
                    // Если уровень новой главы ниже уровня предыдущей, вставим внутрь
                    //
                    else if (previousChapterItem->level() < parentItemLevel) {
                        d->model->insertItem(parentItem, previousItem);
                    }
                    //
                    // Если уровень новой главы выше уровня предыдущей, вставляем наружу
                    //
                    else {
                        auto previousChapterItemParent = previousChapterItem->parent();
                        while (previousChapterItemParent != nullptr) {
                            Q_ASSERT(previousChapterItemParent->type() == TextModelItemType::Group);
                            const auto grandPreviousChapterItem
                                = static_cast<SimpleTextModelChapterItem*>(
                                    previousChapterItemParent);

                            //
                            // Если уровень деда такой же, то вставляем его на том же уровне
                            //
                            if (grandPreviousChapterItem->level() == parentItemLevel) {
                                d->model->insertItem(parentItem, grandPreviousChapterItem);
                                break;
                            }
                            //
                            // Если уровень деда выше, то вставляем внутрь
                            //
                            else if (grandPreviousChapterItem->level() < parentItemLevel) {
                                d->model->insertItem(parentItem, previousChapterItem);
                                break;
                            }

                            previousChapterItem = grandPreviousChapterItem;
                            previousChapterItemParent = grandPreviousChapterItem->parent();
                        }
                        //
                        // Если подходящего деда не нашлось, то вставляем в конец модели
                        //
                        if (previousChapterItemParent == nullptr) {
                            d->model->appendItem(parentItem);
                        }
                    }
                }
                //
                // Если перед вставляемым ничего нет, просто вставим в самое начало
                //
                else {
                    d->model->prependItem(parentItem);
                }

                //
                // Вставляем сам текстовый элемент в родителя
                //
                d->model->appendItem(textItem, parentItem);

                //
                // Переносим все текстовые элементы идущие после вставленной главы,
                // а также главы более низкого уровня к ней внутрь
                //
                {
                    //
                    // Определим родителя из которого нужно извлекать те самые элементы
                    //
                    auto grandParentItem = [previousItem, parentItem] {
                        //
                        // Если есть предыдущий текстовый элемент
                        //
                        if (previousItem != nullptr) {
                            //
                            // Берём родителя предыдущего текстового элемента
                            //
                            return previousItem->parent();
                        }

                        //
                        // Если перед сценой ничего нет, то берём родителя самой сцены
                        //
                        return parentItem->parent();
                    }();
                    Q_ASSERT(grandParentItem);

                    //
                    // Определим индекс, начиная с которого нужно извлекать текстовые элементы
                    //
                    const int itemIndex = [previousItem, parentItem, grandParentItem] {
                        //
                        // Начинаем со следующего за предыдущим
                        //
                        int indexDelta = 1;
                        //
                        // ... а если вставляемый находится в том же родителе, что и предыдущий,
                        //     значит перейдём ещё на один элемент вперёд
                        //
                        if (parentItem->parent() == grandParentItem) {
                            ++indexDelta;
                        }

                        if (previousItem != nullptr) {
                            if (grandParentItem->type() == TextModelItemType::Group) {
                                return grandParentItem->rowOfChild(previousItem) + indexDelta;
                            }
                        }

                        return grandParentItem->rowOfChild(parentItem) + indexDelta;
                    }();

                    //
                    // Собственно переносим элементы
                    //
                    while (grandParentItem->childCount() > itemIndex) {
                        auto grandParentChildItem = grandParentItem->childAt(itemIndex);
                        if (grandParentChildItem->type() == TextModelItemType::Group) {
                            auto grandParentChildChapter
                                = static_cast<SimpleTextModelChapterItem*>(grandParentChildItem);
                            if (grandParentChildChapter->level() <= parentItemLevel) {
                                break;
                            }
                        }

                        d->model->takeItem(grandParentChildItem, grandParentItem);
                        d->model->appendItem(grandParentChildItem, parentItem);
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
                if (previousItem == nullptr) {
                    d->model->prependItem(textItem);
                }
                //
                // ... после предыдущего элемента
                //
                else {
                    //
                    // ... ставим на уровне с предыдущим элементом
                    //
                    d->model->insertItem(textItem, previousItem);
                }
            }

            auto blockData = new TextBlockData(textItem);
            block.setUserData(blockData);

            previousItem = textItem;
        }
        //
        // Старый блок
        //
        else {
            auto blockData = static_cast<TextBlockData*>(block.userData());
            auto item = blockData->item();

            if (item->type() == TextModelItemType::Text) {
                auto textItem = static_cast<SimpleTextModelTextItem*>(item);
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
            previousItem = item;
        }

        //
        // Запомним новый блок, или обновим старый
        //
        d->positionsToItems.insert_or_assign(block.position(), previousItem);

        //
        // Скорректируем высоту блока, если нужно
        //
        {
            qreal blockHeight = 0.0;
            const auto formats = block.textFormats();
            if (formats.isEmpty()) {
                blockHeight = d->documentTemplate()
                                  .paragraphStyle(paragraphType)
                                  .blockFormat()
                                  .lineHeight();
            } else {
                for (const auto& format : formats) {
                    blockHeight
                        = std::max(blockHeight, TextHelper::fineLineSpacing(format.format.font()));
                }
            }
            if (!qFuzzyCompare(block.blockFormat().lineHeight(), blockHeight)) {
                auto blockFormat = block.blockFormat();
                blockFormat.setLineHeight(blockHeight, QTextBlockFormat::FixedHeight);
                auto cursor = TextCursor(this);
                cursor.setPosition(block.position());
                cursor.setBlockFormat(blockFormat);
            }
        }

        //
        // Переходим к обработке следующего блока
        //
        block = block.next();
    }
}

} // namespace BusinessLayer

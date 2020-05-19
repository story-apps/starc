#include "screenplay_text_document.h"

#include "screenplay_text_block_data.h"
#include "screenplay_text_corrector.h"
#include "screenplay_text_cursor.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_splitter_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>

#include <QTextTable>

using BusinessLayer::ScreenplayBlockStyle;
using BusinessLayer::ScreenplayTemplateFacade;
using BusinessLayer::ScreenplayParagraphType;


namespace Ui
{

enum class DocumentState {
    Undefined,
    Loading,
    Changing,
    Ready
};


class ScreenplayTextDocument::Implementation
{
public:
    explicit Implementation(ScreenplayTextDocument* _document);

    DocumentState state = DocumentState::Undefined;
    BusinessLayer::ScreenplayTextModel* model = nullptr;
    std::map<int, BusinessLayer::ScreenplayTextModelItem*> positionsToItems;
    ScreenplayTextCorrector corrector;
};

ScreenplayTextDocument::Implementation::Implementation(ScreenplayTextDocument* _document)
    : corrector(_document)
{
}


// ****


ScreenplayTextDocument::ScreenplayTextDocument(QObject *_parent)
    : QTextDocument(_parent),
      d(new Implementation(this))
{
    connect(this, &ScreenplayTextDocument::contentsChange, this, &ScreenplayTextDocument::updateModelOnContentChange);
    connect(this, &ScreenplayTextDocument::contentsChange, &d->corrector, &ScreenplayTextCorrector::correct, Qt::QueuedConnection);
}

ScreenplayTextDocument::~ScreenplayTextDocument() = default;

void ScreenplayTextDocument::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->state = DocumentState::Loading;
    d->model = _model;
    d->positionsToItems.clear();

    //
    // Сбрасываем корректор
    //
    d->corrector.clear();

    //
    // Аккуратно очищаем текст, чтобы не сломать форматирование самого документа
    //
    ScreenplayTextCursor cursor(this);
    cursor.select(QTextCursor::Document);
    cursor.deleteChar();

    if (d->model == nullptr) {
        d->state = DocumentState::Ready;
        return;
    }

    //
    // Начинаем операцию вставки
    //
    cursor.beginEditBlock();

    //
    // Последовательно формируем текст документа
    //
    bool isFirstParagraph = true;
    std::function<void(const QModelIndex&)> readDocumentFromModel;
    readDocumentFromModel = [this, &cursor, &isFirstParagraph, &readDocumentFromModel] (const QModelIndex& _parent) {
        using namespace BusinessLayer;
        for (int itemRow = 0; itemRow < d->model->rowCount(_parent); ++itemRow) {
            const auto itemIndex = d->model->index(itemRow, 0, _parent);
            const auto item = d->model->itemForIndex(itemIndex);
            switch (item->type()) {
                case ScreenplayTextModelItemType::Folder: {
                    break;
                }

                case ScreenplayTextModelItemType::Scene: {
                    break;
                }

                case ScreenplayTextModelItemType::Splitter: {
                    const auto splitterItem = static_cast<ScreenplayTextModelSplitterItem*>(item);
                    switch (splitterItem->splitterType()) {
                        case ScreenplayTextModelSplitterItemType::Start: {
                            //
                            // Если это не первый абзац, вставим блок для него
                            //
                            if (!isFirstParagraph) {
                                cursor.insertBlock();
                            }
                            //
                            // ... в противном же случае, новый блок нет необходимости вставлять
                            //
                            else {
                                isFirstParagraph = false;
                            }

                            //
                            // Запомним позицию разделителя
                            //
                            d->positionsToItems.emplace(cursor.position(), splitterItem);

                            //
                            // Назначим блоку перед таблицей формат PageSplitter
                            //
                            auto insertPageSplitter = [&cursor] {
                                const auto style = ScreenplayTemplateFacade::getTemplate().blockStyle(
                                                       ScreenplayParagraphType::PageSplitter);
                                cursor.setBlockFormat(style.blockFormat());
                                cursor.setBlockCharFormat(style.charFormat());
                                cursor.setCharFormat(style.charFormat());
                            };
                            insertPageSplitter();

                            //
                            // Вставляем таблицу
                            //
                            insertTable(cursor);
                            cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, 2);

                            //
                            // Назначим блоку после таблицы формат PageSplitter
                            //
                            insertPageSplitter();

                            //
                            // После вставки таблицы нужно завершить транзакцию изменения документа,
                            // чтобы корректно считывались таблицы в положении курсора
                            //
                            cursor.endEditBlock();
                            cursor.joinPreviousEditBlock();

                            //
                            // Помещаем курсор в первую ячейку для дальнейшего наполнения
                            //
                            cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::MoveAnchor, 2);
                            //
                            // ... и помечаем, что вставлять новый блок нет необходимости
                            //
                            isFirstParagraph = true;

                            break;
                        }

                        case ScreenplayTextModelSplitterItemType::Middle: {
                            //
                            // Если остался не удалённый разделитель, просто пропускаем его
                            //
                            if (!cursor.inTable()) {
                                break;
                            }

                            //
                            // Переходим к следующей колонке
                            //
                            cursor.movePosition(QTextCursor::NextBlock);
                            //
                            // ... и помечаем, что вставлять новый блок нет необходимости
                            //
                            isFirstParagraph = true;
                            break;
                        }

                        case ScreenplayTextModelSplitterItemType::End: {
                            cursor.movePosition(QTextCursor::NextBlock);
                            d->positionsToItems.emplace(cursor.position(), splitterItem);
                            break;
                        }

                        default: break;
                    }

                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    //
                    // Если это не первый абзац, вставим блок для него
                    //
                    if (!isFirstParagraph) {
                        cursor.insertBlock();
                    }
                    //
                    // ... в противном же случае, новый блок нет необходимости вставлять
                    //
                    else {
                        isFirstParagraph = false;
                    }

                    //
                    // Запомним позицию элемента
                    //
                    const auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                    d->positionsToItems.emplace(cursor.position(), textItem);

                    //
                    // Установим стиль блока
                    //
                    const auto currentStyle
                            = ScreenplayTemplateFacade::getTemplate().blockStyle(
                                  textItem->paragraphType());
                    cursor.setBlockFormat(currentStyle.blockFormat(cursor.inTable()));
                    cursor.setBlockCharFormat(currentStyle.charFormat());
                    cursor.setCharFormat(currentStyle.charFormat());

                    //
                    // Вставим текст абзаца
                    //
                    const auto textToInsert = TextHelper::fromHtmlEscaped(textItem->text());
                    cursor.insertText(textToInsert);

                    //
                    // Вставим данные блока
                    //
                    auto blockData = new ScreenplayTextBlockData(textItem);
                    cursor.block().setUserData(blockData);

                    break;
                }

                default: {
                    Q_ASSERT(false);
                    break;
                }
            }

            //
            // Считываем информацию о детях
            //
            readDocumentFromModel(itemIndex);
        }
    };
    readDocumentFromModel({});

    //
    // Завершаем операцию
    //
    cursor.endEditBlock();

    d->state = DocumentState::Ready;
}

int ScreenplayTextDocument::itemPosition(const QModelIndex& _index)
{
    auto item = d->model->itemForIndex(_index);
    if (item == nullptr) {
        return 0;
    }

    while (item->childCount() > 0) {
        item = item->childAt(0);
    }
    for (const auto& [key, value] : d->positionsToItems) {
        if (value == item) {
            return key;
        }
    }

    return 0;
}

void ScreenplayTextDocument::addParagraph(BusinessLayer::ScreenplayParagraphType _type, ScreenplayTextCursor _cursor)
{
    _cursor.beginEditBlock();

    //
    // Если параграф целиком переносится (энтер нажат перед всем текстом блока),
    // необходимо перенести данные блока с текущего на следующий
    //
    if (_cursor.block().text().left(_cursor.positionInBlock()).isEmpty()
        && !_cursor.block().text().isEmpty()) {
        ScreenplayTextBlockData* blockData = nullptr;
        auto block = _cursor.block();
        if (block.userData() != nullptr) {
            blockData = new ScreenplayTextBlockData(static_cast<ScreenplayTextBlockData*>(block.userData()));
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

void ScreenplayTextDocument::setCurrentParagraphType(BusinessLayer::ScreenplayParagraphType _type, const ScreenplayTextCursor& _cursor)
{
    const auto currentParagraphType = ScreenplayBlockStyle::forBlock(_cursor.block());
    if (currentParagraphType == _type) {
        return;
    }

    //
    // Нельзя сменить стиль конечных элементов папок
    //
    if (currentParagraphType == ScreenplayParagraphType::FolderFooter) {
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

void ScreenplayTextDocument::cleanParagraphType(const ScreenplayTextCursor& _cursor)
{
    const auto oldBlockStyle
            = ScreenplayTemplateFacade::getTemplate().blockStyle(
                  ScreenplayBlockStyle::forBlock(_cursor.block()));
    if (!oldBlockStyle.isEmbeddableHeader()) {
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
        const auto currentType = ScreenplayBlockStyle::forBlock(cursor.block());
        if (currentType == oldBlockStyle.embeddableFooter()) {
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
        } else if (currentType == oldBlockStyle.type()) {
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

void ScreenplayTextDocument::applyParagraphType(BusinessLayer::ScreenplayParagraphType _type, const ScreenplayTextCursor& _cursor)
{
    auto cursor = _cursor;
    cursor.beginEditBlock();

    const auto newBlockStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(_type);

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
        // Если в блоке есть выделения, обновляем цвет только тех частей, которые не входят в выделения
        //
        QTextBlock currentBlock = cursor.block();
        if (!currentBlock.textFormats().isEmpty()) {
            const auto formats = currentBlock.textFormats();
            for (const auto& range : formats) {
                if (range.format.boolProperty(ScreenplayBlockStyle::PropertyIsReviewMark)) {
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
    if (newBlockStyle.isEmbeddableHeader()) {
        const auto footerStyle = ScreenplayTemplateFacade::getTemplate().blockStyle(newBlockStyle.embeddableFooter());

        //
        // Вставляем закрывающий блок
        //
        cursor.insertBlock();
        cursor.setBlockCharFormat(footerStyle.charFormat());
        cursor.setBlockFormat(footerStyle.blockFormat(cursor.inTable()));
    }

    cursor.endEditBlock();
}

void ScreenplayTextDocument::splitParagraph(const ScreenplayTextCursor& _cursor)
{
    //
    // Получим курсор для блока, который хочет разделить пользователь
    //
    ScreenplayTextCursor cursor = _cursor;
    cursor.beginEditBlock();

    //
    // Сохраним текущий формат блока
    //
    const auto lastBlockType = ScreenplayBlockStyle::forBlock(cursor.block());
    //
    // Вырезаем выделение, захватываея блоки целиком
    //
    if (cursor.hasSelection()) {
        const auto cursorPositions = std::minmax(cursor.selectionStart(), cursor.selectionEnd());
        cursor.setPosition(cursorPositions.first);
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.setPosition(cursorPositions.second, QTextCursor::KeepAnchor);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    //
    // ... либо только текущий блок
    //
    else {
        cursor.movePosition(QTextCursor::StartOfBlock);
        cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    }
    const QString mime = mimeFromSelection(cursor.selectionStart(), cursor.selectionEnd());
    cursor.removeSelectedText();

    //
    // Назначим блоку перед таблицей формат PageSplitter
    //
    setCurrentParagraphType(ScreenplayParagraphType::PageSplitter, cursor);

    //
    // Вставляем таблицу
    //
    insertTable(cursor);

    //
    // Применяем сохранённый формат блока каждой из колонок
    //
    setCurrentParagraphType(lastBlockType, cursor);
    cursor.movePosition(QTextCursor::NextBlock);
    setCurrentParagraphType(lastBlockType, cursor);
    cursor.movePosition(QTextCursor::NextBlock);

    //
    // Назначим блоку после таблицы формат PageSplitter
    //
    setCurrentParagraphType(ScreenplayParagraphType::PageSplitter, cursor);

    //
    // Вставляем параграф после таблицы - это обязательное условие, чтобы после таблицы всегда
    // оставался один параграф, чтобы пользователь всегда мог выйти из таблицы
    //
    addParagraph(lastBlockType, cursor);

    //
    // Вставляем текст в первую колонку
    //
    insertFromMime(cursor.position(), mime);

    cursor.endEditBlock();
}

void ScreenplayTextDocument::unsplitParagraph(const ScreenplayTextCursor& _cursor)
{
    //
    // Получим курсор для блока, из которого пользователь хочет убрать разделение
    //
    ScreenplayTextCursor cursor = _cursor;
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.beginEditBlock();

    //
    // Идём до начала таблицы
    //
    while (ScreenplayBlockStyle::forBlock(cursor.block()) != ScreenplayParagraphType::PageSplitter) {
        cursor.movePosition(QTextCursor::PreviousBlock);
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
    const QString firstColumnData =
            cursor.selectedText().isEmpty()
            ? QString()
            : mimeFromSelection(cursor.selectionStart(), cursor.selectionEnd());
    cursor.removeSelectedText();

    //
    // Выделяем и сохраняем текст из второй ячейки
    //
    cursor.movePosition(QTextCursor::NextBlock);
    while (cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor));
    cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
    const QString secondColumnData =
            cursor.selectedText().isEmpty()
            ? QString()
            : mimeFromSelection(cursor.selectionStart(), cursor.selectionEnd());
    cursor.removeSelectedText();

    //
    // Удаляем таблицу
    //
    // NOTE: Делается это только таким костылём, как удалить таблицу по-человечески я не нашёл...
    //
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::NextBlock);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.deletePreviousChar();

    //
    // Вставляем текст из удалённых ячеек
    //
    cursor.movePosition(QTextCursor::StartOfBlock);
    cursor.movePosition(QTextCursor::PreviousCharacter);
    const int insertPosition = cursor.position();
    if (!secondColumnData.isEmpty()) {
        cursor.insertBlock();
        insertFromMime(cursor.position(), secondColumnData);
        cursor.setPosition(insertPosition);
    }
    if (!firstColumnData.isEmpty()) {
        cursor.insertBlock();
        insertFromMime(cursor.position(), firstColumnData);
    }

    cursor.endEditBlock();
}

void ScreenplayTextDocument::setCorrectionOptions(bool _needToCorrectCharactersNames, bool _needToCorrectPageBreaks)
{
    d->corrector.setNeedToCorrectCharactersNames(_needToCorrectCharactersNames);
    d->corrector.setNeedToCorrectPageBreaks(_needToCorrectPageBreaks);
}

void ScreenplayTextDocument::updateModelOnContentChange(int _position, int _charsRemoved, int _charsAdded)
{
    if (d->state != DocumentState::Ready) {
        return;
    }

    d->state = DocumentState::Changing;

    using namespace BusinessLayer;

    //
    // Удаляем из модели элементы удалённых блоков и корректируем позиции блоков идущих после правки
    //
    {
        //
        // Собираем элементы которые потенциально могут быть удалены
        //
        std::map<ScreenplayTextModelItem*, int> itemsToDelete;
        if (_charsRemoved > 0) {
            auto itemsToDeleteIter = d->positionsToItems.lower_bound(_position);
            while (itemsToDeleteIter != d->positionsToItems.end()
                   && itemsToDeleteIter->first <= _position + _charsRemoved) {
                itemsToDelete.emplace(itemsToDeleteIter->second, itemsToDeleteIter->first);
                ++itemsToDeleteIter;
            }
        }

        //
        // Проходим по изменённым блокам и фильтруем элементы, которые не были удалены
        //
        auto block = findBlock(_position);
        while (block.isValid()
               && block.position() <= _position + std::max(_charsRemoved, _charsAdded)) {
            if (block.userData() != nullptr) {
                const auto blockData = static_cast<ScreenplayTextBlockData*>(block.userData());
                itemsToDelete.erase(blockData->item());
            }
            block = block.next();
        }

        //
        // Удаляем блоки, которые действительно были удалены из текста
        //
        auto removeIter = d->positionsToItems.lower_bound(_position);
        while (removeIter != d->positionsToItems.end()
               && removeIter->first <= _position + _charsRemoved) {
            //
            // Если элемент действительно удалён - удаляем его из модели
            //
            if (itemsToDelete.find(removeIter->second) != itemsToDelete.end()) {
                auto item = removeIter->second;

                //
                // Если удаляется сцена или папка, нужно удалить соответствующий элемент
                // и перенести элементы к предыдущему группирующему элементу
                //
                bool needToDeleteParent = false;
                if (item->type() == ScreenplayTextModelItemType::Text) {
                    const auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                    needToDeleteParent
                            = textItem->paragraphType() == ScreenplayParagraphType::FolderHeader
                              || textItem->paragraphType() == ScreenplayParagraphType::SceneHeading;
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
                    ScreenplayTextModelItem* previousItem = nullptr;
                    const int itemRow = itemParent->hasParent()
                                        ? itemParent->parent()->rowOfChild(itemParent)
                                        : 0;
                    if (itemRow > 0) {
                        const int previousItemRow = itemRow - 1;
                        previousItem = itemParent->parent()->childAt(previousItemRow);
                    }

                    //
                    // Переносим дочерние элементы на уровень родительского элемента
                    //
                    ScreenplayTextModelItem* lastMovedItem = nullptr;
                    while (itemParent->childCount() > 0) {
                        auto childItem = itemParent->childAt(0);
                        itemParent->takeItem(childItem);

                        //
                        // Папки и сцены переносим на один уровень с текущим элементом
                        //
                        if (childItem->type() == ScreenplayTextModelItemType::Folder
                            || childItem->type() == ScreenplayTextModelItemType::Scene) {
                            if (lastMovedItem == nullptr
                                || lastMovedItem->parent() != itemParent->parent()) {
                                d->model->insertItem(childItem, itemParent);
                            } else {
                                d->model->insertItem(childItem, lastMovedItem);
                            }
                        }
                        //
                        // Все остальные элементы
                        //
                        else {
                            if (lastMovedItem == nullptr) {
                                //
                                // Если перед удаляемым была сцена или папка, то в её конец
                                //
                                if (previousItem != nullptr
                                        && (previousItem->type() == ScreenplayTextModelItemType::Folder
                                            || previousItem->type() == ScreenplayTextModelItemType::Scene)) {
                                    d->model->appendItem(childItem, previousItem);
                                }
                                //
                                // Если перед удаляемым внутри родителя нет ни одного элемента, то вставляем в начало к деду
                                //
                                else if (previousItem == nullptr
                                         && itemParent->parent() != nullptr) {
                                    d->model->prependItem(childItem, itemParent->parent());
                                }
                                //
                                // Во всех остальных случаях просто кладём на один уровень с предыдущим элементом
                                //
                                else {
                                    d->model->insertItem(childItem, previousItem);
                                }
                            }
                            else {
                                d->model->insertItem(childItem, lastMovedItem);
                            }
                        }

                        lastMovedItem = childItem;
                    }

                    //
                    // Удаляем родителя удалённого элемента
                    //
                    d->model->removeItem(itemParent);
                }
            }

            //
            // Убираем информацию о позиции блока, т.к. она могла измениться и будет обновлена далее
            //
            removeIter = d->positionsToItems.erase(removeIter);
        }

        //
        // Корректируем позиции элементов идущих за удаляемым блоком
        //

        auto itemToUpdateIter = removeIter;

        //
        // Формируем мапу элементов со скорректированными позициями
        //
        std::map<int, ScreenplayTextModelItem*> correctedItems;
        for (auto itemIter = itemToUpdateIter; itemIter != d->positionsToItems.end(); ++itemIter) {
            correctedItems.emplace(itemIter->first - _charsRemoved + _charsAdded, itemIter->second);
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
    // Идём с позиции начала, до конца добавления
    //
    auto block = findBlock(_position);
    //
    // ... определим элемент модели для предыдущего блока
    //
    auto previousItem = [block] () -> ScreenplayTextModelItem* {
        if (!block.isValid()) {
            return nullptr;
        }

        auto previousBlock = block.previous();
        if (!previousBlock.isValid()
            || previousBlock.userData() == nullptr) {
            return nullptr;
        }

        auto blockData = static_cast<ScreenplayTextBlockData*>(previousBlock.userData());
        return blockData->item();
    } ();

    //
    // Информация о таблице, в которой находится блок
    //
    struct TableInfo {
        bool inTable = false;
        bool inFirstColumn = false;
    } tableInfo;
    tableInfo = [this, block] () -> TableInfo {
        ScreenplayTextCursor cursor(this);
        cursor.setPosition(block.position());
        return {cursor.inTable(), cursor.inFirstColumn()};
    }();


    while (block.isValid()
           && block.position() <= _position + _charsAdded) {
        const auto paragraphType = ScreenplayBlockStyle::forBlock(block);

        //
        // Новый блок
        //
        if (block.userData() == nullptr) {
            //
            // Разделитель
            //
            if (paragraphType == ScreenplayParagraphType::PageSplitter) {
                ScreenplayTextCursor cursor(this);
                cursor.setPosition(block.position());
                cursor.movePosition(QTextCursor::NextBlock);
                //
                // Сформируем элемент в зависимости от типа разделителя
                //
                ScreenplayTextModelSplitterItem* splitterItem = nullptr;
                if (cursor.inTable() && !tableInfo.inTable) {
                    tableInfo.inTable = true;
                    tableInfo.inFirstColumn = true;
                    splitterItem = new ScreenplayTextModelSplitterItem(ScreenplayTextModelSplitterItemType::Start);
                } else {
                    tableInfo = {};
                    splitterItem = new ScreenplayTextModelSplitterItem(ScreenplayTextModelSplitterItemType::End);
                }
                d->model->insertItem(splitterItem, previousItem);

                //
                // Запомним информацию о разделителе в блоке
                //
                auto blockData = new ScreenplayTextBlockData(splitterItem);
                block.setUserData(blockData);
                previousItem = splitterItem;

                //
                // Запомним новый блок, или обновим старый
                //
                d->positionsToItems.insert_or_assign(block.position(), previousItem);

                block = block.next();
                continue;
            }

            //
            // Если внутри таблицы, то контролируем переход в следующую колонку
            //
            if (tableInfo.inTable && tableInfo.inFirstColumn) {
                ScreenplayTextCursor cursor(this);
                cursor.setPosition(block.position());
                if (!cursor.inFirstColumn()) {
                    //
                    // Формируем, серединный разделитель, но не запоминаем его в блоке,
                    // т.к. в середине таблицы под это просто нет блока
                    //
                    tableInfo.inFirstColumn = false;
                    auto splitterItem = new ScreenplayTextModelSplitterItem(ScreenplayTextModelSplitterItemType::Middle);
                    d->model->insertItem(splitterItem, previousItem);
                    previousItem = splitterItem;
                }
            }

            //
            // Создаём группирующий элемент, если создаётся непосредственно сцена или папка
            //
            ScreenplayTextModelItem* parentItem = nullptr;
            switch (paragraphType) {
                case ScreenplayParagraphType::FolderHeader: {
                    parentItem = new ScreenplayTextModelFolderItem;
                    break;
                }

                case ScreenplayParagraphType::SceneHeading: {
                    parentItem = new ScreenplayTextModelSceneItem;
                    break;
                }

                default: break;
            }

            //
            // Создаём сам текстовый элемент
            //
            auto textItem = new ScreenplayTextModelTextItem;
            textItem->setParagraphType(paragraphType);
            textItem->setText(block.text());

            //
            // Является ли предыдущий элемент футером папки
            //
            const bool previousItemIsFolderFooter = [previousItem] {
                if (!previousItem
                    || previousItem->type() != ScreenplayTextModelItemType::Text) {
                    return false;
                }

                auto textItem = static_cast<ScreenplayTextModelTextItem*>(previousItem);
                return textItem->paragraphType() == ScreenplayParagraphType::FolderFooter;
            }();

            //
            // Добавляем элементы в модель
            //
            // ... в случае, когда вставляем внутрь созданной папки, или сцены
            //
            if (parentItem != nullptr) {
                //
                // Если перед вставляемым элементом что-то уже есть
                //
                if (previousItem != nullptr) {
                    auto previousTextItemParent = previousItem->parent();
                    Q_ASSERT(previousTextItemParent);

                    //
                    // Если элемент вставляется после другой сцены, или после окончания папки,
                    // то вставляем его на том же уровне, что и предыдущий
                    //
                    if (previousTextItemParent->type() == ScreenplayTextModelItemType::Scene
                        || previousItemIsFolderFooter) {
                        d->model->insertItem(parentItem, previousTextItemParent);
                    }
                    //
                    // В противном случае вставляем внутрь папки
                    //
                    else {
                        d->model->insertItem(parentItem, previousItem);
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
                // Если вставляется сцена, то все текстовые элементы идущие после неё нужно
                // положить к ней внутрь
                //
                if (parentItem->type() == ScreenplayTextModelItemType::Scene) {
                    //
                    // Определим родителя из которого нужно извлекать те самые текстовые элементы
                    //
                    auto grandParentItem = [previousItem, previousItemIsFolderFooter, parentItem] {
                        //
                        // Если есть предыдущий текстовый элемент
                        //
                        if (previousItem != nullptr) {
                            //
                            // Если это конец папки, то берём родителя папки
                            //
                            if (previousItemIsFolderFooter) {
                                return previousItem->parent()->parent();
                            }
                            //
                            // В противном случае, берём родителя предыдущего текстового элемента
                            //
                            else {
                                return previousItem->parent();
                            }
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
                    const int itemIndex = [previousItem, previousItemIsFolderFooter, parentItem, grandParentItem] {
                        if (previousItem != nullptr) {
                            if (previousItemIsFolderFooter) {
                                return grandParentItem->rowOfChild(previousItem->parent()) + 2;
                            }
                            else if (grandParentItem->type() == ScreenplayTextModelItemType::Scene) {
                                return grandParentItem->rowOfChild(previousItem) + 1;
                            }
                        }

                        return grandParentItem->rowOfChild(parentItem) + 1;
                    }();

                    //
                    // Собственно переносим элементы
                    //
                    while (grandParentItem->childCount() > itemIndex) {
                        auto grandParentChildItem = grandParentItem->childAt(itemIndex);
                        if (grandParentChildItem->type() != ScreenplayTextModelItemType::Text) {
                            break;
                        }

                        auto grandParentChildTextItem = static_cast<ScreenplayTextModelTextItem*>(grandParentChildItem);
                        if (grandParentChildTextItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
                            break;
                        }

                        d->model->takeItem(grandParentChildItem, grandParentItem);
                        d->model->appendItem(grandParentChildItem, parentItem);
                    }
                }
                //
                // А для папки, если она вставляется после сцены, то нужно перенести все текстовые
                // элементы, которые идут после вставленной папки на уровень самой папки
                //
                else if (previousItem != nullptr
                         && previousItem->parent()->type() == ScreenplayTextModelItemType::Scene) {
                    auto grandParentItem = previousItem->parent();
                    const int lastItemIndex = grandParentItem->rowOfChild(previousItem) + 1;
                    //
                    // Собственно переносим элементы
                    //
                    while (grandParentItem->childCount() > lastItemIndex) {
                        auto grandParentChildItem = grandParentItem->childAt(grandParentItem->childCount() - 1);
                        if (grandParentChildItem->type() != ScreenplayTextModelItemType::Text) {
                            break;
                        }

                        auto grandParentChildTextItem = static_cast<ScreenplayTextModelTextItem*>(grandParentChildItem);
                        if (grandParentChildTextItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
                            break;
                        }

                        d->model->takeItem(grandParentChildItem, grandParentItem);
                        d->model->insertItem(grandParentChildItem, parentItem);
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
                    // ... если блок вставляется после конца папки, то нужно вынести на уровень с папкой
                    //
                    if (previousItemIsFolderFooter) {
                        d->model->insertItem(textItem, previousItem->parent());
                    }
                    //
                    // ... в противном случае ставим на уровне с предыдущим элементом
                    //
                    else {
                        d->model->insertItem(textItem, previousItem);
                    }
                }
            }

            auto blockData = new ScreenplayTextBlockData(textItem);
            block.setUserData(blockData);

            previousItem = textItem;
        }
        //
        // Старый блок
        //
        else {
            auto blockData = static_cast<ScreenplayTextBlockData*>(block.userData());
            auto item = blockData->item();

            if (item->type() == ScreenplayTextModelItemType::Text) {
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                textItem->setParagraphType(paragraphType);
                textItem->setText(block.text());
            }

            d->model->updateItem(item);
            previousItem = item;
        }

        //
        // Запомним новый блок, или обновим старый
        //
        d->positionsToItems.insert_or_assign(block.position(), previousItem);

        //
        // Переходим к обработке следующего блока
        //
        block = block.next();
    }

    d->state = DocumentState::Ready;
}

void ScreenplayTextDocument::insertTable(const ScreenplayTextCursor& _cursor)
{
    const auto scriptTemplate = ScreenplayTemplateFacade::getTemplate();
    const auto tableBorderWidth = scriptTemplate.pageSplitterWidth();
    const qreal tableWidth = pageSize().width()
                             - rootFrame()->frameFormat().leftMargin()
                             - rootFrame()->frameFormat().rightMargin()
                             - 2 * tableBorderWidth;
    const qreal leftColumnWidth = tableWidth * scriptTemplate.leftHalfOfPageWidthPercents() / 100;
    const qreal rightColumnWidth = tableWidth - leftColumnWidth;
    QTextTableFormat format;
    format.setWidth(QTextLength{ QTextLength::FixedLength, tableWidth });
    format.setColumnWidthConstraints({ QTextLength{QTextLength::FixedLength, leftColumnWidth},
                                       QTextLength{QTextLength::FixedLength, rightColumnWidth} });
    format.setBorderStyle(QTextFrameFormat::BorderStyle_None);
    const int qtTableBorderWidth = 2;
    format.setLeftMargin(-2 * tableBorderWidth - qtTableBorderWidth);
    format.setRightMargin(2 * tableBorderWidth - qtTableBorderWidth);
    format.setTopMargin(-2 * tableBorderWidth - qtTableBorderWidth);
    format.setBottomMargin(-tableBorderWidth - qtTableBorderWidth);
    format.setBorder(tableBorderWidth);

    auto cursor = _cursor;
    cursor.insertTable(1, 2, format);
}

} // namespace Ui

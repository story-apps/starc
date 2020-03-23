#include "screenplay_text_document.h"

#include "screenplay_text_block_data.h"
#include "screenplay_text_cursor.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/screenplay_template_facade.h>

#include <utils/helpers/text_helper.h>
#include <utils/shugar.h>


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
    DocumentState state = DocumentState::Undefined;
    BusinessLayer::ScreenplayTextModel* model = nullptr;
    std::map<int, BusinessLayer::ScreenplayTextModelTextItem*> positionsToitems;
};


// ****


ScreenplayTextDocument::ScreenplayTextDocument(QObject *_parent)
    : QTextDocument(_parent),
      d(new Implementation)
{
    connect(this, &ScreenplayTextDocument::contentsChange, this, &ScreenplayTextDocument::updateModelOnContentChange);
}

ScreenplayTextDocument::~ScreenplayTextDocument() = default;

void ScreenplayTextDocument::setModel(BusinessLayer::ScreenplayTextModel* _model)
{
    d->state = DocumentState::Loading;
    d->model = _model;
    d->positionsToitems.clear();

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
        for (int itemRow = 0; itemRow < d->model->rowCount(_parent); ++itemRow) {
            const auto itemIndex = d->model->index(itemRow, 0, _parent);
            const auto item = d->model->itemForIndex(itemIndex);
            switch (item->type()) {
                case BusinessLayer::ScreenplayTextModelItemType::Folder: {
                    break;
                }

                case BusinessLayer::ScreenplayTextModelItemType::Scene: {
                    break;
                }

                case BusinessLayer::ScreenplayTextModelItemType::Text: {
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
                    const auto textItem = static_cast<BusinessLayer::ScreenplayTextModelTextItem*>(item);
                    d->positionsToitems.emplace(cursor.position(), textItem);

                    //
                    // Установим стиль блока
                    //
                    const auto currentStyle
                            = BusinessLayer::ScreenplayTemplateFacade::getTemplate().blockStyle(
                                  textItem->paragraphType());
                    cursor.setBlockFormat(currentStyle.blockFormat());
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
    for (const auto& [key, value] : d->positionsToitems) {
        if (value == item) {
            return key;
        }
    }

    return 0;
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
        std::map<ScreenplayTextModelTextItem*, int> itemsToDelete;
        if (_charsRemoved > 0) {
            auto itemsToDeleteIter = d->positionsToitems.lower_bound(_position);
            while (itemsToDeleteIter != d->positionsToitems.end()
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
        auto removeIter = d->positionsToitems.lower_bound(_position);
        while (removeIter != d->positionsToitems.end()
               && removeIter->first <= _position + _charsRemoved) {
            //
            // Если элемент действительно удалён - удаляем его из модели
            //
            if (itemsToDelete.find(removeIter->second) != itemsToDelete.end()) {
                d->model->removeItem(removeIter->second);
            }

            //
            // Убираем информацию о позиции блока, т.к. она могла измениться и будет обновлена далее
            //
            removeIter = d->positionsToitems.erase(removeIter);
        }

        //
        // Корректируем позиции элементов идущих за удаляемым блоком
        //

        auto itemToUpdateIter = removeIter;

        //
        // Формируем мапу элементов со скорректированными позициями
        //
        std::map<int, ScreenplayTextModelTextItem*> correctedItems;
        for (auto itemIter = itemToUpdateIter; itemIter != d->positionsToitems.end(); ++itemIter) {
            correctedItems.emplace(itemIter->first - _charsRemoved + _charsAdded, itemIter->second);
        }

        //
        // Удаляем элементы со старыми позициями
        //
        d->positionsToitems.erase(itemToUpdateIter, d->positionsToitems.end());

        //
        // И записываем на их место новые элементы
        //
        d->positionsToitems.merge(correctedItems);
    }

    //
    // Идём с позиции начала, до конца добавления
    //
    auto block = findBlock(_position);
    //
    // ... определим элемент модели для предыдущего блока
    //
    auto previousTextItem = [block] () -> ScreenplayTextModelTextItem* {
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
    while (block.isValid()
           && block.position() <= _position + _charsAdded) {
        //
        // Новый блок
        //
        if (block.userData() == nullptr) {
            const auto blockType = ScreenplayBlockStyle::forBlock(block);

            //
            // Создаём группирующий элемент, если создаётся непосредственно сцена или папка
            //
            ScreenplayTextModelItem* parentItem = nullptr;
            switch (blockType) {
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
            textItem->setParagraphType(blockType);
            textItem->setText(block.text());

            //
            // Добавляем элементы в модель
            //
            // ... в случае, когда вставляем внутрь созданной папки, или сцены
            //
            if (parentItem != nullptr) {
                //
                // Если перед вставляемым элементом что-то уже есть
                //
                if (previousTextItem != nullptr) {
                    auto previousTextItemParent = previousTextItem->parent();
                    Q_ASSERT(previousTextItemParent);

                    //
                    // Если элемент вставляется после другой сцены, или после окончания папки,
                    // то вставляем его на том же уровне, что и предыдущий
                    //
                    if (previousTextItemParent->type() == ScreenplayTextModelItemType::Scene
                        || previousTextItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
                        d->model->insertItem(parentItem, previousTextItemParent);
                    }
                    //
                    // В противном случае вставляем внутрь папки
                    //
                    else {
                        d->model->insertItem(parentItem, previousTextItem);
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
                    auto grandParentItem = [previousTextItem, parentItem] {
                        //
                        // Если есть предыдущий текстовый элемент
                        //
                        if (previousTextItem != nullptr) {
                            //
                            // Если это конец папки, то берём родителя папки
                            //
                            if (previousTextItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
                                return previousTextItem->parent()->parent();
                            }
                            //
                            // В противном случае, берём родителя предыдущего текстового элемента
                            //
                            else {
                                return previousTextItem->parent();
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
                    const int itemIndex = [previousTextItem, parentItem, grandParentItem] {
                        if (previousTextItem != nullptr) {
                            if (previousTextItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
                                return grandParentItem->rowOfChild(previousTextItem->parent()) + 2;
                            }
                            else if (grandParentItem->type() == ScreenplayTextModelItemType::Scene) {
                                return grandParentItem->rowOfChild(previousTextItem) + 1;
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
                else if (previousTextItem->parent()->type() == ScreenplayTextModelItemType::Scene) {
                    auto grandParentItem = previousTextItem->parent();
                    const int lastItemIndex = grandParentItem->rowOfChild(previousTextItem) + 1;
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
                if (previousTextItem == nullptr) {
                    d->model->prependItem(textItem);
                }
                //
                // ... после предыдущего элемента
                //
                else {
                    //
                    // ... если блок вставляется после конца папки, то нужно вынести на уровень с папкой
                    //
                    if (previousTextItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
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

            auto blockData = new ScreenplayTextBlockData(textItem);
            block.setUserData(blockData);

            previousTextItem = textItem;
        }
        //
        // Старый блок
        //
        else {
            auto blockData = static_cast<ScreenplayTextBlockData*>(block.userData());
            auto textItem = blockData->item();

            //
            // Если сменился стиль блока, то возможно нужно добавить новую,
            // или удалить предыдущую сцену/папку
            //

            //
            // Если был папкой, то
            //

            textItem->setParagraphType(ScreenplayBlockStyle::forBlock(block));
            textItem->setText(block.text());
            d->model->updateItem(textItem);

            previousTextItem = textItem;
        }

        //
        // Запомним новый блок, или обновим старый
        //
        d->positionsToitems.insert_or_assign(block.position(), previousTextItem);

        //
        // Переходим к обработке следующего блока
        //
        block = block.next();
    }

    d->state = DocumentState::Ready;
}

} // namespace Ui

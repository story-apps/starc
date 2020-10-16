#include "screenplay_text_model.h"

#include "screenplay_text_model_folder_item.h"
#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_splitter_item.h"
#include "screenplay_text_model_text_item.h"
#include "screenplay_text_model_xml.h"

#include <business_layer/templates/screenplay_template.h>

#include <domain/document_object.h>

#include <utils/tools/model_index_path.h>

#include <QDomDocument>
#include <QMimeData>


namespace BusinessLayer
{

namespace {
    const char* kMimeType = "application/x-starc/screenplay/text/item";
}

class ScreenplayTextModel::Implementation
{
public:
    Implementation();

    /**
     * @brief Построить модель структуры из xml хранящегося в документе
     */
    void buildModel(Domain::DocumentObject* _screenplay);

    /**
     * @brief Сформировать xml из данных модели
     */
    QByteArray toXml(Domain::DocumentObject* _screenplay) const;

    /**
     * @brief Обновить номера сцен и реплик
     */
    void updateNumbering();



    /**
     * @brief Корневой элемент дерева
     */
    ScreenplayTextModelFolderItem* rootItem = nullptr;

    /**
     * @brief Модель справочников
     */
    ScreenplayDictionariesModel* dictionariesModel = nullptr;

    /**
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;

    /**
     * @brief Модель локаций
     */
    LocationsModel* locationModel = nullptr;

    /**
     * @brief Последние скопированные данные модели
     */
    struct {
        QModelIndex from;
        QModelIndex to;
        QMimeData* data = nullptr;
    } lastMime;
};

ScreenplayTextModel::Implementation::Implementation()
    : rootItem(new ScreenplayTextModelFolderItem)
{
}

void ScreenplayTextModel::Implementation::buildModel(Domain::DocumentObject* _screenplay)
{
    if (_screenplay == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(_screenplay->content());
    auto documentNode = domDocument.firstChildElement(xml::kDocumentTag);
    auto rootNode = documentNode.firstChildElement();
    while (!rootNode.isNull()) {
        if (rootNode.nodeName() == xml::kFolderTag) {
            rootItem->appendItem(new ScreenplayTextModelFolderItem(rootNode));
        } else if (rootNode.nodeName() == xml::kSceneTag) {
            rootItem->appendItem(new ScreenplayTextModelSceneItem(rootNode));
        } else if (rootNode.nodeName() == xml::kSplitterTag) {
            rootItem->appendItem(new ScreenplayTextModelSplitterItem(rootNode));
        } else {
            rootItem->appendItem(new ScreenplayTextModelTextItem(rootNode));
        }
        rootNode = rootNode.nextSiblingElement();
    }
}

QByteArray ScreenplayTextModel::Implementation::toXml(Domain::DocumentObject* _screenplay) const
{
    if (_screenplay == nullptr) {
        return {};
    }
    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(_screenplay->type()) + "\" version=\"1.0\">\n";
    for (int childIndex = 0; childIndex < rootItem->childCount(); ++childIndex) {
        xml += rootItem->childAt(childIndex)->toXml();
    }
    xml += "</document>";
    return xml;
}

void ScreenplayTextModel::Implementation::updateNumbering()
{
    int sceneNumber = 1;
    int dialogueNumber = 1;
    std::function<void(const ScreenplayTextModelItem*)> updateChildNumbering;
    updateChildNumbering = [&sceneNumber, &dialogueNumber, &updateChildNumbering] (const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder: {
                    updateChildNumbering(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Scene: {
                    updateChildNumbering(childItem);
                    auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(childItem);
                    sceneItem->setNumber(sceneNumber++);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                    if (textItem->paragraphType() == ScreenplayParagraphType::Character
                        && !textItem->isCorrection()) {
                        textItem->setNumber(dialogueNumber++);
                    }
                    break;
                }

                default: break;
            }
        }
    };
    updateChildNumbering(rootItem);
}


// ****


ScreenplayTextModel::ScreenplayTextModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
}

ScreenplayTextModel::~ScreenplayTextModel() = default;

void ScreenplayTextModel::appendItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _parentItem)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (_parentItem->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int itemRow = _parentItem->childCount();
    beginInsertRows(parentIndex, itemRow, itemRow);
    _parentItem->insertItem(itemRow, _item);
    d->updateNumbering();
    endInsertRows();
}

void ScreenplayTextModel::prependItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _parentItem)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (_parentItem->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    beginInsertRows(parentIndex, 0, 0);
    _parentItem->prependItem(_item);
    d->updateNumbering();
    endInsertRows();
}

void ScreenplayTextModel::insertItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _afterSiblingItem)
{
    if (_item == nullptr
        || _afterSiblingItem == nullptr
        || _afterSiblingItem->parent() == nullptr) {
        return;
    }

    auto parent = _afterSiblingItem->parent();

    if (parent->hasChild(_item)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(parent);
    const int itemRowIndex = parent->rowOfChild(_afterSiblingItem) + 1;
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    parent->insertItem(itemRowIndex, _item);
    d->updateNumbering();
    endInsertRows();
}

void ScreenplayTextModel::takeItem(ScreenplayTextModelItem* _item, ScreenplayTextModelItem* _parentItem)
{
    if (_item == nullptr) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem;
    }

    if (!_parentItem->hasChild(_item)) {
        return;
    }

    //
    // Извлекаем элемент
    //
    const QModelIndex parentItemIndex = indexForItem(_item).parent();
    const int itemRowIndex = _parentItem->rowOfChild(_item);
    beginRemoveRows(parentItemIndex, itemRowIndex, itemRowIndex);
    _parentItem->takeItem(_item);
    d->updateNumbering();
    endRemoveRows();
}

void ScreenplayTextModel::removeItem(ScreenplayTextModelItem* _item)
{
    if (_item == nullptr
        || _item->parent() == nullptr) {
        return;
    }

    //
    // Удаляем элемент
    //
    auto itemParent = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = itemParent->rowOfChild(_item);
    beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);
    itemParent->removeItem(_item);
    d->updateNumbering();
    endRemoveRows();
}

void ScreenplayTextModel::updateItem(ScreenplayTextModelItem* _item)
{
    if (_item == nullptr
        || !_item->isChanged()) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
    _item->setChanged(false);

    if (_item->parent() != nullptr) {
        updateItem(_item->parent());
    }
}

QModelIndex ScreenplayTextModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0
        || _row > rowCount(_parent)
        || _column < 0
        || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    auto parentItem = itemForIndex(_parent);
    Q_ASSERT(parentItem);

    auto indexItem = parentItem->childAt(_row);
    if (indexItem == nullptr) {
        return {};
    }

    return createIndex(_row, _column, indexItem);
}

QModelIndex ScreenplayTextModel::parent(const QModelIndex& _child) const
{
    if (!_child.isValid()) {
        return {};
    }

    auto childItem = itemForIndex(_child);
    auto parentItem = childItem->parent();
    if (parentItem == nullptr
        || parentItem == d->rootItem) {
        return {};
    }

    auto grandParentItem = parentItem->parent();
    if (grandParentItem == nullptr) {
        return {};
    }

    const auto row = grandParentItem->rowOfChild(parentItem);
    return createIndex(row, 0, parentItem);
}

int ScreenplayTextModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int ScreenplayTextModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid()
        && _parent.column() != 0) {
        return 0;
    }

    auto item = itemForIndex(_parent);
    if (item == nullptr) {
        return 0;
    }

    return item->childCount();
}

Qt::ItemFlags ScreenplayTextModel::flags(const QModelIndex& _index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled;

    const auto item = itemForIndex(_index);
    if (item->type() == ScreenplayTextModelItemType::Folder) {
        flags |= Qt::ItemIsDropEnabled;
    }

    return flags;
}

QVariant ScreenplayTextModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    auto item = itemForIndex(_index);
    if (item == nullptr) {
        return {};
    }

    return item->data(_role);
}

bool ScreenplayTextModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent) const
{
    Q_UNUSED(_action);
    Q_UNUSED(_row);
    Q_UNUSED(_column);
    Q_UNUSED(_parent);

    return _data->formats().contains(mimeTypes().first());
}

bool ScreenplayTextModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column, const QModelIndex& _parent)
{
    Q_UNUSED(_column);

    //
    // _row - индекс, куда вставлять, если в папку, то он равен -1 и если в самый низ списка, то он тоже равен -1
    //

    if (_data == 0
        || !canDropMimeData(_data, _action, _row, _column, _parent)) {
        return false;
    }

    switch (_action) {
        case Qt::IgnoreAction: {
            return true;
        }

        case Qt::MoveAction:
        case Qt::CopyAction: {
            //
            // Определим элемент после которого планируется вставить данные
            //
            QModelIndex insertAnchorIndex;
            //
            // ... вкладывается первым
            //
            if (_row == 0) {
                insertAnchorIndex = _parent;
            }
            //
            // ... вкладывается в конец
            //
            else if (_row == -1) {
                if (_parent.isValid()) {
                    insertAnchorIndex = _parent;
                } else {
                    insertAnchorIndex = index(d->rootItem->childCount() - 1, 0);
                }
            }
            //
            // ... устанавливается после заданного
            //
            else {
                int delta = 1;
                if (_parent.isValid()
                    && rowCount(_parent) == _row) {
                    //
                    // ... для папок, при вставке в самый конец также нужно учитывать
                    //     текстовый блок закрывающий папку
                    //
                    ++delta;
                }
                insertAnchorIndex = index(_row - delta, 0, _parent);
            }
            if (d->lastMime.from == insertAnchorIndex
                || d->lastMime.to == insertAnchorIndex) {
                return false;
            }
            ScreenplayTextModelItem* insertAnchorItem = itemForIndex(insertAnchorIndex);

            //
            // Если это перемещение внутри модели, то удалим старые элементы
            //
            if (d->lastMime.data == _data) {
                for (int row = d->lastMime.to.row(); row >= d->lastMime.from.row(); --row) {
                    const auto& itemIndex = index(row, 0, d->lastMime.from.parent());
                    auto item = itemForIndex(itemIndex);
                    removeItem(item);
                }
                d->lastMime = {};
            }

            //
            // Вставим перемещаемые элементы
            //
            // ... cчитываем данные и последовательно вставляем в модель
            //
            QDomDocument domDocument;
            domDocument.setContent(_data->data(mimeTypes().first()));
            auto documentNode = domDocument.firstChildElement(xml::kDocumentTag);
            auto contentNode = documentNode.firstChildElement();
            bool isFirstItemHandled = false;
            ScreenplayTextModelItem* lastItem = insertAnchorItem;
            while (!contentNode.isNull()) {
                ScreenplayTextModelItem* newItem = nullptr;
                if (contentNode.nodeName() == xml::kFolderTag) {
                    newItem = new ScreenplayTextModelFolderItem(contentNode);
                } else if (contentNode.nodeName() == xml::kSceneTag) {
                    newItem = new ScreenplayTextModelSceneItem(contentNode);
                } else if (contentNode.nodeName() == xml::kSplitterTag) {
                    newItem = new ScreenplayTextModelSplitterItem(contentNode);
                } else {
                    newItem = new ScreenplayTextModelTextItem(contentNode);
                }

                if (!isFirstItemHandled) {
                    isFirstItemHandled = true;
                    //
                    // Вставить в начало папки
                    //
                    if (_row == 0) {
                        //
                        // При вставке в папку, нужно не забыть про открывающий папку блок
                        //
                        if (lastItem->type() == ScreenplayTextModelItemType::Folder
                            && _parent.isValid()) {
                            insertItem(newItem, lastItem->childAt(0));
                        }
                        //
                        // В остальных слачаях, добавляем в начало
                        //
                        else {
                            prependItem(newItem, lastItem);
                        }
                    }
                    //
                    // Вставить в конец папки
                    //
                    else if (_row == -1) {
                        //
                        // При вставке в папку, нужно не забыть про завершающий папку блок
                        //
                        if (lastItem->type() == ScreenplayTextModelItemType::Folder
                                && _parent.isValid()) {
                            insertItem(newItem, lastItem->childAt(lastItem->childCount() - 2));
                        }
                        //
                        // В остальных случаях просто вставляем после предыдущего
                        //
                        else {
                            insertItem(newItem, lastItem);
                        }
                    }
                    //
                    // Вставить в середину папки
                    //
                    else {
                        insertItem(newItem, lastItem);
                    }
                } else {
                    insertItem(newItem, lastItem);
                }

                lastItem = newItem;
                contentNode = contentNode.nextSiblingElement();
            }

            return true;
        }

        default: {
            return false;
        }
    }
}

QMimeData* ScreenplayTextModel::mimeData(const QModelIndexList& _indexes) const
{
    if (_indexes.isEmpty()) {
        return nullptr;
    }

    //
    // Выделение может быть только последовательным, но нужно учесть ситуацию, когда в выделение
    // попадает родительский элемент и не все его дочерние элементы, поэтому просто использовать
    // последний элемент некорректно, нужно проверить, не входит ли его родитель в выделение
    //

    QModelIndexList correctedIndexes;
    for (const auto& index : _indexes) {
        if (!_indexes.contains(index.parent())) {
            correctedIndexes.append(index);
        }
    }
    if (correctedIndexes.isEmpty()) {
        return nullptr;
    }

    //
    // Для того, чтобы запретить разрывать папки проверяем выделены ли элементы одного уровня
    //
    bool itemsHaveSameParent = true;
    const QModelIndex& genericParent = correctedIndexes.first().parent();
    for (const auto& index : correctedIndexes) {
        if (index.parent() != genericParent) {
            itemsHaveSameParent = false;
            break;
        }
    }
    if (!itemsHaveSameParent) {
        return nullptr;
    }

    //
    // Если выделены элементы одного уровня, то создаём майм-данные
    //

    std::sort(correctedIndexes.begin(), correctedIndexes.end());
    QModelIndex fromIndex = correctedIndexes.first();
    QModelIndex toIndex = correctedIndexes.last();

    auto mimeData = new QMimeData;
    const bool clearUuid = false;
    mimeData->setData(mimeTypes().first(), mimeFromSelection(fromIndex, 0, toIndex, 1, clearUuid).toUtf8());

    d->lastMime = { fromIndex, toIndex, mimeData };

    return mimeData;
}

QStringList ScreenplayTextModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions ScreenplayTextModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions ScreenplayTextModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QString ScreenplayTextModel::mimeFromSelection(const QModelIndex& _from, int _fromPosition,
    const QModelIndex& _to, int _toPosition, bool _clearUuid) const
{
    if (document() == nullptr) {
        return {};
    }

    if (ModelIndexPath(_to) < ModelIndexPath(_from)
        || (_from == _to && _fromPosition >= _toPosition)) {
        return {};
    }

    const auto fromItem = itemForIndex(_from);
    if (fromItem == nullptr) {
        return {};
    }

    const auto toItem = itemForIndex(_to);
    if (toItem == nullptr) {
        return {};
    }


    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += "<document mime-type=\"" + Domain::mimeTypeFor(document()->type()) + "\" version=\"1.0\">\n";

    auto buildXmlFor = [&xml, fromItem, _fromPosition, toItem, _toPosition, _clearUuid]
                       (ScreenplayTextModelItem* _fromItemParent, int _fromItemRow)
    {
        for (int childIndex = _fromItemRow; childIndex < _fromItemParent->childCount(); ++childIndex) {
            const auto childItem = _fromItemParent->childAt(childIndex);

            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder: {
                    const auto folderItem = static_cast<ScreenplayTextModelFolderItem*>(childItem);
                    xml += folderItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                    break;
                }

                case ScreenplayTextModelItemType::Scene: {
                    const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(childItem);
                    xml += sceneItem->toXml(fromItem, _fromPosition, toItem, _toPosition, _clearUuid);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    const auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);

                    //
                    // Не сохраняем закрывающие блоки неоткрытых папок, всё это делается внутри самих папок
                    //
                    if (textItem->paragraphType() == ScreenplayParagraphType::FolderFooter) {
                        break;
                    }

                    if (textItem == fromItem && textItem == toItem) {
                        xml += textItem->toXml(_fromPosition, _toPosition - _fromPosition);
                    } else if (textItem == fromItem) {
                        xml += textItem->toXml(_fromPosition, textItem->text().length() - _fromPosition);
                    } else if (textItem == toItem) {
                        xml += textItem->toXml(0, _toPosition);
                    } else {
                        xml += textItem->toXml();
                    }
                    break;
                }

                default: {
                    xml += childItem->toXml();
                    break;
                }
            }

            const bool recursively = true;
            if (childItem == toItem
                || childItem->hasChild(toItem, recursively)) {
                return true;
            }
        }

        return false;
    };
    auto fromItemParent = fromItem->parent();
    auto fromItemRow = fromItemParent->rowOfChild(fromItem);
    //
    // Если построить нужно начиная с заголовка сцены или папки, то нужно захватить и саму сцену/папку
    //
    if (fromItem->type() == ScreenplayTextModelItemType::Text) {
        const auto textItem = static_cast<ScreenplayTextModelTextItem*>(fromItem);
        if (textItem->paragraphType() == ScreenplayParagraphType::SceneHeading
            || textItem->paragraphType() == ScreenplayParagraphType::FolderHeader) {
            auto newFromItem = fromItemParent;
            fromItemParent = fromItemParent->parent();
            fromItemRow = fromItemParent->rowOfChild(newFromItem);
        }
    }
    //
    // Собственно строим xml с данными выделенного интервала
    //
    while (buildXmlFor(fromItemParent, fromItemRow) != true) {
        auto newFromItem = fromItemParent;
        fromItemParent = fromItemParent->parent();
        fromItemRow = fromItemParent->rowOfChild(newFromItem) + 1; // +1, т.к. текущий мы уже обработали
    }

    xml += "</document>";
    return xml;
}

void ScreenplayTextModel::insertFromMime(const QModelIndex& _index, int _positionInBlock, const QString& _mimeData)
{
    if (!_index.isValid()) {
        return;
    }

    if (_mimeData.isEmpty()) {
        return;
    }

    //
    // Определим элемент, внутрь, или после которого будем вставлять данные
    //
    auto item = itemForIndex(_index);

    //
    // Извлекаем остающийся в блоке текст, если нужно
    //
    QString sourceBlockEndContent;
    QVector<ScreenplayTextModelItem*> lastItemsFromSourceScene;
    if (item->type() == ScreenplayTextModelItemType::Text) {
        auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
        //
        // Если вставка идёт в самое начало блока, то просто переносим блок после вставляемого фрагмента
        //
        if (_positionInBlock == 0
            && textItem->paragraphType() != ScreenplayParagraphType::FolderHeader
            && textItem->paragraphType() != ScreenplayParagraphType::FolderFooter) {
            lastItemsFromSourceScene.append(textItem);
        }
        //
        // В противном случае, дробим блок на две части
        //
        else if (textItem->text().length() > _positionInBlock) {
            const bool clearUuid = true;
            sourceBlockEndContent = mimeFromSelection(_index, _positionInBlock,
                                                     _index, textItem->text().length(), clearUuid);
            textItem->removeText(_positionInBlock);
            updateItem(textItem);
        }
    }

    //
    // Считываем данные и последовательно вставляем в модель
    //
    QDomDocument domDocument;
    domDocument.setContent(_mimeData);
    auto documentNode = domDocument.firstChildElement(xml::kDocumentTag);
    auto contentNode = documentNode.firstChildElement();
    bool isFirstTextItemHandled = false;
    ScreenplayTextModelItem* lastItem = item;
    while (!contentNode.isNull()) {
        ScreenplayTextModelItem* newItem = nullptr;
        //
        // При входе в папку или сцену, если предыдущий текстовый элемент был в сцене,
        // то вставлять их будем не после текстового элемента, а после сцены
        //
        if ((contentNode.nodeName() == xml::kFolderTag
             || contentNode.nodeName() == xml::kSceneTag)
                && (lastItem->type() == ScreenplayTextModelItemType::Text
                    || lastItem->type() == ScreenplayTextModelItemType::Splitter)
                && lastItem->parent()->type() == ScreenplayTextModelItemType::Scene) {
            //
            // ... и при этом вырезаем из него все текстовые блоки, идущие до конца сцены/папки
            //
            auto lastItemParent = lastItem->parent();
            const int maxSize = lastItemParent->rowOfChild(lastItem) + 1;
            while (lastItemParent->childCount() > maxSize) {
                lastItemsFromSourceScene.append(lastItemParent->childAt(maxSize));
            }
            //
            // Собственно берём родителя вместо самого элемента
            //
            lastItem = lastItemParent;
        }


        if (contentNode.nodeName() == xml::kFolderTag) {
            newItem = new ScreenplayTextModelFolderItem(contentNode);
        } else if (contentNode.nodeName() == xml::kSceneTag) {
            newItem = new ScreenplayTextModelSceneItem(contentNode);
        } else if (contentNode.nodeName() == xml::kSplitterTag) {
            newItem = new ScreenplayTextModelSplitterItem(contentNode);
        } else {
            auto newTextItem = new ScreenplayTextModelTextItem(contentNode);
            //
            // Если вставляется текстовый элемент внутрь уже существующего элемента
            //
            if (!isFirstTextItemHandled
                && item->type() == ScreenplayTextModelItemType::Text) {
                //
                // ... то просто объединим их
                //
                isFirstTextItemHandled = true;
                auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
                textItem->mergeWith(newTextItem);
                updateItem(textItem);
                delete newTextItem;
                //
                // ... и исключаем исходный блок из переноса, если он был туда помещён
                //
                lastItemsFromSourceScene.removeAll(textItem);
            }
            //
            // В противном случае вставляем текстовый элемент в модель
            //
            else {
                newItem = newTextItem;
            }
        }

        if (newItem != nullptr) {
            insertItem(newItem, lastItem);
            lastItem = newItem;
        }

        contentNode = contentNode.nextSiblingElement();
    }

    //
    // Если есть оторванный от первого блока текст
    //
    if (!sourceBlockEndContent.isEmpty()) {
        QDomDocument sourceBlockEndContentDocument;
        sourceBlockEndContentDocument.setContent(sourceBlockEndContent);
        auto documentNode = sourceBlockEndContentDocument.firstChildElement(xml::kDocumentTag);
        auto contentNode = documentNode.firstChildElement();
        auto item = new ScreenplayTextModelTextItem(contentNode);
        //
        // ... и последний вставленный элемент был текстовым
        //
        if (lastItem->type() == ScreenplayTextModelItemType::Text) {
            auto lastTextItem = static_cast<ScreenplayTextModelTextItem*>(lastItem);

            //
            // Объединим элементы
            //
            lastTextItem->mergeWith(item);
            updateItem(lastTextItem);
            delete item;
        }
        //
        // В противном случае, вставляем текстовый элемент после последнего вставленного
        //
        else {
            appendItem(item, lastItem);
            lastItem = item;
        }
    }

    //
    // Если есть оторванные текстовые блоки
    //
    if (!lastItemsFromSourceScene.isEmpty()) {
        //
        // Извлечём блоки из родителя
        //
        for (auto item : lastItemsFromSourceScene) {
            if (item->hasParent()) {
                auto itemParent = item->parent();
                takeItem(item, itemParent);

                //
                // Удалим родителя, если у него больше не осталось детей
                // NOTE: актуально для случая, когда в сцене был один пустой абзац заголовка
                //
                if (itemParent->childCount() == 0) {
                    removeItem(itemParent);
                }
            }
        }

        //
        // Просто вставляем их внутрь или после последнего элемента
        //
        for (auto item : lastItemsFromSourceScene) {
            auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
            //
            // Удаляем пустые элементы модели
            //
            if (textItem->text().isEmpty()) {
                delete textItem;
                textItem = nullptr;
                continue;
            }

            if (lastItem->type() == ScreenplayTextModelItemType::Scene) {
                appendItem(item, lastItem);
            } else {
                insertItem(item, lastItem);
            }
            lastItem = item;
        }
    }
}

ScreenplayTextModelItem* ScreenplayTextModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<ScreenplayTextModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

QModelIndex ScreenplayTextModel::indexForItem(ScreenplayTextModelItem* _item) const
{
    if (_item == nullptr) {
        return {};
    }

    int row = 0;
    QModelIndex parent;
    if (_item->hasParent()
        && _item->parent()->hasParent()) {
        row = _item->parent()->rowOfChild(_item);
        parent = indexForItem(_item->parent());
    } else {
        row = d->rootItem->rowOfChild(_item);
    }

    return index(row, 0, parent);
}

void ScreenplayTextModel::setDictionariesModel(ScreenplayDictionariesModel* _model)
{
    d->dictionariesModel = _model;
}

ScreenplayDictionariesModel* ScreenplayTextModel::dictionariesModel() const
{
    return d->dictionariesModel;
}

void ScreenplayTextModel::setCharactersModel(CharactersModel* _model)
{
    d->charactersModel = _model;
}

CharactersModel* ScreenplayTextModel::charactersModel() const
{
    return d->charactersModel;
}

void ScreenplayTextModel::setLocationsModel(LocationsModel* _model)
{
    d->locationModel = _model;
}

LocationsModel* ScreenplayTextModel::locationsModel() const
{
    return d->locationModel;
}

std::chrono::milliseconds ScreenplayTextModel::duration() const
{
    return d->rootItem->duration();
}

void ScreenplayTextModel::recalculateDuration()
{
    std::function<void(const ScreenplayTextModelItem*)> updateChildDuration;
    updateChildDuration = [this, &updateChildDuration] (const ScreenplayTextModelItem* _item) {
        for (int childIndex = 0; childIndex < _item->childCount(); ++childIndex) {
            auto childItem = _item->childAt(childIndex);
            switch (childItem->type()) {
                case ScreenplayTextModelItemType::Folder: {
                    updateChildDuration(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Scene: {
                    updateChildDuration(childItem);
                    break;
                }

                case ScreenplayTextModelItemType::Text: {
                    auto textItem = static_cast<ScreenplayTextModelTextItem*>(childItem);
                    textItem->updateDuration();
                    updateItem(textItem);
                    break;
                }

                default: break;
            }
        }
    };
    updateChildDuration(d->rootItem);
}

void ScreenplayTextModel::initDocument()
{
    //
    // Если документ пустой, создаём первоначальную структуру
    //
    if (document()->content().isEmpty()) {
        auto sceneHeading = new ScreenplayTextModelTextItem;
        sceneHeading->setParagraphType(ScreenplayParagraphType::SceneHeading);
        auto scene = new ScreenplayTextModelSceneItem;
        scene->appendItem(sceneHeading);
        appendItem(scene);
    }
    //
    // А если данные есть, то загрузим их из документа
    //
    else {
        beginResetModel();
        d->buildModel(document());
        endResetModel();
    }

    d->updateNumbering();
    recalculateDuration();
}

void ScreenplayTextModel::clearDocument()
{
    if (!d->rootItem->hasChildren()) {
        return;
    }

    emit beginRemoveRows({}, 0, d->rootItem->childCount() - 1);
    while (d->rootItem->childCount() > 0) {
        d->rootItem->removeItem(d->rootItem->childAt(0));
    }
    emit endRemoveRows();
}

QByteArray ScreenplayTextModel::toXml() const
{
    return d->toXml(document());
}

} // namespace BusinessLayer

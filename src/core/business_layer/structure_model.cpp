#include "structure_model.h"

#include "structure_model_item.h"

#include <domain/document_object.h>

#include <QColor>
#include <QDataStream>
#include <QMimeData>


namespace BusinessLayer
{

namespace {
    const char* kMimeType = "application/x-starc/document";
}

class StructureModel::Implementation
{
public:
    Implementation();

    /**
     * @brief Перестроить модель структуры
     */
    void rebuildModel();


    /**
     * @brief Корневой элемент дерева
     */
    StructureModelItem* rootItem = nullptr;

    /**
     * @brief Документ содержащий структуру
     */
    Domain::DocumentObject* structure = nullptr;

    /**
     * @brief Последние положенные в майм элементы
     */
    mutable QList<StructureModelItem*> m_lastMimeItems;
};

StructureModel::Implementation::Implementation()
    : rootItem(new StructureModelItem({}, Domain::DocumentObjectType::Undefined, {}, {}))
{
}

void StructureModel::Implementation::rebuildModel()
{

}


// ****


StructureModel::StructureModel(QObject* _parent)
    : QAbstractItemModel(_parent),
      d(new Implementation)
{
}

StructureModel::~StructureModel() = default;

void StructureModel::setDocument(Domain::DocumentObject* _document)
{
    if (d->structure == _document) {
        return;
    }

    d->structure = _document;
    d->rebuildModel();
}

void StructureModel::clear()
{
    if (!d->rootItem->hasChildren())

    emit beginRemoveRows({}, 0, d->rootItem->childCount() - 1);
    while (d->rootItem->childCount() > 0) {
        d->rootItem->removeItem(d->rootItem->childAt(0));
    }
    emit endRemoveRows();
}

void StructureModel::prependItem(StructureModelItem* _item, StructureModelItem* _parentItem)
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
    const int itemRowIndex = 0; // т.к. в самое начало
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    _parentItem->prependItem(_item);
    endInsertRows();
}

void StructureModel::appendItem(StructureModelItem* _item, StructureModelItem* _parentItem)
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
    const int itemRowIndex = _parentItem->childCount();
    beginInsertRows(parentIndex, itemRowIndex, itemRowIndex);
    _parentItem->insertItem(itemRowIndex, _item);
    endInsertRows();
}

void StructureModel::insertItem(StructureModelItem* _item, StructureModelItem* _afterSiblingItem)
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
    endInsertRows();
}

void StructureModel::removeItem(StructureModelItem* _item)
{
    if (_item == nullptr
        || _item->parent() == nullptr) {
        return;
    }

    auto itemParent = _item->parent();
    const QModelIndex itemParentIndex = indexForItem(_item).parent();
    const int itemRowIndex = itemParent->rowOfChild(_item);
    beginRemoveRows(itemParentIndex, itemRowIndex, itemRowIndex);
    itemParent->removeItem(_item);
    endRemoveRows();
}

void StructureModel::updateItem(StructureModelItem* _item)
{
    if (_item == nullptr
        || _item->parent() == nullptr) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
}

QModelIndex StructureModel::index(int _row, int _column, const QModelIndex& _parent) const
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

QModelIndex StructureModel::parent(const QModelIndex& _child) const
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

int StructureModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int StructureModel::rowCount(const QModelIndex& _parent) const
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

Qt::ItemFlags StructureModel::flags(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return Qt::NoItemFlags;
    }

    //
    // TODO: Разные варианты
    //

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
}

QVariant StructureModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    auto item = itemForIndex(_index);
    if (item == nullptr) {
        return {};
    }

    switch (_role) {
        case Qt::DisplayRole: {
            return item->name();
        }

        case Qt::DecorationRole: {
            return item->icon();
        }

        case Qt::BackgroundRole: {
            return item->color();
        }

        default: {
            return {};
        }
    }
}

StructureModelItem* StructureModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem;
    }

    auto item = static_cast<StructureModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem;
    }

    return item;
}

bool StructureModel::canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
    int _column, const QModelIndex& _parent) const
{
    Q_UNUSED(_action)
    Q_UNUSED(_row)

    if (!_data->hasFormat(kMimeType)) {
        return false;
    }

    if (_column > 0) {
        return false;
    }

    //
    // TODO: Обработка конкретных случаев что куда можно бросать
    //

    //
    // Во всех остальных случаях можно
    //
    return true;
}

bool StructureModel::dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row,
    int _column, const QModelIndex& _parent)
{
    if (!canDropMimeData(_data, _action, _row, _column, _parent)
        || !_parent.isValid()) {
        return false;
    }

    if (_action == Qt::IgnoreAction) {
        return true;
    }

    //
    // Проверяем, что перемещаются данные из модели
    //
    QByteArray encodedData = _data->data(kMimeType);
    QDataStream stream(&encodedData, QIODevice::ReadOnly);
    int row = 0;
    while (!stream.atEnd()) {
        QUuid itemUuid;
        stream >> itemUuid;
        if (itemUuid != d->m_lastMimeItems[row]->uuid()) {
            //
            // ... если это какие-то внешние данные, то ничего не делаем
            //
            return false;
        }

        ++row;
    }

    //
    // Если с данными всё окей, то перемещаем все элементы по очереди
    //
    const int insertBeforeItemRow = _row - 1; // -1, т.к. нужно вставить перед _row
    const QModelIndex insertBeforeItemIndex = index(insertBeforeItemRow, _column, _parent);
    auto insertBeforeItem = itemForIndex(insertBeforeItemIndex);
    while (!d->m_lastMimeItems.isEmpty()) {
        auto item = d->m_lastMimeItems.takeLast();
        auto itemIndex = indexForItem(item);
        emit beginMoveRows(itemIndex.parent(), itemIndex.row(), itemIndex.row(),
                           insertBeforeItemIndex.parent(), insertBeforeItemRow);
        removeItem(item);
        insertItem(item, insertBeforeItem);
        emit endMoveRows();
    }

    return true;
}

QMimeData* StructureModel::mimeData(const QModelIndexList& _indexes) const
{
    d->m_lastMimeItems.clear();

    if (_indexes.isEmpty()) {
        return nullptr;
    }

    QByteArray encodedData;
    QDataStream stream(&encodedData, QIODevice::WriteOnly);
    for (const QModelIndex& index : _indexes) {
        if (!index.isValid()) {
            continue;
        }

        auto item = itemForIndex(index);
        stream << item->uuid();
        d->m_lastMimeItems << item;
    }

    QMimeData *mimeData = new QMimeData();
    mimeData->setData(kMimeType, encodedData);
    return mimeData;
}

QStringList StructureModel::mimeTypes() const
{
    return { kMimeType };
}

Qt::DropActions StructureModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions StructureModel::supportedDropActions() const
{
    return Qt::MoveAction;
}

QModelIndex StructureModel::indexForItem(StructureModelItem* _item) const
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

} // namespace BusinessLayer

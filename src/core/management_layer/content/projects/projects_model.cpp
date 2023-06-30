#include "projects_model.h"

#include "projects_model_item.h"
#include "projects_model_project_item.h"


namespace BusinessLayer {

class ProjectsModel::Implementation
{
public:
    Implementation();


    QVector<ProjectsModelProjectItem*> projects;

    /**
     * @brief Корневой элемент дерева
     */
    QScopedPointer<ProjectsModelItem> rootItem;
};

ProjectsModel::Implementation::Implementation()
    : rootItem(new ProjectsModelItem)
{
}


// ****


ProjectsModel::ProjectsModel(QObject* _parent)
    : QAbstractItemModel(_parent)
    , d(new Implementation)
{
}

ProjectsModel::~ProjectsModel() = default;

void ProjectsModel::appendItem(ProjectsModelItem* _item, ProjectsModelItem* _parentItem)
{
    appendItems({ _item }, _parentItem);
}

void ProjectsModel::appendItems(const QVector<ProjectsModelItem*>& _items,
                                ProjectsModelItem* _parentItem)
{
    if (_items.isEmpty()) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem.data();
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    const int fromItemRow = _parentItem->childCount();
    const int toItemRow = fromItemRow + _items.size() - 1;
    beginInsertRows(parentIndex, fromItemRow, toItemRow);
    _parentItem->appendItems({ _items.begin(), _items.end() });
    endInsertRows();

    updateItem(_parentItem);
}

void ProjectsModel::prependItem(ProjectsModelItem* _item, ProjectsModelItem* _parentItem)
{
    prependItems({ _item }, _parentItem);
}

void ProjectsModel::prependItems(const QVector<ProjectsModelItem*>& _items,
                                 ProjectsModelItem* _parentItem)
{
    if (_items.isEmpty()) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem.data();
    }

    const QModelIndex parentIndex = indexForItem(_parentItem);
    beginInsertRows(parentIndex, 0, _items.size() - 1);
    _parentItem->prependItems({ _items.begin(), _items.end() });
    endInsertRows();

    updateItem(_parentItem);
}

void ProjectsModel::insertItem(ProjectsModelItem* _item, ProjectsModelItem* _afterSiblingItem)
{
    insertItems({ _item }, _afterSiblingItem);
}

void ProjectsModel::insertItems(const QVector<ProjectsModelItem*>& _items,
                                ProjectsModelItem* _afterSiblingItem)
{
    if (_items.isEmpty()) {
        return;
    }

    if (_afterSiblingItem == nullptr || _afterSiblingItem->parent() == nullptr) {
        return;
    }

    auto parentItem = _afterSiblingItem->parent();
    const QModelIndex parentIndex = indexForItem(parentItem);
    const int fromItemRow = parentItem->rowOfChild(_afterSiblingItem) + 1;
    const int toItemRow = fromItemRow + _items.size() - 1;
    beginInsertRows(parentIndex, fromItemRow, toItemRow);
    parentItem->insertItems(fromItemRow, { _items.begin(), _items.end() });
    endInsertRows();

    updateItem(parentItem);
}

void ProjectsModel::takeItem(ProjectsModelItem* _item)
{
    takeItems(_item, _item, _item->parent());
}

void ProjectsModel::takeItems(ProjectsModelItem* _fromItem, ProjectsModelItem* _toItem,
                              ProjectsModelItem* _parentItem)
{
    if (_fromItem == nullptr || _toItem == nullptr || _fromItem->parent() != _toItem->parent()) {
        return;
    }

    if (_parentItem == nullptr) {
        _parentItem = d->rootItem.data();
    }

    if (!_parentItem->hasChild(_fromItem) || !_parentItem->hasChild(_toItem)) {
        return;
    }

    const QModelIndex parentIndex = indexForItem(_fromItem).parent();
    const int fromItemRow = _parentItem->rowOfChild(_fromItem);
    const int toItemRow = _parentItem->rowOfChild(_toItem);
    Q_ASSERT(fromItemRow <= toItemRow);
    beginRemoveRows(parentIndex, fromItemRow, toItemRow);
    _parentItem->takeItems(fromItemRow, toItemRow);
    endRemoveRows();

    updateItem(_parentItem);
}

void ProjectsModel::removeItem(ProjectsModelItem* _item)
{
    removeItems(_item, _item);
}

void ProjectsModel::removeItems(ProjectsModelItem* _fromItem, ProjectsModelItem* _toItem)
{
    if (_fromItem == nullptr || _fromItem->parent() == nullptr || _toItem == nullptr
        || _toItem->parent() == nullptr || _fromItem->parent() != _toItem->parent()) {
        return;
    }

    auto parentItem = _fromItem->parent();
    const QModelIndex parentIndex = indexForItem(_fromItem).parent();
    const int fromItemRow = parentItem->rowOfChild(_fromItem);
    const int toItemRow = parentItem->rowOfChild(_toItem);
    Q_ASSERT(fromItemRow <= toItemRow);
    beginRemoveRows(parentIndex, fromItemRow, toItemRow);
    parentItem->removeItems(fromItemRow, toItemRow);
    endRemoveRows();

    updateItem(parentItem);
}

void ProjectsModel::moveItem(ProjectsModelItem* _item, ProjectsModelItem* _afterSiblingItem,
                             ProjectsModelItem* _parentItem)
{
    //
    // Попытка переметить тот же самый элемент
    //
    if (_item == _afterSiblingItem) {
        return;
    }

    //
    // Элемент не принадлежит модели
    //
    const auto itemIndex = indexForItem(_item);
    if (!itemIndex.isValid()) {
        return;
    }

    //
    // Определим родителя, если не задан и его индекс
    //
    if (_parentItem == nullptr) {
        _parentItem
            = _afterSiblingItem != nullptr ? _afterSiblingItem->parent() : d->rootItem.data();
    }

    //
    // Перемещение в самое начало
    //
    if (_afterSiblingItem == nullptr) {
        //
        // Если элемент и так самый первый
        //
        if (_item->parent() == _parentItem && itemIndex.row() == 0) {
            return;
        }

        takeItem(_item);
        prependItem(_item, _parentItem);
        return;
    }

    //
    // Перемещение внутри списка
    //
    const auto afterSiblingItemIndex = indexForItem(_afterSiblingItem);
    if (!afterSiblingItemIndex.isValid()) {
        return;
    }
    //
    // ... если элементы и так идут друг за другом
    //
    if (afterSiblingItemIndex.parent() == itemIndex.parent()
        && afterSiblingItemIndex.row() == itemIndex.row() - 1) {
        return;
    }
    //
    // ... собственно перемещение
    //
    takeItem(_item);
    insertItem(_item, _afterSiblingItem);
}

void ProjectsModel::updateItem(ProjectsModelItem* _item)
{
    if (_item == nullptr || !_item->isChanged()) {
        return;
    }

    const QModelIndex indexForUpdate = indexForItem(_item);
    emit dataChanged(indexForUpdate, indexForUpdate);
    _item->setChanged(false);

    if (_item->parent() != nullptr) {
        updateItem(_item->parent());
    }
}

bool ProjectsModel::isEmpty() const
{
    return rowCount() == 0;
}

QModelIndex ProjectsModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
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

QModelIndex ProjectsModel::parent(const QModelIndex& _child) const
{
    if (!_child.isValid()) {
        return {};
    }

    auto childItem = itemForIndex(_child);
    auto parentItem = childItem->parent();
    if (parentItem == nullptr || parentItem == d->rootItem.data()) {
        return {};
    }

    auto grandParentItem = parentItem->parent();
    if (grandParentItem == nullptr) {
        return {};
    }

    const auto row = grandParentItem->rowOfChild(parentItem);
    return createIndex(row, 0, parentItem);
}

int ProjectsModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int ProjectsModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid() && _parent.column() != 0) {
        return 0;
    }

    auto item = itemForIndex(_parent);
    if (item == nullptr) {
        return 0;
    }

    return item->childCount();
}

QVariant ProjectsModel::data(const QModelIndex& _index, int _role) const
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

bool ProjectsModel::moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
                             const QModelIndex& _destinationParent, int _destinationRow)
{
    Q_ASSERT(_count == 1);
    ProjectsModelItem* item = itemForIndex(index(_sourceRow, 0, _sourceParent));
    ProjectsModelItem* afterSiblingItem = nullptr;
    if (_destinationRow != 0) {
        afterSiblingItem = itemForIndex(index(_destinationRow - 1, 0, _destinationParent));
    }
    ProjectsModelItem* parentItem = itemForIndex(_destinationParent);
    moveItem(item, afterSiblingItem, parentItem);
    return true;
}

ProjectsModelItem* ProjectsModel::itemForIndex(const QModelIndex& _index) const
{
    if (!_index.isValid()) {
        return d->rootItem.data();
    }

    auto item = static_cast<ProjectsModelItem*>(_index.internalPointer());
    if (item == nullptr) {
        return d->rootItem.data();
    }

    return item;
}

QModelIndex ProjectsModel::indexForItem(ProjectsModelItem* _item) const
{
    if (_item == nullptr) {
        return {};
    }

    int row = 0;
    QModelIndex parent;
    if (_item->hasParent() && _item->parent()->hasParent()) {
        row = _item->parent()->rowOfChild(_item);
        parent = indexForItem(_item->parent());
    } else {
        row = d->rootItem->rowOfChild(_item);
    }

    return index(row, 0, parent);
}

} // namespace BusinessLayer

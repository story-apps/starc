#include "domain_object.h"

namespace Domain
{

DomainObject::DomainObject(const Identifier& _id) :
    m_id(_id),
    m_isChangesStored(_id.isValid())
{
}

Identifier DomainObject::id() const
{
    return m_id;
}

void DomainObject::setId(const Identifier& _id)
{
    m_id = _id;
}

bool DomainObject::isChangesStored() const
{
    return m_isChangesStored;
}

void DomainObject::markChangesStored()
{
    m_isChangesStored = true;
}

void DomainObject::markChangesNotStored()
{
    m_isChangesStored = false;
}

// ****

DomainObjectsItemModel::DomainObjectsItemModel(QObject* _parent) :
    QAbstractItemModel(_parent)
{
}

QModelIndex DomainObjectsItemModel::index(int _row, int _column, const QModelIndex &_parent) const
{
    if (_row < 0
        || _row > domainObjects().count()
        || _column < 0
        || _column > columnCount(_parent)) {
        return {};
    }

    DomainObject* indexItem = domainObjects().value(_row);
    const QModelIndex resultIndex = createIndex(_row, _column, indexItem);
    return resultIndex;
}

QModelIndex DomainObjectsItemModel::parent(const QModelIndex&) const
{
    return {};
}

int DomainObjectsItemModel::rowCount(const QModelIndex&) const
{
    return m_domainObjects.size();
}

int DomainObjectsItemModel::columnCount(const QModelIndex&) const
{
    return 0;
}

QVariant DomainObjectsItemModel::data(const QModelIndex&, int) const
{
    return {};
}

bool DomainObjectsItemModel::moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
    const QModelIndex& _destinationParent, int _destinationRow)
{
    Q_UNUSED(_sourceParent);
    Q_UNUSED(_destinationParent);

    if (_destinationRow < 0
        || _destinationRow >= m_domainObjects.size()) {
        return false;
    }

    //
    // Реализуем перенос только для одного элемента
    //
    if (_count != 1) {
        return false;
    }

    m_domainObjects.move(_sourceRow, _destinationRow);
    return true;
}

DomainObject *DomainObjectsItemModel::itemForIndex(const QModelIndex &_index) const
{
    return domainObjects().value(_index.row());
}

QModelIndex DomainObjectsItemModel::indexForItem(DomainObject* _object) const
{
    const int objectRow = domainObjects().indexOf(_object);
    return index(objectRow, 0);
}

bool DomainObjectsItemModel::isEmpty() const
{
    return domainObjects().isEmpty();
}

int DomainObjectsItemModel::size() const
{
    return rowCount(QModelIndex());
}

bool DomainObjectsItemModel::contains(DomainObject* _object) const
{
    for (DomainObject* object : domainObjects()) {
        if (object->id() == _object->id()) {
            return true;
        }
    }

    return false;
}

void DomainObjectsItemModel::append(DomainObject* _object)
{
    emit beginInsertRows(QModelIndex(), size(), size());

    m_domainObjects.append(_object);

    emit endInsertRows();
}

void DomainObjectsItemModel::remove(DomainObject* _object)
{
    const int objectRow = m_domainObjects.indexOf(_object);
    beginRemoveRows(QModelIndex(), objectRow, objectRow);
    m_domainObjects.remove(objectRow);
    endRemoveRows();
}

void DomainObjectsItemModel::clear(bool _needRemoveItems)
{
    if (m_domainObjects.isEmpty()) {
        return;
    }

    emit beginRemoveRows(QModelIndex(), 0, size() - 1);

    //
    // Если нужно, то сперва удаляем элементы из памяти
    //
    if (_needRemoveItems) {
        qDeleteAll(m_domainObjects);
    }
    //
    // А затем очищаем список
    //
    m_domainObjects.clear();

    emit endRemoveRows();
}

QVector<DomainObject*>::iterator DomainObjectsItemModel::begin()
{
    return m_domainObjects.begin();
}

QVector<DomainObject*>::const_iterator DomainObjectsItemModel::begin() const
{
    return m_domainObjects.begin();
}

QVector<DomainObject*>::iterator DomainObjectsItemModel::end()
{
    return m_domainObjects.end();
}

QVector<DomainObject*>::const_iterator DomainObjectsItemModel::end() const
{
    return m_domainObjects.end();
}

const QVector<DomainObject*>& DomainObjectsItemModel::domainObjects() const
{
    return m_domainObjects;
}

} // namespace Domain

#include "abstract_model_item.h"

#include <utils/shugar.h>

#include <QVector>


namespace BusinessLayer {

class AbstractModelItem::Implementation
{
public:
    ~Implementation();

    void updateChildRows(int _from);

    AbstractModelItem* parent = nullptr;
    int rowInParent = -1;
    QVector<AbstractModelItem*> children;
    bool changed = false;
};

AbstractModelItem::Implementation::~Implementation()
{
    qDeleteAll(children);
}

void AbstractModelItem::Implementation::updateChildRows(int _from)
{
    for (int row = _from; row < children.size(); ++row) {
        // An item can already belong to a new tree while the old tree is being patched.
        if (children[row]->d->parent && children[row]->d->parent->d.data() == this) {
            children[row]->d->rowInParent = row;
        }
    }
}


// ****


AbstractModelItem::AbstractModelItem()
    : d(new Implementation)
{
}

AbstractModelItem::~AbstractModelItem()
{
    qDeleteAll(d->children);
    d->children.clear();
};

void AbstractModelItem::prependItem(AbstractModelItem* _item)
{
    prependItems({ _item });
}

void AbstractModelItem::prependItems(const QVector<AbstractModelItem*>& _items)
{
    insertItems(0, _items);
}

void AbstractModelItem::appendItem(AbstractModelItem* _item)
{
    appendItems({ _item });
}

void AbstractModelItem::appendItems(const QVector<AbstractModelItem*>& _items)
{
    for (auto item : _items) {
        if (item->parent() == this) {
            continue;
        }

        item->d->parent = this;
        item->d->rowInParent = d->children.size();
        d->children.append(item);
    }

    setChanged(true);
}

void AbstractModelItem::insertItem(int _index, AbstractModelItem* _item)
{
    insertItems(_index, { _item });
}

void AbstractModelItem::insertItems(int _index, const QVector<AbstractModelItem*>& _items)
{
    QVector<AbstractModelItem*> inserted;
    inserted.reserve(_items.size());
    for (auto item : reversed(_items)) {
        if (item->parent() == this) {
            continue;
        }

        item->d->parent = this;
        inserted.append(item);
    }
    if (!inserted.isEmpty()) {
        // Shift the suffix once for the entire batch, rather than once per item.
        d->children.insert(_index, inserted.size(), nullptr);
        for (auto item : reversed(inserted)) {
            d->children[_index++] = item;
        }
        d->updateChildRows(_index - inserted.size());
    }

    setChanged(true);
}

void AbstractModelItem::removeItem(AbstractModelItem* _item)
{
    const auto itemIndex = rowOfChild(_item);

    removeItems(itemIndex, itemIndex);
}

void AbstractModelItem::removeItems(int _fromIndex, int _toIndex)
{
    for (int index = _toIndex; index >= _fromIndex; --index) {
        if (d->children[index]->parent() != this) {
            continue;
        }

        auto item = d->children[index];
        d->children.removeAt(index);
        delete item;
        item = nullptr;
    }

    d->updateChildRows(_fromIndex);
    setChanged(true);
}

void AbstractModelItem::takeItem(AbstractModelItem* _item)
{
    const auto itemIndex = rowOfChild(_item);
    takeItems(itemIndex, itemIndex);
}

void AbstractModelItem::takeItems(int _fromIndex, int _toIndex)
{
    for (int index = _toIndex; index >= _fromIndex; --index) {
        if (d->children[index]->parent() != this) {
            continue;
        }

        d->children[index]->setParent(nullptr);
        d->children.removeAt(index);
    }

    d->updateChildRows(_fromIndex);
    setChanged(true);
}

bool AbstractModelItem::hasParent() const
{
    return d->parent != nullptr;
}

AbstractModelItem* AbstractModelItem::parent() const
{
    return d->parent;
}

void AbstractModelItem::setParent(AbstractModelItem* _parent)
{
    d->parent = _parent;
    d->rowInParent = -1;
}

bool AbstractModelItem::isChildOf(AbstractModelItem* _parent) const
{
    auto parent = this->parent();
    while (parent != nullptr) {
        if (parent == _parent) {
            return true;
        }

        parent = parent->parent();
    }
    return false;
}

bool AbstractModelItem::hasChildren() const
{
    return !d->children.isEmpty();
}

int AbstractModelItem::childCount() const
{
    return d->children.count();
}

bool AbstractModelItem::hasChild(AbstractModelItem* _child, bool _recursively) const
{
    if (!_recursively) {
        return d->children.contains(_child);
    }

    //
    // Рекурсивный поиск
    //

    if (d->children.contains(_child)) {
        return true;
    }

    for (auto child : std::as_const(d->children)) {
        if (child->hasChild(_child, _recursively)) {
            return true;
        }
    }

    return false;
}

int AbstractModelItem::rowOfChild(AbstractModelItem* _child) const
{
    if (_child == nullptr) {
        return -1;
    }

    const auto row = _child->d->rowInParent;
    if (row >= 0 && row < d->children.size() && d->children.at(row) == _child) {
        return row;
    }

    // setParent() also supports drafts and temporary reparenting without changing the
    // child list. Preserve membership-based lookup in these cases.
    const auto actualRow = d->children.indexOf(_child);
    if (_child->d->parent == this) {
        _child->d->rowInParent = actualRow;
    }
    return actualRow;
}

void AbstractModelItem::sortChildren(
    const std::function<bool(AbstractModelItem*, AbstractModelItem*)>& _sorter)
{
    std::sort(d->children.begin(), d->children.end(), _sorter);
    d->updateChildRows(0);
}

AbstractModelItem* AbstractModelItem::childAt(int _index) const
{
    return d->children.value(_index, nullptr);
}

bool AbstractModelItem::isChanged() const
{
    return d->changed;
}

void AbstractModelItem::setChanged(bool _changed)
{
    d->changed = _changed;

    if (_changed) {
        //
        // Сначала обрабатываем собственное изменение элемента
        //
        handleChange();

        //
        // А потом уже изменение его родителя, которое будет учитывать текущее изменение тоже
        //
        if (d->parent != nullptr) {
            d->parent->setChanged(_changed);
        }
    }
}

bool AbstractModelItem::isFilterAccepted(const QString& _text, bool _isCaseSensitive,
                                         int _filterType) const
{
    Q_UNUSED(_text)
    Q_UNUSED(_isCaseSensitive)
    Q_UNUSED(_filterType)
    return false;
}

} // namespace BusinessLayer

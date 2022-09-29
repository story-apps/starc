#include "abstract_model_item.h"

#include <utils/shugar.h>

#include <QVector>


namespace BusinessLayer {

class AbstractModelItem::Implementation
{
public:
    ~Implementation();

    AbstractModelItem* parent = nullptr;
    QVector<AbstractModelItem*> children;
    bool changed = false;
};

AbstractModelItem::Implementation::~Implementation()
{
    qDeleteAll(children);
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
    for (auto item : _items) {
        if (item->parent() == this) {
            continue;
        }

        item->d->parent = this;
        d->children.prepend(item);
    }

    setChanged(true);
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
    for (auto item : reversed(_items)) {
        if (item->parent() == this) {
            continue;
        }

        item->d->parent = this;
        d->children.insert(_index, item);
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
    return d->children.indexOf(_child);
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
        if (d->parent != nullptr) {
            d->parent->setChanged(_changed);
        }

        handleChange();
    }
}

} // namespace BusinessLayer

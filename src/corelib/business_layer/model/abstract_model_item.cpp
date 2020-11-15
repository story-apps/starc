#include "abstract_model_item.h"

#include <QVector>


namespace BusinessLayer
{

class AbstractModelItem::Implementation
{
public:
    AbstractModelItem* parent = nullptr;
    QVector<AbstractModelItem*> children;
    bool changed = false;
};


// ****


AbstractModelItem::AbstractModelItem()
    : d(new Implementation)
{
}

AbstractModelItem::~AbstractModelItem() = default;

void AbstractModelItem::prependItem(AbstractModelItem* _item)
{
    //
    // Устанавливаем себя родителем
    //
    _item->d->parent = this;

    //
    // Добавляем элемент в список детей
    //
    d->children.prepend(_item);
}

void AbstractModelItem::appendItem(AbstractModelItem* _item)
{
    //
    // Устанавливаем себя родителем
    //
    _item->d->parent = this;

    //
    // Добавляем элемент в список детей
    //
    d->children.append(_item);
}

void AbstractModelItem::insertItem(int _index, AbstractModelItem* _item)
{
    _item->d->parent = this;
    d->children.insert(_index, _item);
}

void AbstractModelItem::removeItem(AbstractModelItem* _item)
{
    d->children.removeOne(_item);
    delete _item;
    _item = nullptr;
}

void AbstractModelItem::takeItem(AbstractModelItem* _item)
{
    _item->setParent(nullptr);
    d->children.removeOne(_item);
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

    for (auto child : d->children) {
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

    if (_changed
        && d->parent != nullptr) {
        d->parent->setChanged(_changed);
    }

    handleChange();
}

} // namespace BusinessLayer

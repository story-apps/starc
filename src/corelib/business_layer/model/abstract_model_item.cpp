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

bool AbstractModelItem::hasChildren() const
{
    return !d->children.isEmpty();
}

int AbstractModelItem::childCount() const
{
    return d->children.count();
}

bool AbstractModelItem::hasChild(AbstractModelItem* _child) const
{
    return d->children.contains(_child);
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
}

} // namespace BusinessLayer

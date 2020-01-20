#include "structure_model_item.h"

#include <QColor>
#include <QUuid>
#include <QVector>


namespace BusinessLayer
{

class StructureModelItem::Implementation
{
public:
    explicit Implementation(const QUuid& _uuid, Domain::DocumentObjectType _type, const QString& _name,
        const QColor& _color);

    const QUuid uuid;
    Domain::DocumentObjectType type;
    QString name;
    QColor color;

    StructureModelItem* parent = nullptr;
    QVector<StructureModelItem*> children;
};

StructureModelItem::Implementation::Implementation(const QUuid& _uuid, Domain::DocumentObjectType _type,
    const QString& _name, const QColor& _color)
    : uuid(_uuid),
      type(_type),
      name(_name),
      color(_color)
{
}


// ****


StructureModelItem::StructureModelItem(const QUuid& _uuid, Domain::DocumentObjectType _type,
    const QString& _name, const QColor& _color)
    : d(new Implementation(_uuid, _type, _name, _color))
{
}

StructureModelItem::StructureModelItem(const StructureModelItem& _other)
    : d(new Implementation(_other.d->uuid, _other.d->type, _other.d->name, _other.d->color))
{
}

QUuid StructureModelItem::uuid() const
{
    return d->uuid;
}

Domain::DocumentObjectType StructureModelItem::type() const
{
    return d->type;
}

QString StructureModelItem::name() const
{
    return d->name;
}

void StructureModelItem::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
}

QColor StructureModelItem::color() const
{
    return d->color;
}


//
// Вспомогательные методы для организации работы модели
//

void StructureModelItem::prependItem(StructureModelItem* _item)
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

void StructureModelItem::appendItem(StructureModelItem* _item)
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

void StructureModelItem::insertItem(int _index, StructureModelItem* _item)
{
    _item->d->parent = this;
    d->children.insert(_index, _item);
}

void StructureModelItem::removeItem(StructureModelItem* _item)
{
    d->children.removeOne(_item);
    delete _item;
    _item = nullptr;
}

void StructureModelItem::takeItem(StructureModelItem* _item)
{
    d->children.removeOne(_item);
}

bool StructureModelItem::hasParent() const
{
    return d->parent != nullptr;
}

StructureModelItem* StructureModelItem::parent() const
{
    return d->parent;
}

bool StructureModelItem::hasChildren() const
{
    return !d->children.isEmpty();
}

int StructureModelItem::childCount() const
{
    return d->children.count();
}

bool StructureModelItem::hasChild(StructureModelItem* _child) const
{
    return d->children.contains(_child);
}

int StructureModelItem::rowOfChild(StructureModelItem* _child) const
{
    return d->children.indexOf(_child);
}

StructureModelItem* StructureModelItem::childAt(int _index) const
{
    return d->children.value(_index, nullptr);
}

} // namespace BusinessLayer

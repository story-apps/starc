#include "structure_model_item.h"

#include <domain/document_object.h>

#include <QColor>
#include <QUuid>
#include <QVariant>
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
    : AbstractModelItem(),
      d(new Implementation(_uuid, _type, _name, _color))
{
}

StructureModelItem::StructureModelItem(const StructureModelItem& _other)
    : AbstractModelItem(),
      d(new Implementation(_other.d->uuid, _other.d->type, _other.d->name, _other.d->color))
{
}

const QUuid& StructureModelItem::uuid() const
{
    return d->uuid;
}

Domain::DocumentObjectType StructureModelItem::type() const
{
    return d->type;
}

const QString& StructureModelItem::name() const
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

const QColor& StructureModelItem::color() const
{
    return d->color;
}

QVariant StructureModelItem::data(int _role) const
{
    switch (_role) {
        case Qt::DisplayRole: {
            return name();
        }

        case Qt::DecorationRole: {
            return Domain::iconForType(type());
        }

        case Qt::BackgroundRole: {
            return color();
        }

        default: {
            return {};
        }
    }
}

StructureModelItem* StructureModelItem::parent() const
{
    return static_cast<StructureModelItem*>(AbstractModelItem::parent());
}

StructureModelItem* StructureModelItem::childAt(int _index) const
{
    return static_cast<StructureModelItem*>(AbstractModelItem::childAt(_index));
}

} // namespace BusinessLayer

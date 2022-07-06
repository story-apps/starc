#include "structure_model_item.h"

#include <domain/document_object.h>

#include <QColor>
#include <QUuid>
#include <QVariant>
#include <QVector>


namespace BusinessLayer {

class StructureModelItem::Implementation
{
public:
    explicit Implementation(const QUuid& _uuid, Domain::DocumentObjectType _type,
                            const QString& _name, const QColor& _color, bool _visible);

    const QUuid uuid;
    Domain::DocumentObjectType type;
    QString name;
    QColor color;
    bool visible = true;
    QVector<StructureModelItem*> versions;
};

StructureModelItem::Implementation::Implementation(const QUuid& _uuid,
                                                   Domain::DocumentObjectType _type,
                                                   const QString& _name, const QColor& _color,
                                                   bool _visible)
    : uuid(_uuid)
    , type(_type)
    , name(_name)
    , color(_color)
    , visible(_visible)
{
}


// ****


StructureModelItem::StructureModelItem(const QUuid& _uuid, Domain::DocumentObjectType _type,
                                       const QString& _name, const QColor& _color, bool _visible)
    : AbstractModelItem()
    , d(new Implementation(_uuid, _type, _name, _color, _visible))
{
}

StructureModelItem::StructureModelItem(const StructureModelItem& _other)
    : AbstractModelItem()
    , d(new Implementation(_other.d->uuid, _other.d->type, _other.d->name, _other.d->color,
                           _other.d->visible))
{
}

StructureModelItem::~StructureModelItem()
{
    qDeleteAll(d->versions);
    d->versions.clear();
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
    d->name = _name;
}

const QColor& StructureModelItem::color() const
{
    return d->color;
}

void StructureModelItem::setColor(const QColor& _color)
{
    d->color = _color;
}

bool StructureModelItem::visible() const
{
    return d->visible;
}

void StructureModelItem::setVisible(bool _visible)
{
    d->visible = _visible;
}

bool StructureModelItem::readOnly() const
{
    return d->visible;
}

void StructureModelItem::setReadOnly(bool _readOnly)
{
    d->visible = _readOnly;
}

const QVector<StructureModelItem*>& StructureModelItem::versions() const
{
    return d->versions;
}

StructureModelItem* StructureModelItem::addVersion(StructureModelItem* _version)
{
    _version->setParent(parent());
    d->versions.prepend(_version);
    return _version;
}

StructureModelItem* StructureModelItem::addVersion(const QString& _name, const QColor& _color,
                                                   bool _readOnly)
{
    return addVersion(
        new StructureModelItem(QUuid::createUuid(), type(), _name, _color, _readOnly));
}

void StructureModelItem::setVersions(const QVector<StructureModelItem*>& _versions)
{
    qDeleteAll(d->versions);
    d->versions.clear();

    d->versions = _versions;
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

    case Qt::DecorationPropertyRole: {
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

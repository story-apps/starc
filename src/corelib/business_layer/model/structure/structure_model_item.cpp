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
                            const QString& _name, const QColor& _color, bool _visible,
                            bool _readOnly, bool _comparison);

    QUuid uuid;
    Domain::DocumentObjectType type;
    QString name;
    QColor color;
    bool visible = true;
    bool readOnly = false;
    bool comparison = false;
    QVector<StructureModelItem*> versions;
};

StructureModelItem::Implementation::Implementation(const QUuid& _uuid,
                                                   Domain::DocumentObjectType _type,
                                                   const QString& _name, const QColor& _color,
                                                   bool _visible, bool _readOnly, bool _comparison)
    : uuid(_uuid)
    , type(_type)
    , name(_name)
    , color(_color)
    , visible(_visible)
    , readOnly(_readOnly)
    , comparison(_comparison)
{
}


// ****


StructureModelItem::StructureModelItem(const QUuid& _uuid, Domain::DocumentObjectType _type,
                                       const QString& _name, const QColor& _color, bool _visible,
                                       bool _readOnly, bool _comparison)
    : AbstractModelItem()
    , d(new Implementation(_uuid, _type, _name, _color, _visible, _readOnly, _comparison))
{
}

StructureModelItem::StructureModelItem(const StructureModelItem& _other)
    : AbstractModelItem()
    , d(new Implementation(_other.d->uuid, _other.d->type, _other.d->name, _other.d->color,
                           _other.d->visible, _other.d->readOnly, _other.d->comparison))
{
    //
    // Я не знаю зачем это может понадобится, по идее новый элемент должен создаваться без версий
    //
    Q_ASSERT(_other.versions().isEmpty());
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

bool StructureModelItem::isVisible() const
{
    return d->visible;
}

void StructureModelItem::setVisible(bool _visible)
{
    d->visible = _visible;
}

bool StructureModelItem::isReadOnly() const
{
    return d->readOnly;
}

void StructureModelItem::setReadOnly(bool _readOnly)
{
    d->readOnly = _readOnly;
}

bool StructureModelItem::isComparison() const
{
    return d->comparison;
}

void StructureModelItem::setComparison(bool _comparison)
{
    d->comparison = _comparison;
}

const QVector<StructureModelItem*>& StructureModelItem::versions() const
{
    return d->versions;
}

StructureModelItem* StructureModelItem::addVersion(StructureModelItem* _version)
{
    _version->setParent(parent());
    d->versions.append(_version);
    return _version;
}

StructureModelItem* StructureModelItem::addVersion(const QString& _name, const QColor& _color,
                                                   bool _readOnly, bool _comparison)
{
    const auto visible = true;
    auto version = new StructureModelItem(QUuid::createUuid(), type(), _name, _color, visible,
                                          _readOnly, _comparison);
    version->setParent(parent());
    //
    // Новые драфты добавляются вначало, чтобы идти сразу за текущим драфтом
    //
    if (!_comparison) {
        d->versions.prepend(version);
    }
    //
    // Сравнения же добавляются в конец списка драфтов
    //
    else {
        d->versions.prepend(version);
    }
    return version;
}

void StructureModelItem::removeVersion(int _versionIndex)
{
    if (d->versions.isEmpty()) {
        return;
    }

    if (_versionIndex < 0 || _versionIndex > d->versions.size()) {
        return;
    }

    auto version = d->versions.takeAt(_versionIndex);
    delete version;
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

int StructureModelItem::subtype() const
{
    return 0;
}

QByteArray StructureModelItem::toXml() const
{
    return {};
}

bool StructureModelItem::isEqual(const StructureModelItem* _other) const
{
    return d->uuid == _other->d->uuid && d->type == _other->d->type && d->name == _other->d->name
        && d->color == _other->d->color && d->visible == _other->d->visible
        && d->readOnly == _other->d->readOnly && d->comparison == _other->d->comparison
        && d->versions == _other->d->versions;
}

void StructureModelItem::copyFrom(const StructureModelItem* _other) const
{
    if (_other == nullptr) {
        Q_ASSERT(false);
        return;
    }

    d->uuid = _other->d->uuid;
    d->type = _other->d->type;
    d->name = _other->d->name;
    d->color = _other->d->color;
    d->visible = _other->d->visible;
    d->readOnly = _other->d->readOnly;
    d->comparison = _other->d->comparison;

    qDeleteAll(d->versions);
    d->versions.clear();
    for (auto version : std::as_const(_other->d->versions)) {
        auto newVersion = new StructureModelItem(*version);
        newVersion->setParent(parent());
        d->versions.append(newVersion);
    }
}

} // namespace BusinessLayer

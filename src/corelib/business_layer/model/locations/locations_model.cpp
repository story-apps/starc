#include "locations_model.h"

#include "location_model.h"

#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kLocationsGroupKey("group");
const QLatin1String kLocationKey("location");
const QLatin1String kIdKey("id");
const QLatin1String kNameKey("name");
const QLatin1String kDescriptionKey("description");
const QLatin1String kRectKey("rect");
const QLatin1String kPositionKey("position");
const QLatin1String kLineTypeKey("line");
const QLatin1String kColorKey("color");
} // namespace

class LocationsModel::Implementation
{
public:
    QVector<LocationsGroup> locationsGroups;
    QVector<LocationModel*> locationModels;
    QHash<QString, QPointF> locationsPositions;
};


// ****


bool LocationsGroup::isValid() const
{
    return !id.isNull();
}

bool LocationsGroup::operator==(const LocationsGroup& _other) const
{
    return id == _other.id && rect == _other.rect && name == _other.name
        && description == _other.description && lineType == _other.lineType
        && color == _other.color;
}

bool LocationsGroup::operator!=(const LocationsGroup& _other) const
{
    return !(*this == _other);
}


// ****


LocationsModel::LocationsModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kLocationsGroupKey,
            kLocationKey,
            kIdKey,
            kNameKey,
            kDescriptionKey,
            kRectKey,
            kPositionKey,
            kLineTypeKey,
            kColorKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &LocationsModel::locationsGroupAdded, this,
            &LocationsModel::updateDocumentContent);
    connect(this, &LocationsModel::locationsGroupChanged, this,
            &LocationsModel::updateDocumentContent);
    connect(this, &LocationsModel::locationsGroupRemoved, this,
            &LocationsModel::updateDocumentContent);
    connect(this, &LocationsModel::locationPositionChanged, this,
            &LocationsModel::updateDocumentContent);
}

void LocationsModel::addLocationModel(LocationModel* _locationModel)
{
    if (_locationModel == nullptr || _locationModel->name().isEmpty()
        || d->locationModels.contains(_locationModel)) {
        return;
    }

    const int itemRowIndex = rowCount();
    beginInsertRows({}, itemRowIndex, itemRowIndex);
    d->locationModels.append(_locationModel);
    endInsertRows();
}

void LocationsModel::removeLocationModel(LocationModel* _locationModel)
{
    if (_locationModel == nullptr || !d->locationModels.contains(_locationModel)) {
        return;
    }

    const int itemRowIndex = d->locationModels.indexOf(_locationModel);
    beginRemoveRows({}, itemRowIndex, itemRowIndex);
    d->locationModels.remove(itemRowIndex);
    endRemoveRows();
}

void LocationsModel::createLocation(const QString& _name, const QByteArray& _content)
{
    if (_name.simplified().isEmpty()) {
        return;
    }

    for (const auto location : std::as_const(d->locationModels)) {
        if (location->name() == _name) {
            return;
        }
    }

    emit createLocationRequested(_name, _content);
}

bool LocationsModel::exists(const QString& _name) const
{
    const auto nameCorrected = TextHelper::smartToUpper(_name.simplified());
    for (const auto location : std::as_const(d->locationModels)) {
        if (location->name() == nameCorrected) {
            return true;
        }
    }

    return false;
}

LocationModel* LocationsModel::location(const QUuid& _uuid) const
{
    for (const auto location : std::as_const(d->locationModels)) {
        if (location->document()->uuid() == _uuid) {
            return location;
        }
    }

    return nullptr;
}

LocationModel* LocationsModel::location(const QString& _name) const
{
    for (const auto location : std::as_const(d->locationModels)) {
        if (location->name() == _name) {
            return location;
        }
    }

    return nullptr;
}

QVector<LocationModel*> LocationsModel::locations(const QString& _name) const
{
    QVector<LocationModel*> locations;
    for (const auto location : std::as_const(d->locationModels)) {
        if (location->name() == _name) {
            locations.append(location);
        }
    }
    return locations;
}

void LocationsModel::createLocationsGroup(const QUuid& _groupId)
{
    LocationsGroup group{ _groupId };
    group.name = tr("New group");
    d->locationsGroups.append(group);
    emit locationsGroupAdded(group);
}

void LocationsModel::updateLocationsGroup(const LocationsGroup& _group)
{
    for (auto& group : d->locationsGroups) {
        if (group.id != _group.id) {
            continue;
        }

        if (group != _group) {
            group = _group;
            emit locationsGroupChanged(group);
        }
        return;
    }
}

void LocationsModel::removeLocationsGroup(const QUuid& _groupId)
{
    for (int index = 0; index < d->locationsGroups.size(); ++index) {
        if (d->locationsGroups[index].id != _groupId) {
            continue;
        }

        auto group = d->locationsGroups.takeAt(index);
        emit locationsGroupRemoved(group);
        return;
    }
}

QVector<LocationsGroup> LocationsModel::locationsGroups() const
{
    return d->locationsGroups;
}

QPointF LocationsModel::locationPosition(const QString& _name,
                                         const QPointF& _defaultPosition) const
{
    return d->locationsPositions.value(_name, _defaultPosition);
}

void LocationsModel::setLocationPosition(const QString& _name, const QPointF& _position)
{
    if (d->locationsPositions.value(_name) == _position) {
        return;
    }

    d->locationsPositions[_name] = _position;
    emit locationPositionChanged(_name, _position);
}

QModelIndex LocationsModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    return createIndex(_row, _column, d->locationModels.at(_row));
}

QModelIndex LocationsModel::parent(const QModelIndex& _child) const
{
    Q_UNUSED(_child);
    return {};
}

int LocationsModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int LocationsModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return d->locationModels.size();
}

Qt::ItemFlags LocationsModel::flags(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant LocationsModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    if (_index.row() >= d->locationModels.size()) {
        return {};
    }

    switch (_role) {
    case Qt::DisplayRole:
    case Qt::EditRole: {
        return d->locationModels.at(_index.row())->name();
    }

    default: {
        return {};
    }
    }
}

LocationsModel::~LocationsModel() = default;

void LocationsModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    //
    auto locationsGroupNode = documentNode.firstChildElement(kLocationsGroupKey);
    while (!locationsGroupNode.isNull() && locationsGroupNode.nodeName() == kLocationsGroupKey) {
        LocationsGroup group;
        group.id = QUuid::fromString(locationsGroupNode.attribute(kIdKey));
        group.name = TextHelper::fromHtmlEscaped(locationsGroupNode.attribute(kNameKey));
        group.description
            = TextHelper::fromHtmlEscaped(locationsGroupNode.attribute(kDescriptionKey));
        group.rect = rectFromString(locationsGroupNode.attribute(kRectKey));
        group.lineType = locationsGroupNode.attribute(kLineTypeKey).toInt();
        if (locationsGroupNode.hasAttribute(kColorKey)) {
            group.color = locationsGroupNode.attribute(kColorKey);
        }
        d->locationsGroups.append(group);

        locationsGroupNode = locationsGroupNode.nextSiblingElement();
    }
    //
    auto locationNode = documentNode.firstChildElement(kLocationKey);
    while (!locationNode.isNull() && locationNode.nodeName() == kLocationKey) {
        const auto locationName = TextHelper::fromHtmlEscaped(locationNode.attribute(kNameKey));
        const auto positionText = locationNode.attribute(kPositionKey).split(";");
        Q_ASSERT(positionText.size() == 2);
        const QPointF position(positionText.constFirst().toDouble(),
                               positionText.constLast().toDouble());
        d->locationsPositions[locationName] = position;

        locationNode = locationNode.nextSiblingElement();
    }
}

void LocationsModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray LocationsModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    for (const auto& group : std::as_const(d->locationsGroups)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\" %8=\"%9\" %10=\"%11\" %12/>\n")
                   .arg(kLocationsGroupKey, kIdKey, group.id.toString(), kNameKey,
                        TextHelper::toHtmlEscaped(group.name), kDescriptionKey,
                        TextHelper::toHtmlEscaped(group.description), kRectKey,
                        toString(group.rect), kLineTypeKey, QString::number(group.lineType),
                        (group.color.isValid()
                             ? QString("%1=\"%2\"").arg(kColorKey, group.color.name())
                             : QString()))
                   .toUtf8();
    }
    for (const auto& location : std::as_const(d->locationModels)) {
        const auto locationPosition = this->locationPosition(location->name());
        xml += QString("<%1 %2=\"%3\" %4=\"%5;%6\"/>\n")
                   .arg(kLocationKey, kNameKey, TextHelper::toHtmlEscaped(location->name()),
                        kPositionKey, QString::number(locationPosition.x()),
                        QString::number(locationPosition.y()))
                   .toUtf8();
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

void LocationsModel::applyPatch(const QByteArray& _patch)
{
    //
    // Применяем изменения
    //
    const auto newContent = dmpController().applyPatch(toXml(), _patch);

    //
    // Cчитываем изменённые данные
    //
    QDomDocument domDocument;
    domDocument.setContent(newContent);
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    //
    // Группы локаций
    //
    auto locationsGroupNode = documentNode.firstChildElement(kLocationsGroupKey);
    QVector<LocationsGroup> newLocationsGroups;
    while (!locationsGroupNode.isNull() && locationsGroupNode.nodeName() == kLocationsGroupKey) {
        LocationsGroup group;
        group.id = QUuid::fromString(locationsGroupNode.attribute(kIdKey));
        group.name = TextHelper::fromHtmlEscaped(locationsGroupNode.attribute(kNameKey));
        group.description
            = TextHelper::fromHtmlEscaped(locationsGroupNode.attribute(kDescriptionKey));
        group.rect = rectFromString(locationsGroupNode.attribute(kRectKey));
        group.lineType = locationsGroupNode.attribute(kLineTypeKey).toInt();
        if (locationsGroupNode.hasAttribute(kColorKey)) {
            group.color = locationsGroupNode.attribute(kColorKey);
        }
        newLocationsGroups.append(group);

        locationsGroupNode = locationsGroupNode.nextSiblingElement();
    }
    //
    // ... корректируем текущие группы
    //
    for (int groupIndex = 0; groupIndex < d->locationsGroups.size(); ++groupIndex) {
        const auto& group = d->locationsGroups.at(groupIndex);
        //
        // ... если такая группа осталось актуальной, то оставим её в списке текущих
        //     и удалим из списка новых
        //
        if (newLocationsGroups.contains(group)) {
            newLocationsGroups.removeAll(group);
        }
        //
        // ... если такой группы нет в списке новых, то удалим её из списка текущих
        //
        else {
            removeLocationsGroup(group.id);
            --groupIndex;
        }
    }
    //
    // ... добавляем новые группы
    //
    for (const auto& group : newLocationsGroups) {
        createLocationsGroup(group.id);
        updateLocationsGroup(group);
    }
    //
    // Положения локаций
    //
    auto locationNode = documentNode.firstChildElement(kLocationKey);
    QHash<QString, QPointF> newLocationsPositions;
    while (!locationNode.isNull() && locationNode.nodeName() == kLocationKey) {
        const auto locationName = TextHelper::fromHtmlEscaped(locationNode.attribute(kNameKey));
        const auto positionText = locationNode.attribute(kPositionKey).split(";");
        Q_ASSERT(positionText.size() == 2);
        const QPointF position(positionText.constFirst().toDouble(),
                               positionText.constLast().toDouble());
        newLocationsPositions[locationName] = position;

        locationNode = locationNode.nextSiblingElement();
    }
    //
    // ... корректируем текущие положения
    //
    const auto locationNames = d->locationsPositions.keys();
    for (int positionIndex = 0; positionIndex < locationNames.size(); ++positionIndex) {
        const auto& location = locationNames.at(positionIndex);
        const auto& position = d->locationsPositions[location];
        //
        // ... если такая позиция осталась актуальной, то оставим её в списке текущих
        //     и удалим из списка новых
        //
        if (newLocationsPositions.value(location) == position) {
            newLocationsPositions.remove(location);
        }
        //
        // ... если такой позиции нет в списке новых, то удалим её из списка текущих
        //
        else {
            setLocationPosition(location, position);
        }
    }
    //
    // ... добавляем новые положения
    //
    for (auto iter = newLocationsPositions.begin(); iter != newLocationsPositions.end(); ++iter) {
        setLocationPosition(iter.key(), iter.value());
    }

    reassignContent();
}

} // namespace BusinessLayer

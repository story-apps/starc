#include "worlds_model.h"

#include "world_model.h"

#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kWorldsGroupKey("group");
const QLatin1String kWorldKey("world");
const QLatin1String kIdKey("id");
const QLatin1String kNameKey("name");
const QLatin1String kDescriptionKey("description");
const QLatin1String kRectKey("rect");
const QLatin1String kPositionKey("position");
const QLatin1String kLineTypeKey("line");
const QLatin1String kColorKey("color");
} // namespace

class WorldsModel::Implementation
{
public:
    QVector<WorldsGroup> worldsGroups;
    QVector<WorldModel*> worldModels;
    QHash<QString, QPointF> worldsPositions;
};


// ****


bool WorldsGroup::isValid() const
{
    return !id.isNull();
}

bool WorldsGroup::operator==(const WorldsGroup& _other) const
{
    return id == _other.id && rect == _other.rect && name == _other.name
        && description == _other.description && lineType == _other.lineType
        && color == _other.color;
}

bool WorldsGroup::operator!=(const WorldsGroup& _other) const
{
    return !(*this == _other);
}


// ****


WorldsModel::WorldsModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kWorldsGroupKey,
            kWorldKey,
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
    connect(this, &WorldsModel::worldsGroupAdded, this, &WorldsModel::updateDocumentContent);
    connect(this, &WorldsModel::worldsGroupChanged, this, &WorldsModel::updateDocumentContent);
    connect(this, &WorldsModel::worldsGroupRemoved, this, &WorldsModel::updateDocumentContent);
    connect(this, &WorldsModel::worldPositionChanged, this, &WorldsModel::updateDocumentContent);
}

void WorldsModel::addWorldModel(WorldModel* _worldModel)
{
    if (_worldModel == nullptr || _worldModel->name().isEmpty()
        || d->worldModels.contains(_worldModel)) {
        return;
    }

    const int itemRowIndex = rowCount();
    beginInsertRows({}, itemRowIndex, itemRowIndex);
    d->worldModels.append(_worldModel);
    endInsertRows();

    connect(_worldModel, &WorldModel::nameChanged, this, &WorldsModel::updateDocumentContent);
}

void WorldsModel::removeWorldModel(WorldModel* _worldModel)
{
    if (_worldModel == nullptr || !d->worldModels.contains(_worldModel)) {
        return;
    }

    const int itemRowIndex = d->worldModels.indexOf(_worldModel);
    if (itemRowIndex == -1) {
        return;
    }

    beginRemoveRows({}, itemRowIndex, itemRowIndex);
    d->worldModels.remove(itemRowIndex);
    endRemoveRows();

    disconnect(_worldModel, &WorldModel::nameChanged, this, &WorldsModel::updateDocumentContent);
}

void WorldsModel::createWorld(const QString& _name, const QByteArray& _content)
{
    if (_name.simplified().isEmpty()) {
        return;
    }

    for (const auto world : std::as_const(d->worldModels)) {
        if (world->name() == _name) {
            return;
        }
    }

    emit createWorldRequested(_name, _content);
}

bool WorldsModel::exists(const QString& _name) const
{
    const auto nameCorrected = TextHelper::smartToUpper(_name.simplified());
    for (const auto world : std::as_const(d->worldModels)) {
        if (world->name() == nameCorrected) {
            return true;
        }
    }

    return false;
}

WorldModel* WorldsModel::world(const QUuid& _uuid) const
{
    for (const auto world : std::as_const(d->worldModels)) {
        if (world->document()->uuid() == _uuid) {
            return world;
        }
    }

    return nullptr;
}

WorldModel* WorldsModel::world(const QString& _name) const
{
    for (const auto world : std::as_const(d->worldModels)) {
        if (world->name() == _name) {
            return world;
        }
    }

    return nullptr;
}

QVector<WorldModel*> WorldsModel::worlds(const QString& _name) const
{
    QVector<WorldModel*> worlds;
    for (const auto world : std::as_const(d->worldModels)) {
        if (world->name() == _name) {
            worlds.append(world);
        }
    }
    return worlds;
}

void WorldsModel::createWorldsGroup(const QUuid& _groupId)
{
    WorldsGroup group{ _groupId };
    group.name = tr("New group");
    d->worldsGroups.append(group);
    emit worldsGroupAdded(group);
}

void WorldsModel::updateWorldsGroup(const WorldsGroup& _group)
{
    for (auto& group : d->worldsGroups) {
        if (group.id != _group.id) {
            continue;
        }

        if (group != _group) {
            group = _group;
            emit worldsGroupChanged(group);
        }
        return;
    }
}

void WorldsModel::removeWorldsGroup(const QUuid& _groupId)
{
    for (int index = 0; index < d->worldsGroups.size(); ++index) {
        if (d->worldsGroups[index].id != _groupId) {
            continue;
        }

        auto group = d->worldsGroups.takeAt(index);
        emit worldsGroupRemoved(group);
        return;
    }
}

QVector<WorldsGroup> WorldsModel::worldsGroups() const
{
    return d->worldsGroups;
}

QPointF WorldsModel::worldPosition(const QString& _name, const QPointF& _defaultPosition) const
{
    return d->worldsPositions.value(_name, _defaultPosition);
}

void WorldsModel::setWorldPosition(const QString& _name, const QPointF& _position)
{
    if (d->worldsPositions.value(_name) == _position) {
        return;
    }

    d->worldsPositions[_name] = _position;
    emit worldPositionChanged(_name, _position);
}

QModelIndex WorldsModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    return createIndex(_row, _column, d->worldModels.at(_row));
}

QModelIndex WorldsModel::parent(const QModelIndex& _child) const
{
    Q_UNUSED(_child);
    return {};
}

int WorldsModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int WorldsModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return d->worldModels.size();
}

Qt::ItemFlags WorldsModel::flags(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant WorldsModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    if (_index.row() >= d->worldModels.size()) {
        return {};
    }

    switch (_role) {
    case Qt::DisplayRole:
    case Qt::EditRole: {
        return d->worldModels.at(_index.row())->name();
    }

    default: {
        return {};
    }
    }
}

WorldsModel::~WorldsModel() = default;

void WorldsModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    //
    auto worldsGroupNode = documentNode.firstChildElement(kWorldsGroupKey);
    while (!worldsGroupNode.isNull() && worldsGroupNode.nodeName() == kWorldsGroupKey) {
        WorldsGroup group;
        group.id = QUuid::fromString(worldsGroupNode.attribute(kIdKey));
        group.name = TextHelper::fromHtmlEscaped(worldsGroupNode.attribute(kNameKey));
        group.description = TextHelper::fromHtmlEscaped(worldsGroupNode.attribute(kDescriptionKey));
        group.rect = rectFromString(worldsGroupNode.attribute(kRectKey));
        group.lineType = worldsGroupNode.attribute(kLineTypeKey).toInt();
        if (worldsGroupNode.hasAttribute(kColorKey)) {
            group.color = worldsGroupNode.attribute(kColorKey);
        }
        d->worldsGroups.append(group);

        worldsGroupNode = worldsGroupNode.nextSiblingElement();
    }
    //
    auto worldNode = documentNode.firstChildElement(kWorldKey);
    while (!worldNode.isNull() && worldNode.nodeName() == kWorldKey) {
        const auto worldName = TextHelper::fromHtmlEscaped(worldNode.attribute(kNameKey));
        const auto positionText = worldNode.attribute(kPositionKey).split(";");
        Q_ASSERT(positionText.size() == 2);
        const QPointF position(positionText.constFirst().toDouble(),
                               positionText.constLast().toDouble());
        d->worldsPositions[worldName] = position;

        worldNode = worldNode.nextSiblingElement();
    }
}

void WorldsModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray WorldsModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    for (const auto& group : std::as_const(d->worldsGroups)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\" %8=\"%9\" %10=\"%11\" %12/>\n")
                   .arg(kWorldsGroupKey, kIdKey, group.id.toString(), kNameKey,
                        TextHelper::toHtmlEscaped(group.name), kDescriptionKey,
                        TextHelper::toHtmlEscaped(group.description), kRectKey,
                        toString(group.rect), kLineTypeKey, QString::number(group.lineType),
                        (group.color.isValid()
                             ? QString("%1=\"%2\"").arg(kColorKey, group.color.name())
                             : QString()))
                   .toUtf8();
    }
    for (const auto& world : std::as_const(d->worldModels)) {
        const auto worldPosition = this->worldPosition(world->name());
        xml += QString("<%1 %2=\"%3\" %4=\"%5;%6\"/>\n")
                   .arg(kWorldKey, kNameKey, TextHelper::toHtmlEscaped(world->name()), kPositionKey,
                        QString::number(worldPosition.x()), QString::number(worldPosition.y()))
                   .toUtf8();
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor WorldsModel::applyPatch(const QByteArray& _patch)
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
    // Группы миров
    //
    auto worldsGroupNode = documentNode.firstChildElement(kWorldsGroupKey);
    QVector<WorldsGroup> newWorldsGroups;
    while (!worldsGroupNode.isNull() && worldsGroupNode.nodeName() == kWorldsGroupKey) {
        WorldsGroup group;
        group.id = QUuid::fromString(worldsGroupNode.attribute(kIdKey));
        group.name = TextHelper::fromHtmlEscaped(worldsGroupNode.attribute(kNameKey));
        group.description = TextHelper::fromHtmlEscaped(worldsGroupNode.attribute(kDescriptionKey));
        group.rect = rectFromString(worldsGroupNode.attribute(kRectKey));
        group.lineType = worldsGroupNode.attribute(kLineTypeKey).toInt();
        if (worldsGroupNode.hasAttribute(kColorKey)) {
            group.color = worldsGroupNode.attribute(kColorKey);
        }
        newWorldsGroups.append(group);

        worldsGroupNode = worldsGroupNode.nextSiblingElement();
    }
    //
    // ... корректируем текущие группы
    //
    for (int groupIndex = 0; groupIndex < d->worldsGroups.size(); ++groupIndex) {
        const auto& group = d->worldsGroups.at(groupIndex);
        //
        // ... если такая группа осталось актуальной, то оставим её в списке текущих
        //     и удалим из списка новых
        //
        if (newWorldsGroups.contains(group)) {
            newWorldsGroups.removeAll(group);
        }
        //
        // ... если такой группы нет в списке новых, то удалим её из списка текущих
        //
        else {
            removeWorldsGroup(group.id);
            --groupIndex;
        }
    }
    //
    // ... добавляем новые группы
    //
    for (const auto& group : newWorldsGroups) {
        createWorldsGroup(group.id);
        updateWorldsGroup(group);
    }
    //
    // Положения миров
    //
    auto worldNode = documentNode.firstChildElement(kWorldKey);
    QHash<QString, QPointF> newWorldsPositions;
    while (!worldNode.isNull() && worldNode.nodeName() == kWorldKey) {
        const auto worldName = TextHelper::fromHtmlEscaped(worldNode.attribute(kNameKey));
        const auto positionText = worldNode.attribute(kPositionKey).split(";");
        Q_ASSERT(positionText.size() == 2);
        const QPointF position(positionText.constFirst().toDouble(),
                               positionText.constLast().toDouble());
        newWorldsPositions[worldName] = position;

        worldNode = worldNode.nextSiblingElement();
    }
    //
    // ... корректируем текущие положения
    //
    const auto worldNames = d->worldsPositions.keys();
    for (int positionIndex = 0; positionIndex < worldNames.size(); ++positionIndex) {
        const auto& world = worldNames.at(positionIndex);
        const auto& position = d->worldsPositions[world];
        //
        // ... если такая позиция осталась актуальной, то оставим её в списке текущих
        //     и удалим из списка новых
        //
        if (newWorldsPositions.value(world) == position) {
            newWorldsPositions.remove(world);
        }
        //
        // ... если такой позиции нет в списке новых, то удалим её из списка текущих
        //
        else {
            setWorldPosition(world, position);
        }
    }
    //
    // ... добавляем новые положения
    //
    for (auto iter = newWorldsPositions.begin(); iter != newWorldsPositions.end(); ++iter) {
        setWorldPosition(iter.key(), iter.value());
    }

    return {};
}

} // namespace BusinessLayer

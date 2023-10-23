#include "mind_map_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/string_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNodeKey("node");
const QLatin1String kNodeConnectionKey("connection");
const QLatin1String kNodeGroupKey("group");
const QLatin1String kUuidKey("uuid");
const QLatin1String kFromUuidKey("from_uuid");
const QLatin1String kToUuidKey("to_uuid");
const QLatin1String kNameKey("name");
const QLatin1String kDescriptionKey("description");
const QLatin1String kRectKey("rect");
const QLatin1String kPositionKey("position");
const QLatin1String kLineTypeKey("line");
const QLatin1String kColorKey("color");

} // namespace


bool MindMapNodeConnection::isValid() const
{
    return !fromNodeUuid.isNull() && !toNodeUuid.isNull();
}

bool MindMapNodeConnection::operator==(const MindMapNodeConnection& _other) const
{
    return fromNodeUuid == _other.fromNodeUuid && toNodeUuid == _other.toNodeUuid
        && lineType == _other.lineType && color == _other.color && name == _other.name
        && description == _other.description;
}

bool MindMapNodeConnection::operator!=(const MindMapNodeConnection& _other) const
{
    return !(*this == _other);
}


// ****


bool MindMapNode::isValid() const
{
    return !uuid.isNull();
}

MindMapNodeConnection MindMapNode::connectionTo(const MindMapNode& _node) const
{
    for (const auto& connection : connections) {
        if ((connection.fromNodeUuid == uuid && connection.toNodeUuid == _node.uuid)
            || (connection.fromNodeUuid == _node.uuid && connection.toNodeUuid == uuid)) {
            return connection;
        }
    }

    return {};
}

bool MindMapNode::operator==(const MindMapNode& _other) const
{
    return uuid == _other.uuid && name == _other.name && description == _other.description
        && position == _other.position && color == _other.color
        && connections == _other.connections;
}

bool MindMapNode::operator!=(const MindMapNode& _other) const
{
    return !(*this == _other);
}


// ****


bool MindMapNodeGroup::isValid() const
{
    return !uuid.isNull();
}

bool MindMapNodeGroup::operator==(const MindMapNodeGroup& _other) const
{
    return uuid == _other.uuid && name == _other.name && description == _other.description
        && rect == _other.rect && lineType == _other.lineType && color == _other.color;
}

bool MindMapNodeGroup::operator!=(const MindMapNodeGroup& _other) const
{
    return !(*this == _other);
}


// ****


class MindMapModel::Implementation
{
public:
    QString name;
    QString description;
    QVector<MindMapNode> nodes;
    QVector<MindMapNodeGroup> nodeGroups;
    //    QVector<Domain::DocumentImage> photos;
};


// ****


MindMapModel::MindMapModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{

    connect(this, &MindMapModel::nameChanged, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::descriptionChanged, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeAdded, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeUpdated, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeRemoved, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeConnectionAdded, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeConnectionUpdated, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeConnectionRemoved, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeGroupAdded, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeGroupUpdated, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::nodeGroupRemoved, this, &MindMapModel::updateDocumentContent);
}

MindMapModel::~MindMapModel() = default;

QString MindMapModel::name() const
{
    return d->name;
}

void MindMapModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString MindMapModel::documentName() const
{
    return name();
}

void MindMapModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

QString MindMapModel::description() const
{
    return d->description;
}

void MindMapModel::setDescription(const QString& _description)
{
    if (d->description == _description) {
        return;
    }

    d->description = _description;
    emit descriptionChanged(d->description);
}

void MindMapModel::addNode(const MindMapNode& _node)
{
    if (!_node.isValid() || d->nodes.contains(_node)) {
        return;
    }

    d->nodes.append(_node);
    emit nodeAdded(_node);
}

void MindMapModel::updateNode(const MindMapNode& _node)
{
    if (!_node.isValid()) {
        return;
    }

    for (int index = 0; index < d->nodes.size(); ++index) {
        if (d->nodes[index].uuid != _node.uuid) {
            continue;
        }

        if (d->nodes[index] != _node) {
            d->nodes[index] = _node;
            emit nodeUpdated(_node);
        }
        break;
    }
}

void MindMapModel::removeNode(const QUuid& _nodeUuid)
{
    if (_nodeUuid.isNull()) {
        return;
    }

    for (int index = 0; index < d->nodes.size(); ++index) {
        if (d->nodes[index].uuid != _nodeUuid) {
            continue;
        }

        d->nodes.removeAt(index);
        emit nodeRemoved(_nodeUuid);
        break;
    }
}

QVector<MindMapNode> MindMapModel::nodes() const
{
    return d->nodes;
}

void MindMapModel::addNodeConnection(const MindMapNodeConnection& _nodeConnection)
{
    if (!_nodeConnection.isValid()) {
        return;
    }

    for (int fromNodeIndex = 0; fromNodeIndex < d->nodes.size(); ++fromNodeIndex) {
        if (d->nodes[fromNodeIndex].uuid != _nodeConnection.fromNodeUuid) {
            continue;
        }

        for (int toNodeIndex = 0; toNodeIndex < d->nodes.size(); ++toNodeIndex) {
            if (d->nodes[toNodeIndex].uuid != _nodeConnection.toNodeUuid) {
                continue;
            }


            d->nodes[fromNodeIndex].connections.append(_nodeConnection);
            d->nodes[toNodeIndex].connections.append(_nodeConnection);
            emit nodeUpdated(d->nodes[fromNodeIndex]);
            emit nodeUpdated(d->nodes[toNodeIndex]);
            emit nodeConnectionAdded(_nodeConnection);
            break;
        }
        break;
    }
}

void MindMapModel::updateNodeConnection(const MindMapNodeConnection& _nodeConnection)
{
    if (!_nodeConnection.isValid()) {
        return;
    }

    for (int fromNodeIndex = 0; fromNodeIndex < d->nodes.size(); ++fromNodeIndex) {
        if (d->nodes[fromNodeIndex].uuid != _nodeConnection.fromNodeUuid) {
            continue;
        }

        auto& node = d->nodes[fromNodeIndex];
        for (int connectionIndex = 0; connectionIndex < node.connections.size();
             ++connectionIndex) {
            if (node.connections[connectionIndex].toNodeUuid != _nodeConnection.toNodeUuid) {
                continue;
            }

            node.connections[connectionIndex] = _nodeConnection;
            emit nodeUpdated(d->nodes[fromNodeIndex]);
            break;
        }

        break;
    }

    for (int toNodeIndex = 0; toNodeIndex < d->nodes.size(); ++toNodeIndex) {
        if (d->nodes[toNodeIndex].uuid != _nodeConnection.toNodeUuid) {
            continue;
        }

        auto& node = d->nodes[toNodeIndex];
        for (int connectionIndex = 0; connectionIndex < node.connections.size();
             ++connectionIndex) {
            if (node.connections[connectionIndex].fromNodeUuid != _nodeConnection.fromNodeUuid) {
                continue;
            }

            node.connections[connectionIndex] = _nodeConnection;
            emit nodeUpdated(d->nodes[toNodeIndex]);
            break;
        }

        break;
    }

    emit nodeConnectionUpdated(_nodeConnection);
}

void MindMapModel::removeNodeConnection(const QUuid& _fromNodeUuid, const QUuid& _toNodeUuid)
{
    if (_fromNodeUuid.isNull() || _toNodeUuid.isNull()) {
        return;
    }

    int fromNodeIndex = 0;
    for (; fromNodeIndex < d->nodes.size(); ++fromNodeIndex) {
        if (d->nodes[fromNodeIndex].uuid != _fromNodeUuid) {
            continue;
        }

        auto& node = d->nodes[fromNodeIndex];
        for (int connectionIndex = 0; connectionIndex < node.connections.size();
             ++connectionIndex) {
            if (node.connections[connectionIndex].toNodeUuid != _toNodeUuid) {
                continue;
            }

            node.connections.removeAt(connectionIndex);
            break;
        }

        break;
    }

    int toNodeIndex = 0;
    for (; toNodeIndex < d->nodes.size(); ++toNodeIndex) {
        if (d->nodes[toNodeIndex].uuid != _toNodeUuid) {
            continue;
        }

        auto& node = d->nodes[toNodeIndex];
        for (int connectionIndex = 0; connectionIndex < node.connections.size();
             ++connectionIndex) {
            if (node.connections[connectionIndex].fromNodeUuid != _fromNodeUuid) {
                continue;
            }

            node.connections.removeAt(connectionIndex);
            break;
        }

        break;
    }

    //
    // Уведомляем об обновлении ячеек после того, как соединения были удалены из обеих ячеек
    //
    emit nodeConnectionRemoved(_fromNodeUuid, _toNodeUuid);
    emit nodeUpdated(d->nodes[fromNodeIndex]);
    emit nodeUpdated(d->nodes[toNodeIndex]);
}

void MindMapModel::addNodeGroup(const MindMapNodeGroup& _group)
{
    d->nodeGroups.append(_group);
    emit nodeGroupAdded(_group);
}

void MindMapModel::updateNodeGroup(const MindMapNodeGroup& _group)
{
    if (!_group.isValid()) {
        return;
    }

    for (int index = 0; index < d->nodeGroups.size(); ++index) {
        if (d->nodeGroups[index].uuid != _group.uuid) {
            continue;
        }

        if (d->nodeGroups[index] != _group) {
            d->nodeGroups[index] = _group;
            emit nodeGroupUpdated(_group);
        }
        break;
    }
}

void MindMapModel::removeNodeGroup(const QUuid& _groupUuid)
{
    if (_groupUuid.isNull()) {
        return;
    }

    for (int index = 0; index < d->nodeGroups.size(); ++index) {
        if (d->nodeGroups[index].uuid != _groupUuid) {
            continue;
        }

        d->nodeGroups.removeAt(index);
        emit nodeGroupRemoved(_groupUuid);
        break;
    }
}

QVector<MindMapNodeGroup> MindMapModel::nodeGroups() const
{
    return d->nodeGroups;
}

void MindMapModel::initImageWrapper()
{
    //    connect(imageWrapper(), &AbstractImageWrapper::imageUpdated, this,
    //            [this](const QUuid& _uuid, const QPixmap& _image) {
    //                for (auto& photo : d->photos) {
    //                    if (photo.uuid == _uuid) {
    //                        photo.image = _image;
    //                        emit photosChanged(d->photos);
    //                        break;
    //                    }
    //                }
    //            });
}

void MindMapModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto load = [&documentNode](const QString& _key) {
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
    };
    d->name = load(kNameKey);
    d->description = load(kDescriptionKey);

    auto nodeNode = documentNode.firstChildElement(kNodeKey);
    while (!nodeNode.isNull() && nodeNode.nodeName() == kNodeKey) {
        MindMapNode node;
        node.uuid = QUuid::fromString(nodeNode.attribute(kUuidKey));
        node.name = TextHelper::fromHtmlEscaped(nodeNode.attribute(kNameKey));
        node.description = TextHelper::fromHtmlEscaped(nodeNode.attribute(kDescriptionKey));
        const auto positionText = nodeNode.attribute(kPositionKey).split(";");
        Q_ASSERT(positionText.size() == 2);
        node.position
            = QPointF(positionText.constFirst().toDouble(), positionText.constLast().toDouble());
        if (nodeNode.hasAttribute(kColorKey)) {
            node.color = nodeNode.attribute(kColorKey);
        }
        auto nodeConnectionNode = nodeNode.firstChildElement(kNodeConnectionKey);
        while (!nodeConnectionNode.isNull()
               && nodeConnectionNode.nodeName() == kNodeConnectionKey) {
            MindMapNodeConnection connection;
            connection.fromNodeUuid = QUuid::fromString(nodeConnectionNode.attribute(kFromUuidKey));
            connection.toNodeUuid = QUuid::fromString(nodeConnectionNode.attribute(kToUuidKey));
            connection.name = TextHelper::fromHtmlEscaped(nodeConnectionNode.attribute(kNameKey));
            connection.description
                = TextHelper::fromHtmlEscaped(nodeConnectionNode.attribute(kDescriptionKey));
            connection.lineType = nodeConnectionNode.attribute(kLineTypeKey).toInt();
            if (nodeConnectionNode.hasAttribute(kColorKey)) {
                connection.color = nodeConnectionNode.attribute(kColorKey);
            }
            node.connections.append(connection);

            nodeConnectionNode = nodeConnectionNode.nextSiblingElement();
        }
        d->nodes.append(node);

        nodeNode = nodeNode.nextSiblingElement();
    }
    auto nodeGroupNode = documentNode.firstChildElement(kNodeGroupKey);
    while (!nodeGroupNode.isNull() && nodeGroupNode.nodeName() == kNodeGroupKey) {
        MindMapNodeGroup group;
        group.uuid = QUuid::fromString(nodeGroupNode.attribute(kUuidKey));
        group.name = TextHelper::fromHtmlEscaped(nodeGroupNode.attribute(kNameKey));
        group.description = TextHelper::fromHtmlEscaped(nodeGroupNode.attribute(kDescriptionKey));
        group.rect = rectFromString(nodeGroupNode.attribute(kRectKey));
        group.lineType = nodeGroupNode.attribute(kLineTypeKey).toInt();
        if (nodeGroupNode.hasAttribute(kColorKey)) {
            group.color = nodeGroupNode.attribute(kColorKey);
        }
        d->nodeGroups.append(group);

        nodeGroupNode = nodeGroupNode.nextSiblingElement();
    }
}

void MindMapModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray MindMapModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    auto save = [&xml](const QString& _key, const QString& _value) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(_key, TextHelper::toHtmlEscaped(_value))
                   .toUtf8();
    };
    save(kNameKey, d->name);
    save(kDescriptionKey, d->description);
    for (const auto& node : std::as_const(d->nodes)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\" %8=\"%9;%10\" %11>\n")
                   .arg(kNodeKey, kUuidKey, node.uuid.toString(), kNameKey,
                        TextHelper::toHtmlEscaped(node.name), kDescriptionKey,
                        TextHelper::toHtmlEscaped(node.description), kPositionKey,
                        QString::number(node.position.x()), QString::number(node.position.y()),
                        (node.color.isValid()
                             ? QString("%1=\"%2\"").arg(kColorKey, node.color.name())
                             : QString()))
                   .toUtf8();
        for (const auto& connection : std::as_const(node.connections)) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\" %8=\"%9\" %10=\"%11\" %12/>\n")
                       .arg(kNodeConnectionKey, kFromUuidKey, connection.fromNodeUuid.toString(),
                            kToUuidKey, connection.toNodeUuid.toString(), kNameKey,
                            TextHelper::toHtmlEscaped(connection.name), kDescriptionKey,
                            TextHelper::toHtmlEscaped(connection.description), kLineTypeKey,
                            QString::number(connection.lineType),
                            (connection.color.isValid()
                                 ? QString("%1=\"%2\"").arg(kColorKey, connection.color.name())
                                 : QString()))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(kNodeKey).toUtf8();
    }
    for (const auto& group : std::as_const(d->nodeGroups)) {
        xml += QString("<%1 %2=\"%3\" %4=\"%5\" %6=\"%7\" %8=\"%9\" %10=\"%11\" %12/>\n")
                   .arg(kNodeGroupKey, kUuidKey, group.uuid.toString(), kNameKey,
                        TextHelper::toHtmlEscaped(group.name), kDescriptionKey,
                        TextHelper::toHtmlEscaped(group.description), kRectKey,
                        toString(group.rect), kLineTypeKey, QString::number(group.lineType),
                        (group.color.isValid()
                             ? QString("%1=\"%2\"").arg(kColorKey, group.color.name())
                             : QString()))
                   .toUtf8();
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor MindMapModel::applyPatch(const QByteArray& _patch)
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
    auto load = [&documentNode](const QString& _key) {
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
    };
    setName(load(kNameKey));
    setDescription(load(kDescriptionKey));
    //    //
    //    // Считываем фотографии
    //    //
    //    auto photosNode = documentNode.firstChildElement(kPhotosKey);
    //    QVector<QUuid> newPhotosUuids;
    //    if (!photosNode.isNull()) {
    //        auto photoNode = photosNode.firstChildElement(kPhotoKey);
    //        while (!photoNode.isNull()) {
    //            const auto uuid =
    //            QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
    //            newPhotosUuids.append(uuid);

    //            photoNode = photoNode.nextSiblingElement();
    //        }
    //    }
    //    //
    //    // ... корректируем текущие фотографии персонажа
    //    //
    //    for (int photoIndex = 0; photoIndex < d->photos.size(); ++photoIndex) {
    //        const auto& photo = d->photos.at(photoIndex);
    //        //
    //        // ... если такое отношение осталось актуальным, то оставим его в списке текущих
    //        //     и удалим из списка новых
    //        //
    //        if (newPhotosUuids.contains(photo.uuid)) {
    //            newPhotosUuids.removeAll(photo.uuid);
    //        }
    //        //
    //        // ... если такого отношения нет в списке новых, то удалим его из списка текущих
    //        //
    //        else {
    //            removePhoto(photo.uuid);
    //            --photoIndex;
    //        }
    //    }
    //    //
    //    // ... добавляем новые фотографии к персонажу
    //    //
    //    for (const auto& photoUuid : newPhotosUuids) {
    //        addPhoto({ photoUuid });
    //        imageWrapper()->load(photoUuid);
    //    }

    return {};
}

} // namespace BusinessLayer

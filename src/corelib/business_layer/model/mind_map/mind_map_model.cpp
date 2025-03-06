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

        //
        // Сохраним значение, т.к. после удаления ноды оно станет невалидным
        //
        const auto nodeUuid = _nodeUuid;
        d->nodes.removeAt(index);
        emit nodeRemoved(nodeUuid);
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

    //
    // Сохраним значения, т.к. после удаления соединения они станут невалидными
    //
    const auto fromNodeUuid = _fromNodeUuid;
    const auto toNodeUuid = _toNodeUuid;

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
    emit nodeConnectionRemoved(fromNodeUuid, toNodeUuid);
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

        //
        // Сохраним значение, т.к. после удаления группы оно станет невалидным
        //
        const auto groupUuid = _groupUuid;
        d->nodeGroups.removeAt(index);
        emit nodeGroupRemoved(groupUuid);
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

    //
    // Запомним порядок элементов в документе
    //
    QHash<QUuid, int> elementsPositions;
    QHash<QUuid, QHash<QPair<QUuid, QUuid>, int>> nodeConnectionsPositions;
    auto element = documentNode.firstChildElement();
    int elementPosition = 0;
    while (!element.isNull()) {
        if (element.nodeName() == kNodeKey || element.nodeName() == kNodeGroupKey) {
            QUuid uuid = QUuid::fromString(element.attribute(kUuidKey));
            elementsPositions.insert(uuid, elementPosition);
            ++elementPosition;

            //
            // ... для ячеек запомним порядок связей
            //
            if (element.nodeName() == kNodeKey) {
                QHash<QPair<QUuid, QUuid>, int> connectionsPositions;
                int connectionPosition = 0;
                QDomElement connectionNode = element.firstChildElement(kNodeConnectionKey);
                while (!connectionNode.isNull()) {
                    auto fromUuid = QUuid::fromString(connectionNode.attribute(kFromUuidKey));
                    auto toUuid = QUuid::fromString(connectionNode.attribute(kToUuidKey));
                    connectionsPositions.insert({ fromUuid, toUuid }, connectionPosition);
                    ++connectionPosition;
                    connectionNode = connectionNode.nextSiblingElement(kNodeConnectionKey);
                }
                nodeConnectionsPositions.insert(uuid, connectionsPositions);
            }
        }
        element = element.nextSiblingElement();
    }

    //
    // Ячейки
    //
    auto nodeNode = documentNode.firstChildElement(kNodeKey);
    QVector<MindMapNode> newNodes;
    while (!nodeNode.isNull() && nodeNode.nodeName() == kNodeKey) {
        MindMapNode node;
        node.uuid = QUuid::fromString(nodeNode.attribute(kUuidKey));
        node.name = TextHelper::fromHtmlEscaped(nodeNode.attribute(kNameKey));
        node.description = TextHelper::fromHtmlEscaped(nodeNode.attribute(kDescriptionKey));
        const auto positionText = nodeNode.attribute(kPositionKey).split(";");
        Q_ASSERT(positionText.size() == 2);
        const QPointF position(positionText.constFirst().toDouble(),
                               positionText.constLast().toDouble());
        node.position = position;
        if (nodeNode.hasAttribute(kColorKey)) {
            node.color = nodeNode.attribute(kColorKey);
        }

        //
        // ... соединения ячейки
        //
        QDomElement connectionNode = nodeNode.firstChildElement(kNodeConnectionKey);
        while (!connectionNode.isNull()) {
            MindMapNodeConnection connection;
            connection.fromNodeUuid = QUuid::fromString(connectionNode.attribute(kFromUuidKey));
            connection.toNodeUuid = QUuid::fromString(connectionNode.attribute(kToUuidKey));
            connection.name = TextHelper::fromHtmlEscaped(connectionNode.attribute(kNameKey));
            connection.description
                = TextHelper::fromHtmlEscaped(connectionNode.attribute(kDescriptionKey));
            connection.lineType = connectionNode.attribute(kLineTypeKey).toInt();
            if (connectionNode.hasAttribute(kColorKey)) {
                connection.color = connectionNode.attribute(kColorKey).toInt();
            }
            node.connections.append(connection);
            connectionNode = connectionNode.nextSiblingElement(kNodeConnectionKey);
        }

        newNodes.append(node);
        nodeNode = nodeNode.nextSiblingElement();
    }

    //
    // Корректируем текущие ячейки
    //
    for (int nodeIndex = d->nodes.size() - 1; nodeIndex >= 0; --nodeIndex) {
        const auto& node = d->nodes.at(nodeIndex);
        //
        // ... если такая ячейка осталась актуальной, то оставим её в списке текущих
        //     и удалим из списка новых
        //
        if (int nodeIndex = newNodes.indexOf(node); nodeIndex != -1) {
            //
            // ... при этом обновим список связей если необходимо
            //
            auto newConnections = newNodes[nodeIndex].connections;
            for (int connectionIndex = 0; connectionIndex < node.connections.size();
                 ++connectionIndex) {
                const auto& connection = node.connections[connectionIndex];
                //
                // ... если такое соединение есть, то оставим его в списке текущих
                //     и удалим из списка новых
                //
                if (newConnections.contains(connection)) {
                    newConnections.removeAll(connection);
                }
                //
                // ... если такого соединения нет в списке новых, то удалим его из списка текущих
                //
                else {
                    removeNodeConnection(connection.fromNodeUuid, connection.toNodeUuid);
                    --connectionIndex;
                }
            }
            //
            // ... и добавим новые связи
            //
            for (const auto& connection : std::as_const(newConnections)) {
                addNodeConnection(connection);
            }

            newNodes.removeAll(node);
        }
        //
        // ... если такой ячейки нет в списке новых, то удалим её из списка текущих со всеми
        //     соединениями
        //
        else {
            for (int i = node.connections.size() - 1; i >= 0; --i) {
                const auto& connection = node.connections[i];
                removeNodeConnection(connection.fromNodeUuid, connection.toNodeUuid);
            }
            removeNode(node.uuid);
            --nodeIndex;
        }
    }
    //
    // Добавляем новые ячейки и отдельно - их соединения
    //
    for (auto& node : newNodes) {
        const auto newConnections = node.connections;
        node.connections.clear();
        addNode(node);
        for (const auto& connection : std::as_const(newConnections)) {
            addNodeConnection(connection);
        }
    }

    //
    // Группы ячеек
    //
    auto nodeGroupNode = documentNode.firstChildElement(kNodeGroupKey);
    QVector<MindMapNodeGroup> newNodeGroups;
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
        newNodeGroups.append(group);

        nodeGroupNode = nodeGroupNode.nextSiblingElement();
    }
    //
    // Корректируем текущие группы
    //
    for (int groupIndex = 0; groupIndex < d->nodeGroups.size(); ++groupIndex) {
        const auto& group = d->nodeGroups.at(groupIndex);
        //
        // ... если такая группа осталось актуальной, то оставим её в списке текущих
        //     и удалим из списка новых
        //
        if (newNodeGroups.contains(group)) {
            updateNodeGroup(group);
            newNodeGroups.removeAll(group);
        }
        //
        // ... если такой группы нет в списке новых, то удалим её из списка текущих
        //
        else {
            removeNodeGroup(group.uuid);
            --groupIndex;
        }
    }
    //
    // Добавляем новые группы
    //
    for (const auto& group : std::as_const(newNodeGroups)) {
        addNodeGroup(group);
    }

    //
    // Сортируем ячейки
    //
    std::sort(d->nodes.begin(), d->nodes.end(),
              [elementsPositions](const MindMapNode& _lhs, const MindMapNode& _rhs) {
                  return elementsPositions[_lhs.uuid] < elementsPositions[_rhs.uuid];
              });

    //
    // Сортируем группы
    //
    std::sort(d->nodeGroups.begin(), d->nodeGroups.end(),
              [elementsPositions](const MindMapNodeGroup& _lhs, const MindMapNodeGroup& _rhs) {
                  return elementsPositions[_lhs.uuid] < elementsPositions[_rhs.uuid];
              });

    //
    // Сортируем соединения ячеек
    //
    for (auto& node : d->nodes) {
        std::sort(node.connections.begin(), node.connections.end(),
                  [node, &nodeConnectionsPositions](const MindMapNodeConnection& _lhs,
                                                    const MindMapNodeConnection& _rhs) {
                      const auto& connectionsPositions = nodeConnectionsPositions[node.uuid];
                      return connectionsPositions[{ _lhs.fromNodeUuid, _lhs.toNodeUuid }]
                          < connectionsPositions[{ _rhs.fromNodeUuid, _rhs.toNodeUuid }];
                  });
    }

    return {};
}

} // namespace BusinessLayer

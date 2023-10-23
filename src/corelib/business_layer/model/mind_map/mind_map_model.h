#pragma once

#include "../abstract_model.h"

#include <QColor>
#include <QRectF>
#include <QUuid>

namespace Domain {
struct DocumentImage;
}


namespace BusinessLayer {

/**
 * @brief Соединение с другой ячейкой
 */
class CORE_LIBRARY_EXPORT MindMapNodeConnection
{
public:
    bool isValid() const;

    bool operator==(const MindMapNodeConnection& _other) const;
    bool operator!=(const MindMapNodeConnection& _other) const;

    QUuid fromNodeUuid;
    QUuid toNodeUuid;
    int lineType = Qt::SolidLine;
    QColor color = {};
    QString name = {};
    QString description = {};
};

/**
 * @brief Ячейка ментальной карты
 */
class CORE_LIBRARY_EXPORT MindMapNode
{
public:
    bool isValid() const;

    MindMapNodeConnection connectionTo(const MindMapNode& _node) const;

    bool operator==(const MindMapNode& _other) const;
    bool operator!=(const MindMapNode& _other) const;

    QUuid uuid;
    QString name = {};
    QString description = {};
    QPointF position = {};
    QColor color = {};

    /**
     * @brief Соединения исходящие из текущей ячейки
     */
    QVector<MindMapNodeConnection> connections = {};
};

/**
 * @brief Группа ячеек ментальной карты
 */
class CORE_LIBRARY_EXPORT MindMapNodeGroup
{
public:
    bool isValid() const;

    bool operator==(const MindMapNodeGroup& _other) const;
    bool operator!=(const MindMapNodeGroup& _other) const;

    QUuid uuid;
    QString name = {};
    QString description = {};
    QRectF rect = {};
    int lineType = Qt::SolidLine;
    QColor color = {};
};

/**
 * @brief Модель ментальной карты
 */
class CORE_LIBRARY_EXPORT MindMapModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit MindMapModel(QObject* _parent = nullptr);
    ~MindMapModel() override;

    QString name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;

    QString description() const;
    void setDescription(const QString& _description);
    Q_SIGNAL void descriptionChanged(const QString& _description);

    void addNode(const MindMapNode& _node);
    Q_SIGNAL void nodeAdded(const BusinessLayer::MindMapNode& _node);
    void updateNode(const MindMapNode& _node);
    Q_SIGNAL void nodeUpdated(const BusinessLayer::MindMapNode& _node);
    void removeNode(const QUuid& _nodeUuid);
    Q_SIGNAL void nodeRemoved(const QUuid& _nodeUuid);
    QVector<MindMapNode> nodes() const;

    void addNodeConnection(const MindMapNodeConnection& _nodeConnection);
    Q_SIGNAL void nodeConnectionAdded(const BusinessLayer::MindMapNodeConnection& _nodeConnection);
    void updateNodeConnection(const MindMapNodeConnection& _nodeConnection);
    Q_SIGNAL void nodeConnectionUpdated(
        const BusinessLayer::MindMapNodeConnection& _nodeConnection);
    void removeNodeConnection(const QUuid& _fromNodeUuid, const QUuid& _toNodeUuid);
    Q_SIGNAL void nodeConnectionRemoved(const QUuid& _fromNodeUuid, const QUuid& _toNodeUuid);

    void addNodeGroup(const BusinessLayer::MindMapNodeGroup& _group);
    Q_SIGNAL void nodeGroupAdded(const BusinessLayer::MindMapNodeGroup& _group);
    void updateNodeGroup(const MindMapNodeGroup& _group);
    Q_SIGNAL void nodeGroupUpdated(const BusinessLayer::MindMapNodeGroup& _group);
    void removeNodeGroup(const QUuid& _groupUuid);
    Q_SIGNAL void nodeGroupRemoved(const QUuid& _groupUuid);
    QVector<MindMapNodeGroup> nodeGroups() const;

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initImageWrapper() override;
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    ChangeCursor applyPatch(const QByteArray& _patch) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

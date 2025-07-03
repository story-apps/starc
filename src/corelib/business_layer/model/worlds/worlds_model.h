#pragma once

#include "../abstract_model.h"

#include <QColor>
#include <QRectF>
#include <QUuid>


namespace BusinessLayer {

class WorldModel;

/**
 * @brief Группа миров
 */
class CORE_LIBRARY_EXPORT WorldsGroup
{
public:
    bool isValid() const;

    bool operator==(const WorldsGroup& _other) const;
    bool operator!=(const WorldsGroup& _other) const;

    QUuid id;
    QString name = {};
    QString description = {};
    QRectF rect = {};
    int lineType = Qt::SolidLine;
    QColor color = {};
};

/**
 * @brief Модель списка миров
 */
class CORE_LIBRARY_EXPORT WorldsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit WorldsModel(QObject* _parent = nullptr);
    ~WorldsModel() override;

    /**
     * @brief Добавить модель мира
     */
    void addWorldModel(WorldModel* _worldModel);

    /**
     * @brief Удалить модель мира
     */
    void removeWorldModel(WorldModel* _worldModel);

    /**
     * @brief Создать мир с заданным именем
     */
    void createWorld(const QString& _name, const QByteArray& _content = {});

    /**
     * @brief Существует ли мир с заданным именем
     */
    bool exists(const QString& _name) const;

    /**
     * @brief Получить модель мира по её идентификатору
     */
    WorldModel* world(const QUuid& _uuid) const;

    /**
     * @brief Получить модель мира по её имени
     */
    WorldModel* world(const QString& _name) const;

    /**
     * @brief Получить модель локации по её индексу
     */
    WorldModel* world(int _row) const;

    /**
     * @brief Получить все модели миров с заданным именем
     */
    QVector<WorldModel*> worlds(const QString& _name) const;

    /**
     * @brief Группы миров
     */
    void createWorldsGroup(const QUuid& _groupId);
    void updateWorldsGroup(const WorldsGroup& _group);
    void removeWorldsGroup(const QUuid& _groupId);
    QVector<WorldsGroup> worldsGroups() const;
    Q_SIGNAL void worldsGroupAdded(const BusinessLayer::WorldsGroup& _group);
    Q_SIGNAL void worldsGroupChanged(const BusinessLayer::WorldsGroup& _group);
    Q_SIGNAL void worldsGroupRemoved(const BusinessLayer::WorldsGroup& _group);

    /**
     * @brief Позиция карточки мира на карте
     */
    QPointF worldPosition(const QString& _name, const QPointF& _defaultPosition = {}) const;
    void setWorldPosition(const QString& _name, const QPointF& _position);
    Q_SIGNAL void worldPositionChanged(const QString& _name, const QPointF& _position);

    /**
     * @brief Реализация древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount(const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& _index) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;
    /** @} */

signals:
    /**
     * @brief Неоходимо создать мир с заданным именем
     */
    void createWorldRequested(const QString& _name, const QByteArray& _content);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
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

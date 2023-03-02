#pragma once

#include "../abstract_model.h"

#include <QColor>
#include <QRectF>
#include <QUuid>


namespace BusinessLayer {

class LocationModel;

/**
 * @brief Группа локаций
 */
class CORE_LIBRARY_EXPORT LocationsGroup
{
public:
    bool isValid() const;

    bool operator==(const LocationsGroup& _other) const;
    bool operator!=(const LocationsGroup& _other) const;

    QUuid id;
    QString name = {};
    QString description = {};
    QRectF rect = {};
    int lineType = Qt::SolidLine;
    QColor color = {};
};

/**
 * @brief Модель списка локаций
 */
class CORE_LIBRARY_EXPORT LocationsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit LocationsModel(QObject* _parent = nullptr);
    ~LocationsModel() override;

    /**
     * @brief Добавить модель локации
     */
    void addLocationModel(LocationModel* _locationModel);

    /**
     * @brief Удалить модель локации
     */
    void removeLocationModel(LocationModel* _locationModel);

    /**
     * @brief Создать локацию с заданным именем
     */
    void createLocation(const QString& _name, const QByteArray& _content = {});

    /**
     * @brief Существует ли локация с заданным именем
     */
    bool exists(const QString& _name) const;

    /**
     * @brief Получить модель локации по её идентификатору
     */
    LocationModel* location(const QUuid& _uuid) const;

    /**
     * @brief Получить модель локации по её имени
     */
    LocationModel* location(const QString& _name) const;

    /**
     * @brief Получить модель локации по её индексу
     */
    LocationModel* location(int _row) const;

    /**
     * @brief Получить все модели локаций с заданным именем
     */
    QVector<LocationModel*> locations(const QString& _name) const;

    /**
     * @brief Группы локаций
     */
    void createLocationsGroup(const QUuid& _groupId);
    void updateLocationsGroup(const LocationsGroup& _group);
    void removeLocationsGroup(const QUuid& _groupId);
    QVector<LocationsGroup> locationsGroups() const;
    Q_SIGNAL void locationsGroupAdded(const BusinessLayer::LocationsGroup& _group);
    Q_SIGNAL void locationsGroupChanged(const BusinessLayer::LocationsGroup& _group);
    Q_SIGNAL void locationsGroupRemoved(const BusinessLayer::LocationsGroup& _group);

    /**
     * @brief Позиция карточки локации на карте
     */
    QPointF locationPosition(const QString& _name, const QPointF& _defaultPosition = {}) const;
    void setLocationPosition(const QString& _name, const QPointF& _position);
    Q_SIGNAL void locationPositionChanged(const QString& _name, const QPointF& _position);

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
     * @brief Неоходимо создать локацию с заданным именем
     */
    void createLocationRequested(const QString& _name, const QByteArray& _content);

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

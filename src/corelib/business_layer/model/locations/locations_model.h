#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

class LocationModel;

class LocationsModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit LocationsModel(QObject* _parent = nullptr);
    ~LocationsModel() override;

    /**
     * @brief Добавить модель персонажа
     */
    void addLocationModel(LocationModel* _LocationModel);

    /**
     * @brief Создать локацию с заданным именем
     */
    void createLocation(const QString& _name);

    /**
     * @brief Реализация древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount( const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex &_parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex &_index) const override;
    QVariant data(const QModelIndex &_index, int _role) const override;
    /** @} */

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

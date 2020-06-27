#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

class LocationModel;

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

signals:
    /**
     * @brief Неоходимо создать локацию с заданным именем
     */
    void createLocationRequested(const QString& _name);

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

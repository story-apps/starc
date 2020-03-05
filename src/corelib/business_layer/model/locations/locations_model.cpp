#include "locations_model.h"

#include "location_model.h"

#include <QDomDocument>


namespace BusinessLayer
{

class LocationsModel::Implementation
{
public:
    QVector<LocationModel*> locationModels;
};


// ****


LocationsModel::LocationsModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
}

void LocationsModel::addLocationModel(LocationModel* _LocationModel)
{
    if (d->locationModels.contains(_LocationModel)) {
        return;
    }

    const int itemRowIndex = rowCount();
    beginInsertRows({}, itemRowIndex, itemRowIndex);
    d->locationModels.append(_LocationModel);
    emit endInsertRows();
}

void LocationsModel::createLocation(const QString& _name)
{
    for (const auto location : d->locationModels) {
        if (location->name() == _name) {
            return;
        }
    }

    emit createLocationRequested(_name);
}

QModelIndex LocationsModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0
        || _row > rowCount(_parent)
        || _column < 0
        || _column > columnCount(_parent)
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
        case Qt::DisplayRole: {
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
    //
    // TODO:
    //
}

void LocationsModel::clearDocument()
{
    d->locationModels.clear();
}

QByteArray LocationsModel::toXml() const
{
    //
    // TODO: реализуем, когда сделаем редактор ментальных карт
    //
    return {};
}

} // namespace BusinessLayer

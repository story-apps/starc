#include "locations_model.h"

#include "location_model.h"

#include <QDomDocument>


namespace BusinessLayer {

class LocationsModel::Implementation
{
public:
    QVector<LocationModel*> locationModels;
};


// ****


LocationsModel::LocationsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

void LocationsModel::addLocationModel(LocationModel* _locationModel)
{
    if (_locationModel->name().isEmpty()) {
        return;
    }

    if (d->locationModels.contains(_locationModel)) {
        return;
    }

    const int itemRowIndex = rowCount();
    beginInsertRows({}, itemRowIndex, itemRowIndex);
    d->locationModels.append(_locationModel);
    endInsertRows();
}

void LocationsModel::removeLocationModel(LocationModel* _locationModel)
{
    if (_locationModel == nullptr) {
        return;
    }

    if (!d->locationModels.contains(_locationModel)) {
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

    for (const auto location : d->locationModels) {
        if (location->name() == _name) {
            return;
        }
    }

    emit createLocationRequested(_name, _content);
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

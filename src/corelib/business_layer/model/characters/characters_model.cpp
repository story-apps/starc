#include "characters_model.h"

#include "character_model.h"

#include <QDomDocument>


namespace BusinessLayer
{

class CharactersModel::Implementation
{
public:
    QVector<CharacterModel*> characterModels;
};


// ****


CharactersModel::CharactersModel(QObject* _parent)
    : AbstractModel({}, _parent),
      d(new Implementation)
{
}

void CharactersModel::addCharacterModel(CharacterModel* _characterModel)
{
    if (d->characterModels.contains(_characterModel)) {
        return;
    }

    const int itemRowIndex = rowCount();
    beginInsertRows({}, itemRowIndex, itemRowIndex);
    d->characterModels.append(_characterModel);
    emit endInsertRows();
}

void CharactersModel::createCharacter(const QString& _name)
{
    //
    // FIXME:
    //
}

QModelIndex CharactersModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0
        || _row > rowCount(_parent)
        || _column < 0
        || _column > columnCount(_parent)
        || (_parent.isValid() && (_parent.column() != 0))) {
        return {};
    }

    return createIndex(_row, _column, d->characterModels.at(_row));
}

QModelIndex CharactersModel::parent(const QModelIndex& _child) const
{
    Q_UNUSED(_child);
    return {};
}

int CharactersModel::columnCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return 1;
}

int CharactersModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return d->characterModels.size();
}

Qt::ItemFlags CharactersModel::flags(const QModelIndex& _index) const
{
    Q_UNUSED(_index)
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant CharactersModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    if (_index.row() >= d->characterModels.size()) {
        return {};
    }

    switch (_role) {
        case Qt::DisplayRole: {
            return d->characterModels.at(_index.row())->name();
        }

        default: {
            return {};
        }
    }
}

CharactersModel::~CharactersModel() = default;

void CharactersModel::initDocument()
{
    //
    // TODO:
    //
}

void CharactersModel::clearDocument()
{
    d->characterModels.clear();
}

QByteArray CharactersModel::toXml() const
{
    //
    // TODO: реализуем, когда сделаем редактор ментальных карт
    //
    return {};
}

} // namespace BusinessLayer

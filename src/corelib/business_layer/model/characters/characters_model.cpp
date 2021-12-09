#include "characters_model.h"

#include "character_model.h"

#include <domain/document_object.h>

#include <QDomDocument>
#include <QPointF>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kCharacterKey("character");
const QLatin1String kNameKey("name");
const QLatin1String kPositionKey("position");
} // namespace

class CharactersModel::Implementation
{
public:
    QVector<CharacterModel*> characterModels;
    QHash<QString, QPointF> charactersPositions;
};


// ****


CharactersModel::CharactersModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
    connect(this, &CharactersModel::characterPositionChanged, this,
            &CharactersModel::updateDocumentContent);
}

void CharactersModel::addCharacterModel(CharacterModel* _characterModel)
{
    if (_characterModel->name().isEmpty()) {
        return;
    }

    if (d->characterModels.contains(_characterModel)) {
        return;
    }

    const int itemRowIndex = rowCount();
    beginInsertRows({}, itemRowIndex, itemRowIndex);
    d->characterModels.append(_characterModel);
    endInsertRows();
}

void CharactersModel::removeCharacterModel(CharacterModel* _characterModel)
{
    if (_characterModel == nullptr) {
        return;
    }

    if (!d->characterModels.contains(_characterModel)) {
        return;
    }

    const int itemRowIndex = d->characterModels.indexOf(_characterModel);
    beginRemoveRows({}, itemRowIndex, itemRowIndex);
    d->characterModels.remove(itemRowIndex);
    endRemoveRows();
}

void CharactersModel::createCharacter(const QString& _name, const QByteArray& _content)
{
    if (_name.simplified().isEmpty()) {
        return;
    }

    for (const auto character : std::as_const(d->characterModels)) {
        if (character->name() == _name) {
            return;
        }
    }

    emit createCharacterRequested(_name, _content);
}

bool CharactersModel::exists(const QString& _name) const
{
    for (const auto character : std::as_const(d->characterModels)) {
        if (character->name() == _name) {
            return true;
        }
    }

    return false;
}

CharacterModel* CharactersModel::character(const QUuid& _uuid) const
{
    for (const auto character : std::as_const(d->characterModels)) {
        if (character->document()->uuid() == _uuid) {
            return character;
        }
    }

    return nullptr;
}

CharacterModel* CharactersModel::character(const QString& _name) const
{
    for (const auto character : std::as_const(d->characterModels)) {
        if (character->name() == _name) {
            return character;
        }
    }

    return nullptr;
}

QPointF CharactersModel::characterPosition(const QString& _name) const
{
    return d->charactersPositions.value(_name);
}

void CharactersModel::setCharacterPosition(const QString& _name, const QPointF& _position)
{
    if (d->charactersPositions.value(_name) == _position) {
        return;
    }

    d->charactersPositions[_name] = _position;
    emit characterPositionChanged(_name, _position);
}

QModelIndex CharactersModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_row < 0 || _row > rowCount(_parent) || _column < 0 || _column > columnCount(_parent)
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
    case Qt::DisplayRole:
    case Qt::EditRole: {
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
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto characterNode = documentNode.firstChildElement(kCharacterKey);
    while (!characterNode.isNull()) {
        const auto characterName = characterNode.attribute(kNameKey);
        const auto positionText = characterNode.attribute(kPositionKey).split(";");
        Q_ASSERT(positionText.size() == 2);
        const QPointF position(positionText.constFirst().toDouble(),
                               positionText.constLast().toDouble());
        d->charactersPositions[characterName] = position;

        characterNode = characterNode.nextSiblingElement();
    }
}

void CharactersModel::clearDocument()
{
    d->characterModels.clear();
    d->charactersPositions.clear();
}

QByteArray CharactersModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    for (auto character : std::as_const(d->characterModels)) {
        const auto characterPosition = this->characterPosition(character->name());
        xml += QString("<%1 %2=\"%3\" %4=\"%5;%6\"/>\n")
                   .arg(kCharacterKey, kNameKey, character->name(), kPositionKey,
                        QString::number(characterPosition.x()),
                        QString::number(characterPosition.y()))
                   .toUtf8();
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer

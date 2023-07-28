#include "script_text_model.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/locations/locations_model.h>

#include <QStringListModel>


namespace BusinessLayer {

class ScriptTextModel::Implementation
{
public:
    explicit Implementation(ScriptTextModel* _q);

    /**
     * @brief Получить корневой элемент
     */
    TextModelItem* rootItem() const;


    /**
     * @brief Родительский элемент
     */
    ScriptTextModel* q = nullptr;

    /**
     * @brief Модель персонажей
     */
    CharactersModel* charactersModel = nullptr;

    /**
     * @brief Модель локаций
     */
    LocationsModel* locationsModel = nullptr;

    /**
     * @brief Нужно ли обновить справочники, которые строятся в рантайме
     */
    bool needUpdateRuntimeDictionaries = false;

    /**
     * @brief Справочники, которые строятся в рантайме
     */
    QScopedPointer<QStringListModel> charactersModelFromText;
    QScopedPointer<QStringListModel> locationsModelFromText;
};

ScriptTextModel::Implementation::Implementation(ScriptTextModel* _q)
    : q(_q)
{
}

TextModelItem* ScriptTextModel::Implementation::rootItem() const
{
    return q->itemForIndex({});
}


// ****


ScriptTextModel::ScriptTextModel(QObject* _parent, TextModelFolderItem* _rootItem)
    : TextModel(_parent, _rootItem)
    , d(new Implementation(this))
{
}

ScriptTextModel::~ScriptTextModel() = default;

void ScriptTextModel::setCharactersModel(CharactersModel* _model)
{
    if (d->charactersModel) {
        d->charactersModel->disconnect(this);
    }

    d->charactersModel = _model;
    d->needUpdateRuntimeDictionaries = true;

    connect(d->charactersModel, &CharactersModel::contentsChanged, this,
            [this] { d->needUpdateRuntimeDictionaries = true; });
}

CharactersModel* ScriptTextModel::charactersModel() const
{
    return d->charactersModel;
}

QAbstractItemModel* ScriptTextModel::charactersList() const
{
    if (d->charactersModelFromText != nullptr) {
        return d->charactersModelFromText.data();
    }

    return d->charactersModel;
}

BusinessLayer::CharacterModel* ScriptTextModel::character(const QString& _name) const
{
    return d->charactersModel->character(_name);
}

void ScriptTextModel::createCharacter(const QString& _name)
{
    d->charactersModel->createCharacter(_name);
}

void ScriptTextModel::setLocationsModel(LocationsModel* _model)
{
    if (d->locationsModel) {
        d->locationsModel->disconnect(this);
    }

    d->locationsModel = _model;
    d->needUpdateRuntimeDictionaries = true;

    connect(d->locationsModel, &LocationsModel::contentsChanged, this,
            [this] { d->needUpdateRuntimeDictionaries = true; });
}

LocationsModel* ScriptTextModel::locationsModel() const
{
    return d->locationsModel;
}

QAbstractItemModel* ScriptTextModel::locationsList() const
{
    if (d->locationsModelFromText != nullptr) {
        return d->locationsModelFromText.data();
    }

    return d->locationsModel;
}

LocationModel* ScriptTextModel::location(const QString& _name) const
{
    return d->locationsModel->location(_name);
}

void ScriptTextModel::createLocation(const QString& _name)
{
    d->locationsModel->createLocation(_name);
}

void ScriptTextModel::updateRuntimeDictionariesIfNeeded()
{
    if (!d->needUpdateRuntimeDictionaries) {
        return;
    }

    updateRuntimeDictionaries();

    d->needUpdateRuntimeDictionaries = false;
}

void ScriptTextModel::markNeedUpdateRuntimeDictionaries()
{
    d->needUpdateRuntimeDictionaries = true;
}

QStringListModel* ScriptTextModel::charactersModelFromText() const
{
    if (d->charactersModelFromText == nullptr) {
        d->charactersModelFromText.reset(new QStringListModel);
    }
    return d->charactersModelFromText.data();
}

QStringListModel* ScriptTextModel::locationsModelFromText() const
{
    if (d->locationsModelFromText == nullptr) {
        d->locationsModelFromText.reset(new QStringListModel);
    }
    return d->locationsModelFromText.data();
}

} // namespace BusinessLayer

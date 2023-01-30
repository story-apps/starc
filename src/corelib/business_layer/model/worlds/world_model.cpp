#include "world_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kColorKey("color");
const QLatin1String kOneSentenceDescriptionKey("one_sentence_description");
const QLatin1String kLongDescriptionKey("long_description");
const QLatin1String kMainPhotoKey("main_photo");
const QLatin1String kPhotosKey("photos");
const QLatin1String kPhotoKey("photo");
const QLatin1String kRoutesKey("routes");
const QLatin1String kRouteKey("route");
const QLatin1String kRouteToWorldKey("to");
const QLatin1String kLineTypeKey("line_type");
const QLatin1String kDetailsKey("details");

const QLatin1String kOverviewKey("overview");
const QLatin1String kEarthLikeKey("earth_like");
const QLatin1String kHistoryKey("history");
const QLatin1String kMoodKey("mood");
//
const QLatin1String kBiologyKey("biology");
const QLatin1String kPhysicsKey("physics");
const QLatin1String kAstoronomyKey("astronomy");
const QLatin1String kGeographyKey("geography");
const QLatin1String kRacesKey("races");
const QLatin1String kRaceKey("race");
const QLatin1String kFlorasKey("floras");
const QLatin1String kFloraKey("flora");
const QLatin1String kAnimalsKey("animals");
const QLatin1String kAnimalKey("animal");
const QLatin1String kNaturalResourcesKey("naturalResources");
const QLatin1String kNaturalResourceKey("naturalResource");
const QLatin1String kClimatesKey("climates");
const QLatin1String kClimateKey("climate");
//
const QLatin1String kReligionsKey("religons");
const QLatin1String kReligionKey("religon");
const QLatin1String kEthicsKey("ethics");
const QLatin1String kEthicKey("ethic");
const QLatin1String kLanguagesKey("languages");
const QLatin1String kLanguageKey("language");
const QLatin1String kCastesKey("castes");
const QLatin1String kCasteKey("caste");
//
const QLatin1String kTechnologyKey("technology");
const QLatin1String kEconomyKey("economy");
const QLatin1String kTradeKey("trade");
const QLatin1String kBusinessKey("business");
const QLatin1String kIndustryKey("industry");
const QLatin1String kCurrencyKey("currency");
const QLatin1String kEducationKey("education");
const QLatin1String kCommunicationKey("communication");
const QLatin1String kArtKey("art");
const QLatin1String kEntertainmentKey("entertainment");
const QLatin1String kTravelKey("travel");
const QLatin1String kScienceKey("science");
//
const QLatin1String kGovernmentFormatKey("government_format");
const QLatin1String kGovernmentHistoryKey("government_history");
const QLatin1String kLawsKey("laws");
const QLatin1String kForeignRelationsKey("foreign_rRelations");
const QLatin1String kPerceptionOfGovernmentKey("perception_of_government");
const QLatin1String kPropagandaKey("propaganda");
const QLatin1String kAntiGovernmentOrganisationsKey("anti_government_organisations");
const QLatin1String kPastWarKey("past_war");
const QLatin1String kCurrentWarKey("current_war");
const QLatin1String kPotentialWarKey("potential_war");
//
const QLatin1String kMagicRuleKey("magic_rule");
const QLatin1String kWhoCanUseKey("who_can_use");
const QLatin1String kEffectToWorldKey("effect_to_world");
const QLatin1String kEffectToSocietyKey("effect_to_society");
const QLatin1String kEffectToTechnologyKey("effect_to_technology");
const QLatin1String kMagicTypesKey("magic_types");
const QLatin1String kMagicTypeKey("magic_type");
} // namespace

class WorldModel::Implementation
{
public:
    QString name;
    QString oneSentenceDescription;
    QString longDescription;
    QVector<Domain::DocumentImage> photos;
    QVector<WorldRoute> routes;

    QString overview;
    QString earthLike;
    QString history;
    QString mood;

    QString biology;
    QString physics;
    QString astronomy;
    QString geography;
    QVector<WorldItem> races;
    QVector<WorldItem> floras;
    QVector<WorldItem> animals;
    QVector<WorldItem> naturalResources;
    QVector<WorldItem> climates;

    QVector<WorldItem> religions;
    QVector<WorldItem> ethics;
    QVector<WorldItem> languages;
    QVector<WorldItem> castes;

    QString technology;
    QString economy;
    QString trade;
    QString business;
    QString industry;
    QString currency;
    QString education;
    QString communication;
    QString art;
    QString entertainment;
    QString travel;
    QString science;

    QString governmentFormat;
    QString governmentHistory;
    QString laws;
    QString foreignRelations;
    QString perceptionOfGovernment;
    QString propaganda;
    QString antiGovernmentOrganisations;
    QString pastWar;
    QString currentWar;
    QString potentialWar;

    QString magicRule;
    QString whoCanUse;
    QString effectToWorld;
    QString effectToSociety;
    QString effectToTechnology;
    QVector<WorldItem> magicTypes;
};


// ****


bool WorldRoute::isValid() const
{
    return !world.isNull();
}

bool WorldRoute::operator==(const WorldRoute& _other) const
{
    return world == _other.world && lineType == _other.lineType && color == _other.color
        && name == _other.name && details == _other.details;
}

bool WorldRoute::operator!=(const WorldRoute& _other) const
{
    return !(*this == _other);
}


// ****


WorldModel::WorldModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kNameKey,
            kOneSentenceDescriptionKey,
            kOverviewKey,
            kMainPhotoKey,
            kRoutesKey,
            kRouteKey,
        },
        _parent)
    , d(new Implementation)
{
    //
    // Как минимум один элемент должен всегда быть тут
    //
    d->races.append(WorldItem());
    d->floras.append(WorldItem());
    d->animals.append(WorldItem());
    d->naturalResources.append(WorldItem());
    d->climates.append(WorldItem());
    d->religions.append(WorldItem());
    d->ethics.append(WorldItem());
    d->languages.append(WorldItem());
    d->castes.append(WorldItem());
    d->magicTypes.append(WorldItem());

    connect(this, &WorldModel::nameChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::oneSentenceDescriptionChanged, this,
            &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::longDescriptionChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::mainPhotoChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::photosChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::routeAdded, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::routeChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::routeRemoved, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::overviewChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::earthLikeChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::historyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::moodChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::biologyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::physicsChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::astronomyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::geographyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::racesChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::florasChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::animalsChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::naturalResourcesChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::climatesChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::religionsChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::ethicsChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::languagesChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::castesChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::technologyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::economyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::tradeChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::businessChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::industryChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::currencyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::educationChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::communicationChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::artChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::entertainmentChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::travelChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::scienceChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::governmentFormatChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::governmentHistoryChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::lawsChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::foreignRelationsChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::perceptionOfGovernmentChanged, this,
            &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::propagandaChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::antiGovernmentOrganisationsChanged, this,
            &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::pastWarChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::currentWarChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::potentialWarChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::magicRuleChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::whoCanUseChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::effectToWorldChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::effectToSocietyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::effectToTechnologyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::magicTypesChanged, this, &WorldModel::updateDocumentContent);
}

WorldModel::~WorldModel() = default;

const QString& WorldModel::name() const
{
    return d->name;
}

void WorldModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    const auto oldName = d->name;
    d->name = _name;

    emit nameChanged(d->name, oldName);
    emit documentNameChanged(d->name);
}

QString WorldModel::documentName() const
{
    return name();
}

void WorldModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

QString WorldModel::oneSentenceDescription() const
{
    return d->oneSentenceDescription;
}

void WorldModel::setOneSentenceDescription(const QString& _text)
{
    if (d->oneSentenceDescription == _text) {
        return;
    }

    d->oneSentenceDescription = _text;
    emit oneSentenceDescriptionChanged(d->oneSentenceDescription);
}

QString WorldModel::longDescription() const
{
    return d->longDescription;
}

void WorldModel::setLongDescription(const QString& _text)
{
    if (d->longDescription == _text) {
        return;
    }

    d->longDescription = _text;
    emit longDescriptionChanged(d->longDescription);
}

Domain::DocumentImage WorldModel::mainPhoto() const
{
    if (d->photos.isEmpty()) {
        return {};
    }

    return d->photos.constFirst();
}

void WorldModel::setMainPhoto(const QPixmap& _photo)
{
    //
    // Если прилетела пустая картинка
    //
    if (_photo.isNull()) {
        if (d->photos.isEmpty()) {
            return;
        }

        imageWrapper()->remove(d->photos.constFirst().uuid);
        d->photos.removeFirst();
    }
    //
    // А если картинка не пустая
    //
    else {
        //
        // ... если фоток ещё не было, то создаём первую и сохраняем её в списке
        //
        if (d->photos.isEmpty()) {
            d->photos.append({ imageWrapper()->save(_photo), _photo });
        }
        //
        // ... а если уже были, то проверяем, действительно ли она изменилась
        //
        else {
            auto& mainPhoto = d->photos.first();
            if (mainPhoto.image.cacheKey() == _photo.cacheKey()) {
                return;
            }

            //
            // ... если изменилась, то удаляем старую и сохраняем новую
            //
            imageWrapper()->remove(mainPhoto.uuid);
            mainPhoto.uuid = imageWrapper()->save(_photo);
            mainPhoto.image = _photo;
        }
    }

    emit mainPhotoChanged(d->photos.isEmpty() ? Domain::DocumentImage() : d->photos.constFirst());
    emit photosChanged(d->photos);
}

QVector<Domain::DocumentImage> WorldModel::photos() const
{
    return d->photos;
}

void WorldModel::addPhoto(const Domain::DocumentImage& _photo)
{
    if (_photo.uuid.isNull() && _photo.image.isNull()) {
        return;
    }

    const bool isMainPhotoAdded = d->photos.isEmpty();
    d->photos.append(_photo);
    if (isMainPhotoAdded) {
        emit mainPhotoChanged(d->photos.constFirst());
    }
    emit photosChanged(d->photos);
}

void WorldModel::addPhotos(const QVector<QPixmap>& _photos)
{
    if (_photos.isEmpty()) {
        return;
    }

    const bool isMainPhotoAdded = d->photos.isEmpty();
    for (const auto& photo : _photos) {
        d->photos.append({ imageWrapper()->save(photo), photo });
    }
    if (isMainPhotoAdded) {
        emit mainPhotoChanged(d->photos.constFirst());
    }
    emit photosChanged(d->photos);
}

void WorldModel::removePhoto(const QUuid& _photoUuid)
{
    for (int index = 0; index < d->photos.size(); ++index) {
        if (d->photos.at(index).uuid != _photoUuid) {
            continue;
        }

        imageWrapper()->remove(_photoUuid);
        d->photos.removeAt(index);
        const bool isMainPhotoChanged = index == 0;
        if (isMainPhotoChanged) {
            emit mainPhotoChanged(d->photos.isEmpty() ? Domain::DocumentImage()
                                                      : d->photos.constFirst());
        }
        emit photosChanged(d->photos);

        break;
    }
}

void WorldModel::createRoute(const QUuid& _toWorld)
{
    for (const auto& relation : std::as_const(d->routes)) {
        if (relation.world == _toWorld) {
            return;
        }
    }

    d->routes.append({ _toWorld });
    emit routeAdded(d->routes.constLast());
}

void WorldModel::updateRoute(const WorldRoute& _way)
{
    for (auto& relation : d->routes) {
        if (relation.world != _way.world) {
            continue;
        }

        if (relation != _way) {
            relation = _way;
            emit routeChanged(relation);
        }
        return;
    }
}

void WorldModel::removeRoute(QUuid _toWorld)
{
    for (int index = 0; index < d->routes.size(); ++index) {
        if (d->routes[index].world != _toWorld) {
            continue;
        }

        auto relation = d->routes.takeAt(index);
        emit routeRemoved(relation);
        return;
    }
}

WorldRoute WorldModel::route(const QUuid& _toWorld)
{
    for (auto& relation : d->routes) {
        if (relation.world == _toWorld) {
            return relation;
        }
    }

    return {};
}

WorldRoute WorldModel::route(WorldModel* _toWorld)
{
    return route(_toWorld->document()->uuid());
}

QVector<WorldRoute> WorldModel::routes() const
{
    return d->routes;
}

QString WorldModel::overview() const
{
    return d->overview;
}

void WorldModel::setOverview(const QString& _text)
{
    if (d->overview == _text) {
        return;
    }

    d->overview = _text;
    emit overviewChanged(d->overview);
}

QString WorldModel::earthLike() const
{
    return d->earthLike;
}

void WorldModel::setEarthLike(const QString& _text)
{
    if (d->earthLike == _text) {
        return;
    }

    d->earthLike = _text;
    emit earthLikeChanged(d->earthLike);
}

QString WorldModel::history() const
{
    return d->history;
}

void WorldModel::setHistory(const QString& _text)
{
    if (d->history == _text) {
        return;
    }

    d->history = _text;
    emit historyChanged(d->history);
}

QString WorldModel::mood() const
{
    return d->mood;
}

void WorldModel::setMood(const QString& _text)
{
    if (d->mood == _text) {
        return;
    }

    d->mood = _text;
    emit moodChanged(d->mood);
}

QString WorldModel::biology() const
{
    return d->biology;
}

void WorldModel::setBiology(const QString& _text)
{
    if (d->biology == _text) {
        return;
    }
    d->biology = _text;
    emit biologyChanged(d->biology);
}

QString WorldModel::physics() const
{
    return d->physics;
}

void WorldModel::setPhysics(const QString& _text)
{
    if (d->physics == _text) {
        return;
    }
    d->physics = _text;
    emit physicsChanged(d->physics);
}

QString WorldModel::astronomy() const
{
    return d->astronomy;
}

void WorldModel::setAstronomy(const QString& _text)
{
    if (d->astronomy == _text) {
        return;
    }
    d->astronomy = _text;
    emit astronomyChanged(d->astronomy);
}

QString WorldModel::geography() const
{
    return d->geography;
}

void WorldModel::setGeography(const QString& _text)
{
    if (d->geography == _text) {
        return;
    }
    d->geography = _text;
    emit geographyChanged(d->geography);
}

QVector<WorldItem> WorldModel::races() const
{
    return d->races;
}

void WorldModel::setRaces(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->races.size() >= index);
        if (d->races.size() == index) {
            d->races.append(WorldItem());
        }

        auto& oldRace = d->races[index];
        auto& newRace = _items[index];
        if (!oldRace.photo.uuid.isNull() && oldRace.photo.uuid != newRace.photo.uuid) {
            imageWrapper()->remove(oldRace.photo.uuid);
            oldRace.photo = {};
        }
        if (newRace.photo.uuid.isNull() && !newRace.photo.image.isNull()) {
            oldRace.photo = { imageWrapper()->save(newRace.photo.image), newRace.photo.image };
        }

        oldRace.name = newRace.name;
        oldRace.oneSentenceDescription = newRace.oneSentenceDescription;
        oldRace.longDescription = newRace.longDescription;
    }

    for (int lastIndex = d->races.size() - 1; lastIndex >= index; --lastIndex) {
        auto& race = d->races[lastIndex];
        if (!race.photo.uuid.isNull()) {
            imageWrapper()->remove(race.photo.uuid);
        }
        d->races.removeLast();
    }

    emit racesChanged(d->races);
}

QVector<WorldItem> WorldModel::floras() const
{
    return d->floras;
}

void WorldModel::setFloras(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->floras.size() >= index);
        if (d->floras.size() == index) {
            d->floras.append(WorldItem());
        }

        auto& oldFlora = d->floras[index];
        auto& newFlora = _items[index];
        if (!oldFlora.photo.uuid.isNull() && oldFlora.photo.uuid != newFlora.photo.uuid) {
            imageWrapper()->remove(oldFlora.photo.uuid);
            oldFlora.photo = {};
        }
        if (newFlora.photo.uuid.isNull() && !newFlora.photo.image.isNull()) {
            oldFlora.photo = { imageWrapper()->save(newFlora.photo.image), newFlora.photo.image };
        }

        oldFlora.name = newFlora.name;
        oldFlora.oneSentenceDescription = newFlora.oneSentenceDescription;
        oldFlora.longDescription = newFlora.longDescription;
    }

    for (int lastIndex = d->floras.size() - 1; lastIndex >= index; --lastIndex) {
        auto& flora = d->floras[lastIndex];
        if (!flora.photo.uuid.isNull()) {
            imageWrapper()->remove(flora.photo.uuid);
        }
        d->floras.removeLast();
    }

    emit florasChanged(d->floras);
}

QVector<WorldItem> WorldModel::animals() const
{
    return d->animals;
}

void WorldModel::setAnimals(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->animals.size() >= index);
        if (d->animals.size() == index) {
            d->animals.append(WorldItem());
        }

        auto& oldAnimal = d->animals[index];
        auto& newAnimal = _items[index];
        if (!oldAnimal.photo.uuid.isNull() && oldAnimal.photo.uuid != newAnimal.photo.uuid) {
            imageWrapper()->remove(oldAnimal.photo.uuid);
            oldAnimal.photo = {};
        }
        if (newAnimal.photo.uuid.isNull() && !newAnimal.photo.image.isNull()) {
            oldAnimal.photo
                = { imageWrapper()->save(newAnimal.photo.image), newAnimal.photo.image };
        }

        oldAnimal.name = newAnimal.name;
        oldAnimal.oneSentenceDescription = newAnimal.oneSentenceDescription;
        oldAnimal.longDescription = newAnimal.longDescription;
    }

    for (int lastIndex = d->animals.size() - 1; lastIndex >= index; --lastIndex) {
        auto& animal = d->animals[lastIndex];
        if (!animal.photo.uuid.isNull()) {
            imageWrapper()->remove(animal.photo.uuid);
        }
        d->animals.removeLast();
    }

    emit animalsChanged(d->animals);
}

QVector<WorldItem> WorldModel::naturalResources() const
{
    return d->naturalResources;
}

void WorldModel::setNaturalResources(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->naturalResources.size() >= index);
        if (d->naturalResources.size() == index) {
            d->naturalResources.append(WorldItem());
        }

        auto& oldNaturalResource = d->naturalResources[index];
        auto& newNaturalResource = _items[index];
        if (!oldNaturalResource.photo.uuid.isNull()
            && oldNaturalResource.photo.uuid != newNaturalResource.photo.uuid) {
            imageWrapper()->remove(oldNaturalResource.photo.uuid);
            oldNaturalResource.photo = {};
        }
        if (newNaturalResource.photo.uuid.isNull() && !newNaturalResource.photo.image.isNull()) {
            oldNaturalResource.photo = { imageWrapper()->save(newNaturalResource.photo.image),
                                         newNaturalResource.photo.image };
        }

        oldNaturalResource.name = newNaturalResource.name;
        oldNaturalResource.oneSentenceDescription = newNaturalResource.oneSentenceDescription;
        oldNaturalResource.longDescription = newNaturalResource.longDescription;
    }

    for (int lastIndex = d->naturalResources.size() - 1; lastIndex >= index; --lastIndex) {
        auto& naturalResource = d->naturalResources[lastIndex];
        if (!naturalResource.photo.uuid.isNull()) {
            imageWrapper()->remove(naturalResource.photo.uuid);
        }
        d->naturalResources.removeLast();
    }

    emit naturalResourcesChanged(d->naturalResources);
}

QVector<WorldItem> WorldModel::climates() const
{
    return d->climates;
}

void WorldModel::setClimates(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->climates.size() >= index);
        if (d->climates.size() == index) {
            d->climates.append(WorldItem());
        }

        auto& oldClimate = d->climates[index];
        auto& newClimate = _items[index];
        if (!oldClimate.photo.uuid.isNull() && oldClimate.photo.uuid != newClimate.photo.uuid) {
            imageWrapper()->remove(oldClimate.photo.uuid);
            oldClimate.photo = {};
        }
        if (newClimate.photo.uuid.isNull() && !newClimate.photo.image.isNull()) {
            oldClimate.photo
                = { imageWrapper()->save(newClimate.photo.image), newClimate.photo.image };
        }

        oldClimate.name = newClimate.name;
        oldClimate.oneSentenceDescription = newClimate.oneSentenceDescription;
        oldClimate.longDescription = newClimate.longDescription;
    }

    for (int lastIndex = d->climates.size() - 1; lastIndex >= index; --lastIndex) {
        auto& climate = d->climates[lastIndex];
        if (!climate.photo.uuid.isNull()) {
            imageWrapper()->remove(climate.photo.uuid);
        }
        d->climates.removeLast();
    }

    emit climatesChanged(d->climates);
}

QVector<WorldItem> WorldModel::religions() const
{
    return d->religions;
}

void WorldModel::setReligions(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->religions.size() >= index);
        if (d->religions.size() == index) {
            d->religions.append(WorldItem());
        }

        auto& oldReligion = d->religions[index];
        auto& newReligion = _items[index];
        if (!oldReligion.photo.uuid.isNull() && oldReligion.photo.uuid != newReligion.photo.uuid) {
            imageWrapper()->remove(oldReligion.photo.uuid);
            oldReligion.photo = {};
        }
        if (newReligion.photo.uuid.isNull() && !newReligion.photo.image.isNull()) {
            oldReligion.photo
                = { imageWrapper()->save(newReligion.photo.image), newReligion.photo.image };
        }

        oldReligion.name = newReligion.name;
        oldReligion.oneSentenceDescription = newReligion.oneSentenceDescription;
        oldReligion.longDescription = newReligion.longDescription;
    }

    for (int lastIndex = d->religions.size() - 1; lastIndex >= index; --lastIndex) {
        auto& religion = d->religions[lastIndex];
        if (!religion.photo.uuid.isNull()) {
            imageWrapper()->remove(religion.photo.uuid);
        }
        d->religions.removeLast();
    }

    emit religionsChanged(d->religions);
}

QVector<WorldItem> WorldModel::ethics() const
{
    return d->ethics;
}

void WorldModel::setEthics(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->ethics.size() >= index);
        if (d->ethics.size() == index) {
            d->ethics.append(WorldItem());
        }

        auto& oldEthic = d->ethics[index];
        auto& newEthic = _items[index];
        if (!oldEthic.photo.uuid.isNull() && oldEthic.photo.uuid != newEthic.photo.uuid) {
            imageWrapper()->remove(oldEthic.photo.uuid);
            oldEthic.photo = {};
        }
        if (newEthic.photo.uuid.isNull() && !newEthic.photo.image.isNull()) {
            oldEthic.photo = { imageWrapper()->save(newEthic.photo.image), newEthic.photo.image };
        }

        oldEthic.name = newEthic.name;
        oldEthic.oneSentenceDescription = newEthic.oneSentenceDescription;
        oldEthic.longDescription = newEthic.longDescription;
    }

    for (int lastIndex = d->ethics.size() - 1; lastIndex >= index; --lastIndex) {
        auto& ethic = d->ethics[lastIndex];
        if (!ethic.photo.uuid.isNull()) {
            imageWrapper()->remove(ethic.photo.uuid);
        }
        d->ethics.removeLast();
    }

    emit ethicsChanged(d->ethics);
}

QVector<WorldItem> WorldModel::languages() const
{
    return d->languages;
}

void WorldModel::setLanguages(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->languages.size() >= index);
        if (d->languages.size() == index) {
            d->languages.append(WorldItem());
        }

        auto& oldLanguage = d->languages[index];
        auto& newLanguage = _items[index];
        if (!oldLanguage.photo.uuid.isNull() && oldLanguage.photo.uuid != newLanguage.photo.uuid) {
            imageWrapper()->remove(oldLanguage.photo.uuid);
            oldLanguage.photo = {};
        }
        if (newLanguage.photo.uuid.isNull() && !newLanguage.photo.image.isNull()) {
            oldLanguage.photo
                = { imageWrapper()->save(newLanguage.photo.image), newLanguage.photo.image };
        }

        oldLanguage.name = newLanguage.name;
        oldLanguage.oneSentenceDescription = newLanguage.oneSentenceDescription;
        oldLanguage.longDescription = newLanguage.longDescription;
    }

    for (int lastIndex = d->languages.size() - 1; lastIndex >= index; --lastIndex) {
        auto& language = d->languages[lastIndex];
        if (!language.photo.uuid.isNull()) {
            imageWrapper()->remove(language.photo.uuid);
        }
        d->languages.removeLast();
    }

    emit languagesChanged(d->languages);
}

QVector<WorldItem> WorldModel::castes() const
{
    return d->castes;
}

void WorldModel::setCastes(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->castes.size() >= index);
        if (d->castes.size() == index) {
            d->castes.append(WorldItem());
        }

        auto& oldCaste = d->castes[index];
        auto& newCaste = _items[index];
        if (!oldCaste.photo.uuid.isNull() && oldCaste.photo.uuid != newCaste.photo.uuid) {
            imageWrapper()->remove(oldCaste.photo.uuid);
            oldCaste.photo = {};
        }
        if (newCaste.photo.uuid.isNull() && !newCaste.photo.image.isNull()) {
            oldCaste.photo = { imageWrapper()->save(newCaste.photo.image), newCaste.photo.image };
        }

        oldCaste.name = newCaste.name;
        oldCaste.oneSentenceDescription = newCaste.oneSentenceDescription;
        oldCaste.longDescription = newCaste.longDescription;
    }

    for (int lastIndex = d->castes.size() - 1; lastIndex >= index; --lastIndex) {
        auto& caste = d->castes[lastIndex];
        if (!caste.photo.uuid.isNull()) {
            imageWrapper()->remove(caste.photo.uuid);
        }
        d->castes.removeLast();
    }

    emit castesChanged(d->castes);
}

QString WorldModel::technology() const
{
    return d->technology;
}

void WorldModel::setTechnology(const QString& _text)
{
    if (d->technology == _text) {
        return;
    }
    d->technology = _text;
    emit technologyChanged(d->technology);
}

QString WorldModel::economy() const
{
    return d->economy;
}

void WorldModel::setEconomy(const QString& _text)
{
    if (d->economy == _text) {
        return;
    }
    d->economy = _text;
    emit economyChanged(d->economy);
}

QString WorldModel::trade() const
{
    return d->trade;
}

void WorldModel::setTrade(const QString& _text)
{
    if (d->trade == _text) {
        return;
    }
    d->trade = _text;
    emit tradeChanged(d->trade);
}

QString WorldModel::business() const
{
    return d->business;
}

void WorldModel::setBusiness(const QString& _text)
{
    if (d->business == _text) {
        return;
    }
    d->business = _text;
    emit businessChanged(d->business);
}

QString WorldModel::industry() const
{
    return d->industry;
}

void WorldModel::setIndustry(const QString& _text)
{
    if (d->industry == _text) {
        return;
    }
    d->industry = _text;
    emit industryChanged(d->industry);
}

QString WorldModel::currency() const
{
    return d->currency;
}

void WorldModel::setCurrency(const QString& _text)
{
    if (d->currency == _text) {
        return;
    }
    d->currency = _text;
    emit currencyChanged(d->currency);
}

QString WorldModel::education() const
{
    return d->education;
}

void WorldModel::setEducation(const QString& _text)
{
    if (d->education == _text) {
        return;
    }
    d->education = _text;
    emit educationChanged(d->education);
}

QString WorldModel::communication() const
{
    return d->communication;
}

void WorldModel::setCommunication(const QString& _text)
{
    if (d->communication == _text) {
        return;
    }
    d->communication = _text;
    emit communicationChanged(d->communication);
}

QString WorldModel::art() const
{
    return d->art;
}

void WorldModel::setArt(const QString& _text)
{
    if (d->art == _text) {
        return;
    }
    d->art = _text;
    emit artChanged(d->art);
}

QString WorldModel::entertainment() const
{
    return d->entertainment;
}

void WorldModel::setEntertainment(const QString& _text)
{
    if (d->entertainment == _text) {
        return;
    }
    d->entertainment = _text;
    emit entertainmentChanged(d->entertainment);
}

QString WorldModel::travel() const
{
    return d->travel;
}

void WorldModel::setTravel(const QString& _text)
{
    if (d->travel == _text) {
        return;
    }
    d->travel = _text;
    emit travelChanged(d->travel);
}

QString WorldModel::science() const
{
    return d->science;
}

void WorldModel::setScience(const QString& _text)
{
    if (d->science == _text) {
        return;
    }
    d->science = _text;
    emit scienceChanged(d->science);
}

QString WorldModel::governmentFormat() const
{
    return d->governmentFormat;
}

void WorldModel::setGovernmentFormat(const QString& _text)
{
    if (d->governmentFormat == _text) {
        return;
    }
    d->governmentFormat = _text;
    emit governmentFormatChanged(d->governmentFormat);
}

QString WorldModel::governmentHistory() const
{
    return d->governmentHistory;
}

void WorldModel::setGovernmentHistory(const QString& _text)
{
    if (d->governmentHistory == _text) {
        return;
    }
    d->governmentHistory = _text;
    emit governmentHistoryChanged(d->governmentHistory);
}

QString WorldModel::laws() const
{
    return d->laws;
}

void WorldModel::setLaws(const QString& _text)
{
    if (d->laws == _text) {
        return;
    }
    d->laws = _text;
    emit lawsChanged(d->laws);
}

QString WorldModel::foreignRelations() const
{
    return d->foreignRelations;
}

void WorldModel::setForeignRelations(const QString& _text)
{
    if (d->foreignRelations == _text) {
        return;
    }
    d->foreignRelations = _text;
    emit foreignRelationsChanged(d->foreignRelations);
}

QString WorldModel::perceptionOfGovernment() const
{
    return d->perceptionOfGovernment;
}

void WorldModel::setPerceptionOfGovernment(const QString& _text)
{
    if (d->perceptionOfGovernment == _text) {
        return;
    }
    d->perceptionOfGovernment = _text;
    emit perceptionOfGovernmentChanged(d->perceptionOfGovernment);
}

QString WorldModel::propaganda() const
{
    return d->propaganda;
}

void WorldModel::setPropaganda(const QString& _text)
{
    if (d->propaganda == _text) {
        return;
    }
    d->propaganda = _text;
    emit propagandaChanged(d->propaganda);
}

QString WorldModel::antiGovernmentOrganisations() const
{
    return d->antiGovernmentOrganisations;
}

void WorldModel::setAntiGovernmentOrganisations(const QString& _text)
{
    if (d->antiGovernmentOrganisations == _text) {
        return;
    }
    d->antiGovernmentOrganisations = _text;
    emit antiGovernmentOrganisationsChanged(d->antiGovernmentOrganisations);
}

QString WorldModel::pastWar() const
{
    return d->pastWar;
}

void WorldModel::setPastWar(const QString& _text)
{
    if (d->pastWar == _text) {
        return;
    }
    d->pastWar = _text;
    emit pastWarChanged(d->pastWar);
}

QString WorldModel::currentWar() const
{
    return d->currentWar;
}

void WorldModel::setCurrentWar(const QString& _text)
{
    if (d->currentWar == _text) {
        return;
    }
    d->currentWar = _text;
    emit currentWarChanged(d->currentWar);
}

QString WorldModel::potentialWar() const
{
    return d->potentialWar;
}

void WorldModel::setPotentialWar(const QString& _text)
{
    if (d->potentialWar == _text) {
        return;
    }
    d->potentialWar = _text;
    emit potentialWarChanged(d->potentialWar);
}

QString WorldModel::magicRule() const
{
    return d->magicRule;
}

void WorldModel::setMagicRule(const QString& _text)
{
    if (d->magicRule == _text) {
        return;
    }
    d->magicRule = _text;
    emit magicRuleChanged(d->magicRule);
}

QString WorldModel::whoCanUse() const
{
    return d->whoCanUse;
}

void WorldModel::setWhoCanUse(const QString& _text)
{
    if (d->whoCanUse == _text) {
        return;
    }
    d->whoCanUse = _text;
    emit whoCanUseChanged(d->whoCanUse);
}

QString WorldModel::effectToWorld() const
{
    return d->effectToWorld;
}

void WorldModel::setEffectToWorld(const QString& _text)
{
    if (d->effectToWorld == _text) {
        return;
    }
    d->effectToWorld = _text;
    emit effectToWorldChanged(d->effectToWorld);
}

QString WorldModel::effectToSociety() const
{
    return d->effectToSociety;
}

void WorldModel::setEffectToSociety(const QString& _text)
{
    if (d->effectToSociety == _text) {
        return;
    }
    d->effectToSociety = _text;
    emit effectToSocietyChanged(d->effectToSociety);
}

QString WorldModel::effectToTechnology() const
{
    return d->effectToTechnology;
}

void WorldModel::setEffectToTechnology(const QString& _text)
{
    if (d->effectToTechnology == _text) {
        return;
    }
    d->effectToTechnology = _text;
    emit effectToTechnologyChanged(d->effectToTechnology);
}

QVector<WorldItem> WorldModel::magicTypes() const
{
    return d->magicTypes;
}

void WorldModel::setMagicTypes(const QVector<WorldItem>& _items)
{
    int index = 0;
    for (; index < _items.size(); ++index) {
        Q_ASSERT(d->magicTypes.size() >= index);
        if (d->magicTypes.size() == index) {
            d->magicTypes.append(WorldItem());
        }

        auto& oldMagicType = d->magicTypes[index];
        auto& newMagicType = _items[index];
        if (!oldMagicType.photo.uuid.isNull()
            && oldMagicType.photo.uuid != newMagicType.photo.uuid) {
            imageWrapper()->remove(oldMagicType.photo.uuid);
            oldMagicType.photo = {};
        }
        if (newMagicType.photo.uuid.isNull() && !newMagicType.photo.image.isNull()) {
            oldMagicType.photo
                = { imageWrapper()->save(newMagicType.photo.image), newMagicType.photo.image };
        }

        oldMagicType.name = newMagicType.name;
        oldMagicType.oneSentenceDescription = newMagicType.oneSentenceDescription;
        oldMagicType.longDescription = newMagicType.longDescription;
    }

    for (int lastIndex = d->magicTypes.size() - 1; lastIndex >= index; --lastIndex) {
        auto& magicType = d->magicTypes[lastIndex];
        if (!magicType.photo.uuid.isNull()) {
            imageWrapper()->remove(magicType.photo.uuid);
        }
        d->magicTypes.removeLast();
    }

    emit magicTypesChanged(d->magicTypes);
}

void WorldModel::initImageWrapper()
{
    connect(imageWrapper(), &AbstractImageWrapper::imageUpdated, this,
            [this](const QUuid& _uuid, const QPixmap& _image) {
                for (auto& photo : d->photos) {
                    if (photo.uuid == _uuid) {
                        photo.image = _image;

                        if (photo.uuid == d->photos.constFirst().uuid) {
                            emit mainPhotoChanged(photo);
                        }
                        emit photosChanged(d->photos);

                        break;
                    }
                }
            });
}

void WorldModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto load = [&documentNode](const QString& _key) {
        return documentNode.firstChildElement(_key).text();
    };
    d->name = load(kNameKey);
    d->oneSentenceDescription = load(kOneSentenceDescriptionKey);
    d->longDescription = load(kLongDescriptionKey);
    const auto photosNode = documentNode.firstChildElement(kPhotosKey);
    if (!photosNode.isNull()) {
        auto photoNode = photosNode.firstChildElement(kPhotoKey);
        while (!photoNode.isNull()) {
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                d->photos.append({ uuid, imageWrapper()->load(uuid) });
            }

            photoNode = photoNode.nextSiblingElement();
        }
    }
    auto relationsNode = documentNode.firstChildElement(kRoutesKey);
    if (!relationsNode.isNull()) {
        auto routeNode = relationsNode.firstChildElement(kRouteKey);
        while (!routeNode.isNull()) {
            WorldRoute route;
            route.world = QUuid::fromString(routeNode.firstChildElement(kRouteToWorldKey).text());
            route.lineType = routeNode.firstChildElement(kLineTypeKey).text().toInt();
            route.color = ColorHelper::fromString(routeNode.firstChildElement(kColorKey).text());
            route.name = TextHelper::fromHtmlEscaped(routeNode.firstChildElement(kNameKey).text());
            route.details
                = TextHelper::fromHtmlEscaped(routeNode.firstChildElement(kDetailsKey).text());
            d->routes.append(route);

            routeNode = routeNode.nextSiblingElement();
        }
    }
    d->overview = load(kOverviewKey);
    d->earthLike = load(kEarthLikeKey);
    d->history = load(kHistoryKey);
    d->mood = load(kMoodKey);
    d->biology = load(kBiologyKey);
    d->physics = load(kPhysicsKey);
    d->astronomy = load(kAstoronomyKey);
    d->geography = load(kGeographyKey);
    auto racesNode = documentNode.firstChildElement(kRacesKey);
    if (!racesNode.isNull()) {
        d->races.clear();
        auto raceNode = racesNode.firstChildElement(kRaceKey);
        while (!raceNode.isNull()) {
            WorldItem race;
            const auto photoNode = raceNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                race.photo = { uuid, imageWrapper()->load(uuid) };
            }
            race.name = TextHelper::fromHtmlEscaped(raceNode.firstChildElement(kNameKey).text());
            race.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                raceNode.firstChildElement(kOneSentenceDescriptionKey).text());
            race.longDescription = TextHelper::fromHtmlEscaped(
                raceNode.firstChildElement(kLongDescriptionKey).text());
            d->races.append(race);

            raceNode = raceNode.nextSiblingElement();
        }
    }
    auto florasNode = documentNode.firstChildElement(kFlorasKey);
    if (!florasNode.isNull()) {
        d->floras.clear();
        auto floraNode = florasNode.firstChildElement(kFloraKey);
        while (!floraNode.isNull()) {
            WorldItem flora;
            const auto photoNode = floraNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                flora.photo = { uuid, imageWrapper()->load(uuid) };
            }
            flora.name = TextHelper::fromHtmlEscaped(floraNode.firstChildElement(kNameKey).text());
            flora.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                floraNode.firstChildElement(kOneSentenceDescriptionKey).text());
            flora.longDescription = TextHelper::fromHtmlEscaped(
                floraNode.firstChildElement(kLongDescriptionKey).text());
            d->floras.append(flora);

            floraNode = floraNode.nextSiblingElement();
        }
    }
    auto animalsNode = documentNode.firstChildElement(kAnimalsKey);
    if (!animalsNode.isNull()) {
        d->animals.clear();
        auto animalNode = animalsNode.firstChildElement(kAnimalKey);
        while (!animalNode.isNull()) {
            WorldItem animal;
            const auto photoNode = animalNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                animal.photo = { uuid, imageWrapper()->load(uuid) };
            }
            animal.name
                = TextHelper::fromHtmlEscaped(animalNode.firstChildElement(kNameKey).text());
            animal.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                animalNode.firstChildElement(kOneSentenceDescriptionKey).text());
            animal.longDescription = TextHelper::fromHtmlEscaped(
                animalNode.firstChildElement(kLongDescriptionKey).text());
            d->animals.append(animal);

            animalNode = animalNode.nextSiblingElement();
        }
    }
    auto naturalResourcesNode = documentNode.firstChildElement(kNaturalResourcesKey);
    if (!naturalResourcesNode.isNull()) {
        d->naturalResources.clear();
        auto naturalResourceNode = naturalResourcesNode.firstChildElement(kNaturalResourceKey);
        while (!naturalResourceNode.isNull()) {
            WorldItem naturalResource;
            const auto photoNode = naturalResourceNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                naturalResource.photo = { uuid, imageWrapper()->load(uuid) };
            }
            naturalResource.name = TextHelper::fromHtmlEscaped(
                naturalResourceNode.firstChildElement(kNameKey).text());
            naturalResource.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                naturalResourceNode.firstChildElement(kOneSentenceDescriptionKey).text());
            naturalResource.longDescription = TextHelper::fromHtmlEscaped(
                naturalResourceNode.firstChildElement(kLongDescriptionKey).text());
            d->naturalResources.append(naturalResource);

            naturalResourceNode = naturalResourceNode.nextSiblingElement();
        }
    }
    auto climatesNode = documentNode.firstChildElement(kClimatesKey);
    if (!climatesNode.isNull()) {
        d->climates.clear();
        auto climateNode = climatesNode.firstChildElement(kClimateKey);
        while (!climateNode.isNull()) {
            WorldItem climate;
            const auto photoNode = climateNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                climate.photo = { uuid, imageWrapper()->load(uuid) };
            }
            climate.name
                = TextHelper::fromHtmlEscaped(climateNode.firstChildElement(kNameKey).text());
            climate.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                climateNode.firstChildElement(kOneSentenceDescriptionKey).text());
            climate.longDescription = TextHelper::fromHtmlEscaped(
                climateNode.firstChildElement(kLongDescriptionKey).text());
            d->climates.append(climate);

            climateNode = climateNode.nextSiblingElement();
        }
    }
    auto religionsNode = documentNode.firstChildElement(kReligionsKey);
    if (!religionsNode.isNull()) {
        d->religions.clear();
        auto religionNode = religionsNode.firstChildElement(kReligionKey);
        while (!religionNode.isNull()) {
            WorldItem religion;
            const auto photoNode = religionNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                religion.photo = { uuid, imageWrapper()->load(uuid) };
            }
            religion.name
                = TextHelper::fromHtmlEscaped(religionNode.firstChildElement(kNameKey).text());
            religion.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                religionNode.firstChildElement(kOneSentenceDescriptionKey).text());
            religion.longDescription = TextHelper::fromHtmlEscaped(
                religionNode.firstChildElement(kLongDescriptionKey).text());
            d->religions.append(religion);

            religionNode = religionNode.nextSiblingElement();
        }
    }
    auto ethicssNode = documentNode.firstChildElement(kEthicsKey);
    if (!ethicssNode.isNull()) {
        d->ethics.clear();
        auto ethicNode = ethicssNode.firstChildElement(kEthicKey);
        while (!ethicNode.isNull()) {
            WorldItem ethic;
            const auto photoNode = ethicNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                ethic.photo = { uuid, imageWrapper()->load(uuid) };
            }
            ethic.name = TextHelper::fromHtmlEscaped(ethicNode.firstChildElement(kNameKey).text());
            ethic.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                ethicNode.firstChildElement(kOneSentenceDescriptionKey).text());
            ethic.longDescription = TextHelper::fromHtmlEscaped(
                ethicNode.firstChildElement(kLongDescriptionKey).text());
            d->ethics.append(ethic);

            ethicNode = ethicNode.nextSiblingElement();
        }
    }
    auto languagesNode = documentNode.firstChildElement(kLanguagesKey);
    if (!languagesNode.isNull()) {
        d->languages.clear();
        auto languageNode = languagesNode.firstChildElement(kLanguageKey);
        while (!languageNode.isNull()) {
            WorldItem language;
            const auto photoNode = languageNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                language.photo = { uuid, imageWrapper()->load(uuid) };
            }
            language.name
                = TextHelper::fromHtmlEscaped(languageNode.firstChildElement(kNameKey).text());
            language.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                languageNode.firstChildElement(kOneSentenceDescriptionKey).text());
            language.longDescription = TextHelper::fromHtmlEscaped(
                languageNode.firstChildElement(kLongDescriptionKey).text());
            d->languages.append(language);

            languageNode = languageNode.nextSiblingElement();
        }
    }
    auto castesNode = documentNode.firstChildElement(kCastesKey);
    if (!castesNode.isNull()) {
        d->castes.clear();
        auto casteNode = castesNode.firstChildElement(kCasteKey);
        while (!casteNode.isNull()) {
            WorldItem caste;
            const auto photoNode = casteNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                caste.photo = { uuid, imageWrapper()->load(uuid) };
            }
            caste.name = TextHelper::fromHtmlEscaped(casteNode.firstChildElement(kNameKey).text());
            caste.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                casteNode.firstChildElement(kOneSentenceDescriptionKey).text());
            caste.longDescription = TextHelper::fromHtmlEscaped(
                casteNode.firstChildElement(kLongDescriptionKey).text());
            d->castes.append(caste);

            casteNode = casteNode.nextSiblingElement();
        }
    }
    d->technology = load(kTechnologyKey);
    d->economy = load(kEconomyKey);
    d->trade = load(kTradeKey);
    d->business = load(kBusinessKey);
    d->industry = load(kIndustryKey);
    d->currency = load(kCurrencyKey);
    d->education = load(kEducationKey);
    d->communication = load(kCommunicationKey);
    d->art = load(kArtKey);
    d->entertainment = load(kEntertainmentKey);
    d->travel = load(kTravelKey);
    d->science = load(kScienceKey);
    d->governmentFormat = load(kGovernmentFormatKey);
    d->governmentHistory = load(kGovernmentHistoryKey);
    d->laws = load(kLawsKey);
    d->foreignRelations = load(kForeignRelationsKey);
    d->perceptionOfGovernment = load(kPerceptionOfGovernmentKey);
    d->propaganda = load(kPropagandaKey);
    d->antiGovernmentOrganisations = load(kAntiGovernmentOrganisationsKey);
    d->pastWar = load(kPastWarKey);
    d->currentWar = load(kCurrentWarKey);
    d->potentialWar = load(kPotentialWarKey);
    d->magicRule = load(kMagicRuleKey);
    d->whoCanUse = load(kWhoCanUseKey);
    d->effectToWorld = load(kEffectToWorldKey);
    d->effectToSociety = load(kEffectToSocietyKey);
    d->effectToTechnology = load(kEffectToTechnologyKey);
    auto magicTypesNode = documentNode.firstChildElement(kMagicTypesKey);
    if (!magicTypesNode.isNull()) {
        d->magicTypes.clear();
        auto magicTypeNode = magicTypesNode.firstChildElement(kMagicTypeKey);
        while (!magicTypeNode.isNull()) {
            WorldItem magicType;
            const auto photoNode = magicTypeNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                magicType.photo = { uuid, imageWrapper()->load(uuid) };
            }
            magicType.name
                = TextHelper::fromHtmlEscaped(magicTypeNode.firstChildElement(kNameKey).text());
            magicType.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                magicTypeNode.firstChildElement(kOneSentenceDescriptionKey).text());
            magicType.longDescription = TextHelper::fromHtmlEscaped(
                magicTypeNode.firstChildElement(kLongDescriptionKey).text());
            d->magicTypes.append(magicType);

            magicTypeNode = magicTypeNode.nextSiblingElement();
        }
    }
}

void WorldModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray WorldModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    auto save = [&xml](const QString& _key, const QString& _value) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(_key, _value).toUtf8();
    };
    save(kNameKey, d->name);
    save(kOneSentenceDescriptionKey, d->oneSentenceDescription);
    save(kLongDescriptionKey, d->longDescription);
    if (!d->photos.isEmpty()) {
        xml += QString("<%1>\n").arg(kPhotosKey).toUtf8();
        for (const auto& photo : std::as_const(d->photos)) {
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(photo.uuid.toString()))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(kPhotosKey).toUtf8();
    }
    if (!d->routes.isEmpty()) {
        xml += QString("<%1>\n").arg(kRoutesKey).toUtf8();
        for (const auto& relation : std::as_const(d->routes)) {
            xml += QString("<%1>\n").arg(kRouteKey).toUtf8();
            save(kRouteToWorldKey, relation.world.toString());
            save(kLineTypeKey, QString::number(relation.lineType));
            if (relation.color.isValid()) {
                save(kColorKey, relation.color.name());
            }
            save(kNameKey, relation.name);
            save(kDetailsKey, relation.details);
            xml += QString("</%1>\n").arg(kRouteKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kRoutesKey).toUtf8();
    }
    save(kOverviewKey, d->overview);
    save(kEarthLikeKey, d->earthLike);
    save(kHistoryKey, d->history);
    save(kMoodKey, d->mood);
    save(kBiologyKey, d->biology);
    save(kPhysicsKey, d->physics);
    save(kAstoronomyKey, d->astronomy);
    save(kGeographyKey, d->geography);
    if (!d->races.isEmpty()) {
        xml += QString("<%1>\n").arg(kRacesKey).toUtf8();
        for (const auto& race : std::as_const(d->races)) {
            xml += QString("<%1>\n").arg(kRaceKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(race.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, race.name);
            save(kOneSentenceDescriptionKey, race.oneSentenceDescription);
            save(kLongDescriptionKey, race.longDescription);
            xml += QString("</%1>\n").arg(kRaceKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kRacesKey).toUtf8();
    }
    if (!d->floras.isEmpty()) {
        xml += QString("<%1>\n").arg(kFlorasKey).toUtf8();
        for (const auto& flora : std::as_const(d->floras)) {
            xml += QString("<%1>\n").arg(kFloraKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(flora.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, flora.name);
            save(kOneSentenceDescriptionKey, flora.oneSentenceDescription);
            save(kLongDescriptionKey, flora.longDescription);
            xml += QString("</%1>\n").arg(kFloraKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kFlorasKey).toUtf8();
    }
    if (!d->animals.isEmpty()) {
        xml += QString("<%1>\n").arg(kAnimalsKey).toUtf8();
        for (const auto& animal : std::as_const(d->animals)) {
            xml += QString("<%1>\n").arg(kAnimalKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(animal.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, animal.name);
            save(kOneSentenceDescriptionKey, animal.oneSentenceDescription);
            save(kLongDescriptionKey, animal.longDescription);
            xml += QString("</%1>\n").arg(kAnimalKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kAnimalsKey).toUtf8();
    }
    if (!d->naturalResources.isEmpty()) {
        xml += QString("<%1>\n").arg(kNaturalResourcesKey).toUtf8();
        for (const auto& naturalResource : std::as_const(d->naturalResources)) {
            xml += QString("<%1>\n").arg(kNaturalResourceKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey,
                            TextHelper::toHtmlEscaped(naturalResource.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, naturalResource.name);
            save(kOneSentenceDescriptionKey, naturalResource.oneSentenceDescription);
            save(kLongDescriptionKey, naturalResource.longDescription);
            xml += QString("</%1>\n").arg(kNaturalResourceKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kNaturalResourcesKey).toUtf8();
    }
    if (!d->climates.isEmpty()) {
        xml += QString("<%1>\n").arg(kClimatesKey).toUtf8();
        for (const auto& climate : std::as_const(d->climates)) {
            xml += QString("<%1>\n").arg(kClimateKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(climate.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, climate.name);
            save(kOneSentenceDescriptionKey, climate.oneSentenceDescription);
            save(kLongDescriptionKey, climate.longDescription);
            xml += QString("</%1>\n").arg(kClimateKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kClimatesKey).toUtf8();
    }
    if (!d->religions.isEmpty()) {
        xml += QString("<%1>\n").arg(kReligionsKey).toUtf8();
        for (const auto& religion : std::as_const(d->religions)) {
            xml += QString("<%1>\n").arg(kReligionKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(religion.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, religion.name);
            save(kOneSentenceDescriptionKey, religion.oneSentenceDescription);
            save(kLongDescriptionKey, religion.longDescription);
            xml += QString("</%1>\n").arg(kReligionKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kReligionsKey).toUtf8();
    }
    if (!d->ethics.isEmpty()) {
        xml += QString("<%1>\n").arg(kEthicsKey).toUtf8();
        for (const auto& ethic : std::as_const(d->ethics)) {
            xml += QString("<%1>\n").arg(kEthicKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(ethic.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, ethic.name);
            save(kOneSentenceDescriptionKey, ethic.oneSentenceDescription);
            save(kLongDescriptionKey, ethic.longDescription);
            xml += QString("</%1>\n").arg(kEthicKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kEthicsKey).toUtf8();
    }
    if (!d->languages.isEmpty()) {
        xml += QString("<%1>\n").arg(kLanguagesKey).toUtf8();
        for (const auto& language : std::as_const(d->languages)) {
            xml += QString("<%1>\n").arg(kLanguageKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(language.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, language.name);
            save(kOneSentenceDescriptionKey, language.oneSentenceDescription);
            save(kLongDescriptionKey, language.longDescription);
            xml += QString("</%1>\n").arg(kLanguageKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kLanguagesKey).toUtf8();
    }
    if (!d->castes.isEmpty()) {
        xml += QString("<%1>\n").arg(kCastesKey).toUtf8();
        for (const auto& caste : std::as_const(d->castes)) {
            xml += QString("<%1>\n").arg(kCasteKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(caste.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, caste.name);
            save(kOneSentenceDescriptionKey, caste.oneSentenceDescription);
            save(kLongDescriptionKey, caste.longDescription);
            xml += QString("</%1>\n").arg(kCasteKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kCastesKey).toUtf8();
    }
    save(kTechnologyKey, d->technology);
    save(kEconomyKey, d->economy);
    save(kTradeKey, d->trade);
    save(kBusinessKey, d->business);
    save(kIndustryKey, d->industry);
    save(kCurrencyKey, d->currency);
    save(kEducationKey, d->education);
    save(kCommunicationKey, d->communication);
    save(kArtKey, d->art);
    save(kEntertainmentKey, d->entertainment);
    save(kTravelKey, d->travel);
    save(kScienceKey, d->science);
    save(kGovernmentFormatKey, d->governmentFormat);
    save(kGovernmentHistoryKey, d->governmentHistory);
    save(kLawsKey, d->laws);
    save(kForeignRelationsKey, d->foreignRelations);
    save(kPerceptionOfGovernmentKey, d->perceptionOfGovernment);
    save(kPropagandaKey, d->propaganda);
    save(kAntiGovernmentOrganisationsKey, d->antiGovernmentOrganisations);
    save(kPastWarKey, d->pastWar);
    save(kCurrentWarKey, d->currentWar);
    save(kPotentialWarKey, d->potentialWar);
    save(kMagicRuleKey, d->magicRule);
    save(kWhoCanUseKey, d->whoCanUse);
    save(kEffectToWorldKey, d->effectToWorld);
    save(kEffectToSocietyKey, d->effectToSociety);
    save(kEffectToTechnologyKey, d->effectToTechnology);
    if (!d->magicTypes.isEmpty()) {
        xml += QString("<%1>\n").arg(kMagicTypesKey).toUtf8();
        for (const auto& magicType : std::as_const(d->magicTypes)) {
            xml += QString("<%1>\n").arg(kMagicTypeKey).toUtf8();
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(magicType.photo.uuid.toString()))
                       .toUtf8();
            save(kNameKey, magicType.name);
            save(kOneSentenceDescriptionKey, magicType.oneSentenceDescription);
            save(kLongDescriptionKey, magicType.longDescription);
            xml += QString("</%1>\n").arg(kMagicTypeKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kMagicTypesKey).toUtf8();
    }

    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor WorldModel::applyPatch(const QByteArray& _patch)
{
    //
    // Применяем изменения
    //
    const auto newContent = dmpController().applyPatch(toXml(), _patch);

    //
    // Cчитываем изменённые данные
    //
    QDomDocument domDocument;
    domDocument.setContent(newContent);
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto load = [&documentNode](const QString& _key) {
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
    };
    setName(load(kNameKey));
    setOneSentenceDescription(load(kOneSentenceDescriptionKey));
    setLongDescription(load(kLongDescriptionKey));
    //
    // Считываем фотографии
    //
    auto photosNode = documentNode.firstChildElement(kPhotosKey);
    QVector<QUuid> newPhotosUuids;
    if (!photosNode.isNull()) {
        auto photoNode = photosNode.firstChildElement(kPhotoKey);
        while (!photoNode.isNull()) {
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            newPhotosUuids.append(uuid);

            photoNode = photoNode.nextSiblingElement();
        }
    }
    //
    // ... корректируем текущие фотографии персонажа
    //
    for (int photoIndex = 0; photoIndex < d->photos.size(); ++photoIndex) {
        const auto& photo = d->photos.at(photoIndex);
        //
        // ... если такое отношение осталось актуальным, то оставим его в списке текущих
        //     и удалим из списка новых
        //
        if (newPhotosUuids.contains(photo.uuid)) {
            newPhotosUuids.removeAll(photo.uuid);
        }
        //
        // ... если такого отношения нет в списке новых, то удалим его из списка текущих
        //
        else {
            removePhoto(photo.uuid);
            --photoIndex;
        }
    }
    //
    // ... добавляем новые фотографии к персонажу
    //
    for (const auto& photoUuid : newPhotosUuids) {
        addPhoto({ photoUuid });
        imageWrapper()->load(photoUuid);
    }
    //
    // Cчитываем отношения
    //
    auto routesNode = documentNode.firstChildElement(kRoutesKey);
    QVector<WorldRoute> newRoutes;
    if (!routesNode.isNull()) {
        auto routeNode = routesNode.firstChildElement(kRouteKey);
        while (!routeNode.isNull()) {
            WorldRoute route;
            route.world = QUuid::fromString(routeNode.firstChildElement(kRouteToWorldKey).text());
            route.lineType = routeNode.firstChildElement(kLineTypeKey).text().toInt();
            route.color = ColorHelper::fromString(routeNode.firstChildElement(kColorKey).text());
            route.name = TextHelper::fromHtmlEscaped(routeNode.firstChildElement(kNameKey).text());
            route.details
                = TextHelper::fromHtmlEscaped(routeNode.firstChildElement(kDetailsKey).text());
            newRoutes.append(route);

            routeNode = routeNode.nextSiblingElement();
        }
    }
    //
    // ... корректируем текущие отношения персонажа
    //
    for (int routeIndex = 0; routeIndex < d->routes.size(); ++routeIndex) {
        const auto& route = d->routes.at(routeIndex);
        //
        // ... если такое отношение осталось актуальным, то оставим его в списке текущих
        //     и удалим из списка новых
        //
        if (newRoutes.contains(route)) {
            newRoutes.removeAll(route);
        }
        //
        // ... если такого отношения нет в списке новых, то удалим его из списка текущих
        //
        else {
            removeRoute(route.world);
            --routeIndex;
        }
    }
    //
    // ... добавляем новые отношения к персонажу
    //
    for (const auto& route : newRoutes) {
        createRoute(route.world);
        updateRoute(route);
    }
    setOverview(load(kOverviewKey));
    setEarthLike(load(kEarthLikeKey));
    setHistory(load(kHistoryKey));
    setMood(load(kMoodKey));
    setBiology(load(kBiologyKey));
    setPhysics(load(kPhysicsKey));
    setAstronomy(load(kAstoronomyKey));
    setGeography(load(kGeographyKey));
    //
    auto racesNode = documentNode.firstChildElement(kRacesKey);
    QVector<WorldItem> newRaces;
    if (!racesNode.isNull()) {
        auto raceNode = racesNode.firstChildElement(kRaceKey);
        while (!raceNode.isNull()) {
            WorldItem race;
            const auto photoNode = raceNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                race.photo = { uuid, imageWrapper()->load(uuid) };
            }
            race.name = TextHelper::fromHtmlEscaped(raceNode.firstChildElement(kNameKey).text());
            race.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                raceNode.firstChildElement(kOneSentenceDescriptionKey).text());
            race.longDescription = TextHelper::fromHtmlEscaped(
                raceNode.firstChildElement(kLongDescriptionKey).text());
            newRaces.append(race);

            raceNode = raceNode.nextSiblingElement();
        }
    }
    setRaces(newRaces);
    //
    auto florasNode = documentNode.firstChildElement(kFlorasKey);
    QVector<WorldItem> newFloras;
    if (!florasNode.isNull()) {
        auto floraNode = florasNode.firstChildElement(kFloraKey);
        while (!floraNode.isNull()) {
            WorldItem flora;
            const auto photoNode = floraNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                flora.photo = { uuid, imageWrapper()->load(uuid) };
            }
            flora.name = TextHelper::fromHtmlEscaped(floraNode.firstChildElement(kNameKey).text());
            flora.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                floraNode.firstChildElement(kOneSentenceDescriptionKey).text());
            flora.longDescription = TextHelper::fromHtmlEscaped(
                floraNode.firstChildElement(kLongDescriptionKey).text());
            newFloras.append(flora);

            floraNode = floraNode.nextSiblingElement();
        }
    }
    setFloras(newFloras);
    //
    auto animalsNode = documentNode.firstChildElement(kAnimalsKey);
    QVector<WorldItem> newAnimals;
    if (!animalsNode.isNull()) {
        auto animalNode = animalsNode.firstChildElement(kAnimalKey);
        while (!animalNode.isNull()) {
            WorldItem animal;
            const auto photoNode = animalNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                animal.photo = { uuid, imageWrapper()->load(uuid) };
            }
            animal.name
                = TextHelper::fromHtmlEscaped(animalNode.firstChildElement(kNameKey).text());
            animal.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                animalNode.firstChildElement(kOneSentenceDescriptionKey).text());
            animal.longDescription = TextHelper::fromHtmlEscaped(
                animalNode.firstChildElement(kLongDescriptionKey).text());
            newAnimals.append(animal);

            animalNode = animalNode.nextSiblingElement();
        }
    }
    setAnimals(newAnimals);
    //
    auto naturalResourcesNode = documentNode.firstChildElement(kNaturalResourcesKey);
    QVector<WorldItem> newNaturalResources;
    if (!naturalResourcesNode.isNull()) {
        auto naturalResourceNode = naturalResourcesNode.firstChildElement(kNaturalResourceKey);
        while (!naturalResourceNode.isNull()) {
            WorldItem naturalResource;
            const auto photoNode = naturalResourceNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                naturalResource.photo = { uuid, imageWrapper()->load(uuid) };
            }
            naturalResource.name = TextHelper::fromHtmlEscaped(
                naturalResourceNode.firstChildElement(kNameKey).text());
            naturalResource.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                naturalResourceNode.firstChildElement(kOneSentenceDescriptionKey).text());
            naturalResource.longDescription = TextHelper::fromHtmlEscaped(
                naturalResourceNode.firstChildElement(kLongDescriptionKey).text());
            newNaturalResources.append(naturalResource);

            naturalResourceNode = naturalResourceNode.nextSiblingElement();
        }
    }
    setNaturalResources(newNaturalResources);
    //
    auto climatesNode = documentNode.firstChildElement(kClimatesKey);
    QVector<WorldItem> newClimates;
    if (!climatesNode.isNull()) {
        auto climateNode = climatesNode.firstChildElement(kClimateKey);
        while (!climateNode.isNull()) {
            WorldItem climate;
            const auto photoNode = climateNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                climate.photo = { uuid, imageWrapper()->load(uuid) };
            }
            climate.name
                = TextHelper::fromHtmlEscaped(climateNode.firstChildElement(kNameKey).text());
            climate.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                climateNode.firstChildElement(kOneSentenceDescriptionKey).text());
            climate.longDescription = TextHelper::fromHtmlEscaped(
                climateNode.firstChildElement(kLongDescriptionKey).text());
            newClimates.append(climate);

            climateNode = climateNode.nextSiblingElement();
        }
    }
    setClimates(newClimates);
    //
    auto religionsNode = documentNode.firstChildElement(kReligionsKey);
    QVector<WorldItem> newReligions;
    if (!religionsNode.isNull()) {
        auto religionNode = religionsNode.firstChildElement(kReligionKey);
        while (!religionNode.isNull()) {
            WorldItem religion;
            const auto photoNode = religionNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                religion.photo = { uuid, imageWrapper()->load(uuid) };
            }
            religion.name
                = TextHelper::fromHtmlEscaped(religionNode.firstChildElement(kNameKey).text());
            religion.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                religionNode.firstChildElement(kOneSentenceDescriptionKey).text());
            religion.longDescription = TextHelper::fromHtmlEscaped(
                religionNode.firstChildElement(kLongDescriptionKey).text());
            newReligions.append(religion);

            religionNode = religionNode.nextSiblingElement();
        }
    }
    setReligions(newReligions);
    //
    auto ethicsNode = documentNode.firstChildElement(kEthicsKey);
    QVector<WorldItem> newEthics;
    if (!ethicsNode.isNull()) {
        auto ethicNode = ethicsNode.firstChildElement(kEthicKey);
        while (!ethicNode.isNull()) {
            WorldItem ethic;
            const auto photoNode = ethicNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                ethic.photo = { uuid, imageWrapper()->load(uuid) };
            }
            ethic.name = TextHelper::fromHtmlEscaped(ethicNode.firstChildElement(kNameKey).text());
            ethic.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                ethicNode.firstChildElement(kOneSentenceDescriptionKey).text());
            ethic.longDescription = TextHelper::fromHtmlEscaped(
                ethicNode.firstChildElement(kLongDescriptionKey).text());
            newEthics.append(ethic);

            ethicNode = ethicNode.nextSiblingElement();
        }
    }
    setEthics(newEthics);
    //
    auto languagesNode = documentNode.firstChildElement(kLanguagesKey);
    QVector<WorldItem> newLanguages;
    if (!languagesNode.isNull()) {
        auto languageNode = languagesNode.firstChildElement(kLanguageKey);
        while (!languageNode.isNull()) {
            WorldItem language;
            const auto photoNode = languageNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                language.photo = { uuid, imageWrapper()->load(uuid) };
            }
            language.name
                = TextHelper::fromHtmlEscaped(languageNode.firstChildElement(kNameKey).text());
            language.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                languageNode.firstChildElement(kOneSentenceDescriptionKey).text());
            language.longDescription = TextHelper::fromHtmlEscaped(
                languageNode.firstChildElement(kLongDescriptionKey).text());
            newLanguages.append(language);

            languageNode = languageNode.nextSiblingElement();
        }
    }
    setLanguages(newLanguages);
    //
    auto castesNode = documentNode.firstChildElement(kCastesKey);
    QVector<WorldItem> newCastes;
    if (!castesNode.isNull()) {
        auto casteNode = castesNode.firstChildElement(kCasteKey);
        while (!casteNode.isNull()) {
            WorldItem caste;
            const auto photoNode = casteNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                caste.photo = { uuid, imageWrapper()->load(uuid) };
            }
            caste.name = TextHelper::fromHtmlEscaped(casteNode.firstChildElement(kNameKey).text());
            caste.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                casteNode.firstChildElement(kOneSentenceDescriptionKey).text());
            caste.longDescription = TextHelper::fromHtmlEscaped(
                casteNode.firstChildElement(kLongDescriptionKey).text());
            newCastes.append(caste);

            casteNode = casteNode.nextSiblingElement();
        }
    }
    setCastes(newCastes);
    //
    setTechnology(load(kTechnologyKey));
    setEconomy(load(kEconomyKey));
    setTrade(load(kTradeKey));
    setBusiness(load(kBusinessKey));
    setIndustry(load(kIndustryKey));
    setCurrency(load(kCurrencyKey));
    setEducation(load(kEducationKey));
    setCommunication(load(kCommunicationKey));
    setArt(load(kArtKey));
    setEntertainment(load(kEntertainmentKey));
    setTravel(load(kTravelKey));
    setScience(load(kScienceKey));
    //
    setGovernmentFormat(load(kGovernmentFormatKey));
    setGovernmentHistory(load(kGovernmentHistoryKey));
    setLaws(load(kLawsKey));
    setForeignRelations(load(kForeignRelationsKey));
    setPerceptionOfGovernment(load(kPerceptionOfGovernmentKey));
    setPropaganda(load(kPropagandaKey));
    setAntiGovernmentOrganisations(load(kAntiGovernmentOrganisationsKey));
    setPastWar(load(kPastWarKey));
    setCurrentWar(load(kCurrentWarKey));
    setPotentialWar(load(kPotentialWarKey));
    //
    setMagicRule(load(kMagicRuleKey));
    setWhoCanUse(load(kWhoCanUseKey));
    setEffectToWorld(load(kEffectToWorldKey));
    setEffectToSociety(load(kEffectToSocietyKey));
    setEffectToTechnology(load(kEffectToTechnologyKey));
    auto magicTypesNode = documentNode.firstChildElement(kMagicTypesKey);
    QVector<WorldItem> newMagicTypes;
    if (!magicTypesNode.isNull()) {
        auto magicTypeNode = magicTypesNode.firstChildElement(kMagicTypeKey);
        while (!magicTypeNode.isNull()) {
            WorldItem magicType;
            const auto photoNode = magicTypeNode.firstChildElement(kPhotoKey);
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                magicType.photo = { uuid, imageWrapper()->load(uuid) };
            }
            magicType.name
                = TextHelper::fromHtmlEscaped(magicTypeNode.firstChildElement(kNameKey).text());
            magicType.oneSentenceDescription = TextHelper::fromHtmlEscaped(
                magicTypeNode.firstChildElement(kOneSentenceDescriptionKey).text());
            magicType.longDescription = TextHelper::fromHtmlEscaped(
                magicTypeNode.firstChildElement(kLongDescriptionKey).text());
            newMagicTypes.append(magicType);

            magicTypeNode = magicTypeNode.nextSiblingElement();
        }
    }
    setMagicTypes(newMagicTypes);

    return {};
}

} // namespace BusinessLayer

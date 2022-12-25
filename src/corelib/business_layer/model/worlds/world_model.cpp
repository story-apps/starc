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
const QLatin1String kBiologyKey("biology");
const QLatin1String kPhisicsKey("phisics");
const QLatin1String kAstoronomyKey("astronomy");
const QLatin1String kGeographyKey("geography");
const QLatin1String kRacesKey("races");
const QLatin1String kRaceKey("race");
// const QLatin1String kWorldKey("world");
// const QLatin1String kClimateKey("climate");
// const QLatin1String kLandmarkKey("landmark");
// const QLatin1String kNearbyPlacesKey("nearby_places");
// const QLatin1String kHistoryKey("history");
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
    QString phisics;
    QString astronomy;
    QString geography;
    QVector<WorldItem> races;
    //    QString world;
    //    QString climate;
    //    QString landmark;
    //    QString nearbyPlaces;
    //    QString history;
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
    connect(this, &WorldModel::phisicsChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::astronomyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::geographyChanged, this, &WorldModel::updateDocumentContent);
    connect(this, &WorldModel::racesChanged, this, &WorldModel::updateDocumentContent);
    //    connect(this, &WorldModel::worldChanged, this, &WorldModel::updateDocumentContent);
    //    connect(this, &WorldModel::climateChanged, this, &WorldModel::updateDocumentContent);
    //    connect(this, &WorldModel::landmarkChanged, this, &WorldModel::updateDocumentContent);
    //    connect(this, &WorldModel::nearbyPlacesChanged, this, &WorldModel::updateDocumentContent);
    //    connect(this, &WorldModel::historyChanged, this, &WorldModel::updateDocumentContent);
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

QString WorldModel::phisics() const
{
    return d->phisics;
}

void WorldModel::setPhisics(const QString& _text)
{
    if (d->phisics == _text) {
        return;
    }
    d->phisics = _text;
    emit phisicsChanged(d->phisics);
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

void WorldModel::setRaces(const QVector<WorldItem>& _races)
{
    int index = 0;
    for (; index < _races.size(); ++index) {
        Q_ASSERT(d->races.size() >= index);
        if (d->races.size() == index) {
            d->races.append(WorldItem());
        }

        auto& oldRace = d->races[index];
        auto& newRace = _races[index];
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

// QString WorldModel::world() const
//{
//    return d->world;
//}

// void WorldModel::setWorld(const QString& _text)
//{
//    if (d->world == _text) {
//        return;
//    }
//    d->world = _text;
//    emit worldChanged(d->world);
//}

// QString WorldModel::climate() const
//{
//    return d->climate;
//}

// void WorldModel::setClimate(const QString& _text)
//{
//    if (d->climate == _text) {
//        return;
//    }
//    d->climate = _text;
//    emit climateChanged(d->climate);
//}

// QString WorldModel::landmark() const
//{
//    return d->landmark;
//}

// void WorldModel::setLandmark(const QString& _text)
//{
//    if (d->landmark == _text) {
//        return;
//    }
//    d->landmark = _text;
//    emit landmarkChanged(d->landmark);
//}

// QString WorldModel::nearbyPlaces() const
//{
//    return d->nearbyPlaces;
//}

// void WorldModel::setNearbyPlaces(const QString& _text)
//{
//    if (d->nearbyPlaces == _text) {
//        return;
//    }
//    d->nearbyPlaces = _text;
//    emit nearbyPlacesChanged(d->nearbyPlaces);
//}

// QString WorldModel::history() const
//{
//    return d->history;
//}

// void WorldModel::setHistory(const QString& _text)
//{
//    if (d->history == _text) {
//        return;
//    }
//    d->history = _text;
//    emit historyChanged(d->history);
//}

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
    d->phisics = load(kPhisicsKey);
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
    //    d->world = load(kWorldKey);
    //    d->climate = load(kClimateKey);
    //    d->landmark = load(kLandmarkKey);
    //    d->nearbyPlaces = load(kNearbyPlacesKey);
    //    d->history = load(kHistoryKey);
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
    save(kPhisicsKey, d->phisics);
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
    //    save(kWorldKey, d->world);
    //    save(kClimateKey, d->climate);
    //    save(kLandmarkKey, d->landmark);
    //    save(kNearbyPlacesKey, d->nearbyPlaces);
    //    save(kHistoryKey, d->history);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

void WorldModel::applyPatch(const QByteArray& _patch)
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
    setPhisics(load(kPhisicsKey));
    setAstronomy(load(kAstoronomyKey));
    setGeography(load(kGeographyKey));
    //
    // Cчитываем расы
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
    //
    // ... корректируем текущие расы
    //
    setRaces(newRaces);
    //    setWorld(load(kWorldKey));
    //    setClimate(load(kClimateKey));
    //    setLandmark(load(kLandmarkKey));
    //    setNearbyPlaces(load(kNearbyPlacesKey));
    //    setHistory(load(kHistoryKey));
}

} // namespace BusinessLayer

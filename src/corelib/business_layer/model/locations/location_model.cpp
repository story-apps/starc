#include "location_model.h"

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
const QLatin1String kStoryRoleKey("story_role");
const QLatin1String kOneSentenceDescriptionKey("one_sentence_description");
const QLatin1String kLongDescriptionKey("long_description");
const QLatin1String kMainPhotoKey("main_photo");
const QLatin1String kPhotosKey("photos");
const QLatin1String kPhotoKey("photo");
const QLatin1String kRoutesKey("routes");
const QLatin1String kRouteKey("route");
const QLatin1String kRouteToLocationKey("to");
const QLatin1String kLineTypeKey("line_type");
const QLatin1String kDetailsKey("details");

const QLatin1String kSightKey("sight");
const QLatin1String kSmellKey("smell");
const QLatin1String kSoundKey("sound");
const QLatin1String kTasteKey("taste");
const QLatin1String kTouchKey("touch");
const QLatin1String kLocationKey("location");
const QLatin1String kClimateKey("climate");
const QLatin1String kLandmarkKey("landmark");
const QLatin1String kNearbyPlacesKey("nearby_places");
const QLatin1String kHistoryKey("history");
} // namespace

class LocationModel::Implementation
{
public:
    QString name;
    LocationStoryRole storyRole = LocationStoryRole::Undefined;
    QString oneSentenceDescription;
    QString longDescription;
    QVector<Domain::DocumentImage> photos;
    QVector<LocationRoute> routes;

    QString sight;
    QString smell;
    QString sound;
    QString taste;
    QString touch;
    QString location;
    QString climate;
    QString landmark;
    QString nearbyPlaces;
    QString history;
};


// ****


bool LocationRoute::isValid() const
{
    return !location.isNull();
}

bool LocationRoute::operator==(const LocationRoute& _other) const
{
    return location == _other.location && lineType == _other.lineType && color == _other.color
        && name == _other.name && details == _other.details;
}

bool LocationRoute::operator!=(const LocationRoute& _other) const
{
    return !(*this == _other);
}


// ****


LocationModel::LocationModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kNameKey,
            kStoryRoleKey,
            kOneSentenceDescriptionKey,
            kLongDescriptionKey,
            kMainPhotoKey,
            kRoutesKey,
            kRouteKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &LocationModel::nameChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::storyRoleChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::oneSentenceDescriptionChanged, this,
            &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::longDescriptionChanged, this,
            &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::mainPhotoChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::photosChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::routeAdded, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::routeChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::routeRemoved, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::sightChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::smellChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::soundChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::tasteChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::touchChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::locationChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::climateChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::landmarkChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::nearbyPlacesChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::historyChanged, this, &LocationModel::updateDocumentContent);
}

LocationModel::~LocationModel() = default;

const QString& LocationModel::name() const
{
    return d->name;
}

void LocationModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    const auto oldName = d->name;
    d->name = _name;

    emit nameChanged(d->name, oldName);
    emit documentNameChanged(d->name);
}

QString LocationModel::documentName() const
{
    return name();
}

void LocationModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

LocationStoryRole LocationModel::storyRole() const
{
    return d->storyRole;
}

void LocationModel::setStoryRole(LocationStoryRole _role)
{
    if (d->storyRole == _role) {
        return;
    }

    d->storyRole = _role;
    emit storyRoleChanged(d->storyRole);
}

QString LocationModel::oneSentenceDescription() const
{
    return d->oneSentenceDescription;
}

void LocationModel::setOneSentenceDescription(const QString& _text)
{
    if (d->oneSentenceDescription == _text) {
        return;
    }

    d->oneSentenceDescription = _text;
    emit oneSentenceDescriptionChanged(d->oneSentenceDescription);
}

QString LocationModel::longDescription() const
{
    return d->longDescription;
}

void LocationModel::setLongDescription(const QString& _text)
{
    if (d->longDescription == _text) {
        return;
    }

    d->longDescription = _text;
    emit longDescriptionChanged(d->longDescription);
}

Domain::DocumentImage LocationModel::mainPhoto() const
{
    if (d->photos.isEmpty()) {
        return {};
    }

    return d->photos.constFirst();
}

void LocationModel::setMainPhoto(const QPixmap& _photo)
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

QVector<Domain::DocumentImage> LocationModel::photos() const
{
    return d->photos;
}

void LocationModel::addPhoto(const Domain::DocumentImage& _photo)
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

void LocationModel::addPhotos(const QVector<QPixmap>& _photos)
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

void LocationModel::removePhoto(const QUuid& _photoUuid)
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

void LocationModel::createRoute(const QUuid& _toLocation)
{
    for (const auto& relation : d->routes) {
        if (relation.location == _toLocation) {
            return;
        }
    }

    d->routes.append({ _toLocation });
    emit routeAdded(d->routes.constLast());
}

void LocationModel::updateRoute(const LocationRoute& _way)
{
    for (auto& relation : d->routes) {
        if (relation.location != _way.location) {
            continue;
        }

        if (relation != _way) {
            relation = _way;
            emit routeChanged(relation);
        }
        return;
    }
}

void LocationModel::removeRoute(QUuid _toLocation)
{
    for (int index = 0; index < d->routes.size(); ++index) {
        if (d->routes[index].location != _toLocation) {
            continue;
        }

        auto relation = d->routes.takeAt(index);
        emit routeRemoved(relation);
        return;
    }
}

LocationRoute LocationModel::route(const QUuid& _toLocation)
{
    for (auto& relation : d->routes) {
        if (relation.location == _toLocation) {
            return relation;
        }
    }

    return {};
}

LocationRoute LocationModel::route(LocationModel* _toLocation)
{
    return route(_toLocation->document()->uuid());
}

QVector<LocationRoute> LocationModel::routes() const
{
    return d->routes;
}

QString LocationModel::sight() const
{
    return d->sight;
}

void LocationModel::setSight(const QString& _text)
{
    if (d->sight == _text) {
        return;
    }
    d->sight = _text;
    emit sightChanged(d->sight);
}

QString LocationModel::smell() const
{
    return d->smell;
}

void LocationModel::setSmell(const QString& _text)
{
    if (d->smell == _text) {
        return;
    }
    d->smell = _text;
    emit smellChanged(d->smell);
}

QString LocationModel::sound() const
{
    return d->sound;
}

void LocationModel::setSound(const QString& _text)
{
    if (d->sound == _text) {
        return;
    }
    d->sound = _text;
    emit soundChanged(d->sound);
}

QString LocationModel::taste() const
{
    return d->taste;
}

void LocationModel::setTaste(const QString& _text)
{
    if (d->taste == _text) {
        return;
    }
    d->taste = _text;
    emit tasteChanged(d->taste);
}

QString LocationModel::touch() const
{
    return d->touch;
}

void LocationModel::setTouch(const QString& _text)
{
    if (d->touch == _text) {
        return;
    }
    d->touch = _text;
    emit touchChanged(d->touch);
}

QString LocationModel::location() const
{
    return d->location;
}

void LocationModel::setLocation(const QString& _text)
{
    if (d->location == _text) {
        return;
    }
    d->location = _text;
    emit locationChanged(d->location);
}

QString LocationModel::climate() const
{
    return d->climate;
}

void LocationModel::setClimate(const QString& _text)
{
    if (d->climate == _text) {
        return;
    }
    d->climate = _text;
    emit climateChanged(d->climate);
}

QString LocationModel::landmark() const
{
    return d->landmark;
}

void LocationModel::setLandmark(const QString& _text)
{
    if (d->landmark == _text) {
        return;
    }
    d->landmark = _text;
    emit landmarkChanged(d->landmark);
}

QString LocationModel::nearbyPlaces() const
{
    return d->nearbyPlaces;
}

void LocationModel::setNearbyPlaces(const QString& _text)
{
    if (d->nearbyPlaces == _text) {
        return;
    }
    d->nearbyPlaces = _text;
    emit nearbyPlacesChanged(d->nearbyPlaces);
}

QString LocationModel::history() const
{
    return d->history;
}

void LocationModel::setHistory(const QString& _text)
{
    if (d->history == _text) {
        return;
    }
    d->history = _text;
    emit historyChanged(d->history);
}

void LocationModel::initImageWrapper()
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

void LocationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto contains = [&documentNode](const QString& _key) {
        return !documentNode.firstChildElement(_key).isNull();
    };
    auto load = [&documentNode](const QString& _key) {
        return documentNode.firstChildElement(_key).text();
    };
    d->name = load(kNameKey);
    if (contains(kStoryRoleKey)) {
        d->storyRole = static_cast<LocationStoryRole>(load(kStoryRoleKey).toInt());
    }
    d->oneSentenceDescription = load(kOneSentenceDescriptionKey);
    d->longDescription = load(kLongDescriptionKey);
    //
    // TODO: выпилить старый метод на считываниме главного изображения в версии 0.4.0
    //
    if (contains(kMainPhotoKey)) {
        const auto uuid = QUuid::fromString(load(kMainPhotoKey));
        if (!uuid.isNull()) {
            d->photos.append({ uuid, imageWrapper()->load(uuid) });
        }
    } else {
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
    }
    auto relationsNode = documentNode.firstChildElement(kRoutesKey);
    if (!relationsNode.isNull()) {
        auto routeNode = relationsNode.firstChildElement(kRouteKey);
        while (!routeNode.isNull()) {
            LocationRoute route;
            route.location
                = QUuid::fromString(routeNode.firstChildElement(kRouteToLocationKey).text());
            route.lineType = routeNode.firstChildElement(kLineTypeKey).text().toInt();
            route.color = ColorHelper::fromString(routeNode.firstChildElement(kColorKey).text());
            route.name = TextHelper::fromHtmlEscaped(routeNode.firstChildElement(kNameKey).text());
            route.details
                = TextHelper::fromHtmlEscaped(routeNode.firstChildElement(kDetailsKey).text());
            d->routes.append(route);

            routeNode = routeNode.nextSiblingElement();
        }
    }
    d->sight = load(kSightKey);
    d->smell = load(kSmellKey);
    d->sound = load(kSoundKey);
    d->taste = load(kTasteKey);
    d->touch = load(kTouchKey);
    d->location = load(kLocationKey);
    d->climate = load(kClimateKey);
    d->landmark = load(kLandmarkKey);
    d->nearbyPlaces = load(kNearbyPlacesKey);
    d->history = load(kHistoryKey);
}

void LocationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray LocationModel::toXml() const
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
    save(kStoryRoleKey, QString::number(static_cast<int>(d->storyRole)));
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
            save(kRouteToLocationKey, relation.location.toString());
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
    save(kSightKey, d->sight);
    save(kSmellKey, d->smell);
    save(kSoundKey, d->sound);
    save(kTasteKey, d->taste);
    save(kTouchKey, d->touch);
    save(kLocationKey, d->location);
    save(kClimateKey, d->climate);
    save(kLandmarkKey, d->landmark);
    save(kNearbyPlacesKey, d->nearbyPlaces);
    save(kHistoryKey, d->history);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor LocationModel::applyPatch(const QByteArray& _patch)
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
    auto contains = [&documentNode](const QString& _key) {
        return !documentNode.firstChildElement(_key).isNull();
    };
    auto load = [&documentNode](const QString& _key) {
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
    };
    setName(load(kNameKey));
    if (contains(kStoryRoleKey)) {
        setStoryRole(static_cast<LocationStoryRole>(load(kStoryRoleKey).toInt()));
    }
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
    QVector<LocationRoute> newRoutes;
    if (!routesNode.isNull()) {
        auto routeNode = routesNode.firstChildElement(kRouteKey);
        while (!routeNode.isNull()) {
            LocationRoute route;
            route.location
                = QUuid::fromString(routeNode.firstChildElement(kRouteToLocationKey).text());
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
            removeRoute(route.location);
            --routeIndex;
        }
    }
    //
    // ... добавляем новые отношения к персонажу
    //
    for (const auto& route : newRoutes) {
        createRoute(route.location);
        updateRoute(route);
    }
    setSight(load(kSightKey));
    setSmell(load(kSmellKey));
    setSound(load(kSoundKey));
    setTaste(load(kTasteKey));
    setTouch(load(kTouchKey));
    setLocation(load(kLocationKey));
    setClimate(load(kClimateKey));
    setLandmark(load(kLandmarkKey));
    setNearbyPlaces(load(kNearbyPlacesKey));
    setHistory(load(kHistoryKey));

    return {};
}

} // namespace BusinessLayer

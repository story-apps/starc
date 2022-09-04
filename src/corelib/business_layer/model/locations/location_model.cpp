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
const QLatin1String kRoutesKey("routes");
const QLatin1String kRouteKey("route");
const QLatin1String kRouteToLocationKey("to");
const QLatin1String kLineTypeKey("line_type");
const QLatin1String kDetailsKey("details");
} // namespace

class LocationModel::Implementation
{
public:
    QString name;
    LocationStoryRole storyRole = LocationStoryRole::Undefined;
    QString oneSentenceDescription;
    QString longDescription;
    Domain::DocumentImage mainPhoto;
    QVector<LocationRoute> routes;
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
    connect(this, &LocationModel::routeAdded, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::routeChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::routeRemoved, this, &LocationModel::updateDocumentContent);
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

const QPixmap& LocationModel::mainPhoto() const
{
    return d->mainPhoto.image;
}

void LocationModel::setMainPhoto(const QPixmap& _photo)
{
    if (_photo.cacheKey() == d->mainPhoto.image.cacheKey()) {
        return;
    }

    d->mainPhoto.image = _photo;
    if (d->mainPhoto.uuid.isNull()) {
        d->mainPhoto.uuid = imageWrapper()->save(d->mainPhoto.image);
    } else {
        imageWrapper()->save(d->mainPhoto.uuid, d->mainPhoto.image);
    }
    emit mainPhotoChanged(d->mainPhoto.image);
}

void LocationModel::setMainPhoto(const QUuid& _uuid, const QPixmap& _photo)
{
    if (d->mainPhoto.uuid == _uuid && _photo.cacheKey() == d->mainPhoto.image.cacheKey()) {
        return;
    }

    d->mainPhoto.image = _photo;
    if (d->mainPhoto.uuid != _uuid) {
        d->mainPhoto.uuid = _uuid;
    }
    emit mainPhotoChanged(d->mainPhoto.image);
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

void LocationModel::initImageWrapper()
{
    connect(imageWrapper(), &AbstractImageWrapper::imageUpdated, this,
            [this](const QUuid& _uuid, const QPixmap& _image) {
                if (_uuid != d->mainPhoto.uuid) {
                    return;
                }

                setMainPhoto(_uuid, _image);
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
    d->mainPhoto.uuid = QUuid::fromString(load(kMainPhotoKey));
    d->mainPhoto.image = imageWrapper()->load(d->mainPhoto.uuid);
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
    save(kMainPhotoKey, d->mainPhoto.uuid.toString());
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
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

void LocationModel::applyPatch(const QByteArray& _patch)
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
    setMainPhoto(load(kMainPhotoKey), imageWrapper()->load(load(kMainPhotoKey)));

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
}

} // namespace BusinessLayer

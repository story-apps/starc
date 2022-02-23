#include "location_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>
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
    d->mainPhoto.image = _photo;
    d->mainPhoto.uuid = imageWrapper()->save(d->mainPhoto.image);
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
        auto relationNode = relationsNode.firstChildElement(kRouteKey);
        while (!relationNode.isNull()) {
            LocationRoute relation;
            relation.location
                = QUuid::fromString(relationNode.firstChildElement(kRouteToLocationKey).text());
            relation.lineType = relationNode.firstChildElement(kLineTypeKey).text().toInt();
            relation.color
                = ColorHelper::fromString(relationNode.firstChildElement(kColorKey).text());
            relation.name
                = TextHelper::fromHtmlEscaped(relationNode.firstChildElement(kNameKey).text());
            relation.details
                = TextHelper::fromHtmlEscaped(relationNode.firstChildElement(kDetailsKey).text());
            d->routes.append(relation);

            relationNode = relationNode.nextSiblingElement();
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

} // namespace BusinessLayer

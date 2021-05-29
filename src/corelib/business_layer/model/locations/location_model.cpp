#include "location_model.h"

#include <business_layer/model/abstract_image_wrapper.h>

#include <domain/document_object.h>

#include <QDomDocument>


namespace BusinessLayer
{

namespace {
    const QString kDocumentKey = QStringLiteral("document");
    const QString kNameKey = QStringLiteral("name");
    const QString kStoryRoleKey = QStringLiteral("story_role");
    const QString kOneSentenceDescriptionKey = QStringLiteral("one_sentence_description");
    const QString kLongDescriptionKey = QStringLiteral("long_description");
    const QString kMainPhotoKey = QStringLiteral("main_photo");
}

class LocationModel::Implementation
{
public:
    QString name;
    int storyRole = 3;
    QString oneSentenceDescription;
    QString longDescription;
    Domain::DocumentImage mainPhoto;
};


// ****


LocationModel::LocationModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kNameKey },
                    _parent),
      d(new Implementation)
{
    connect(this, &LocationModel::nameChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::storyRoleChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::oneSentenceDescriptionChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::longDescriptionChanged, this, &LocationModel::updateDocumentContent);
    connect(this, &LocationModel::mainPhotoChanged, this, &LocationModel::updateDocumentContent);
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

int LocationModel::storyRole() const
{
    return d->storyRole;
}

void LocationModel::setStoryRole(int _role)
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

void LocationModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto contains = [&documentNode] (const QString& _key) {
        return !documentNode.firstChildElement(_key).isNull();
    };
    auto load = [&documentNode] (const QString& _key) {
        return documentNode.firstChildElement(_key).text();
    };
    d->name = load(kNameKey);
    if (contains(kStoryRoleKey)) {
        d->storyRole = load(kStoryRoleKey).toInt();
    }
    d->oneSentenceDescription = load(kOneSentenceDescriptionKey);
    d->longDescription = load(kLongDescriptionKey);
    d->mainPhoto.uuid = load(kMainPhotoKey);
    d->mainPhoto.image = imageWrapper()->load(d->mainPhoto.uuid);
}

void LocationModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    setName({});
    setOneSentenceDescription({});
    setLongDescription({});
    setMainPhoto({});
}

QByteArray LocationModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n").arg(kDocumentKey, Domain::mimeTypeFor(document()->type())).toUtf8();
    auto save = [&xml] (const QString& _key, const QString& _value) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n").arg(_key, _value).toUtf8();
    };
    save(kNameKey, d->name);
    save(kStoryRoleKey, QString::number(d->storyRole));
    save(kOneSentenceDescriptionKey, d->oneSentenceDescription);
    save(kLongDescriptionKey, d->longDescription);
    save(kMainPhotoKey, d->mainPhoto.uuid.toString());
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer

#include "character_model.h"

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
    //
    const QString kAgeKey = QStringLiteral("age");
    const QString kGenderKey = QStringLiteral("gender");
}

class CharacterModel::Implementation
{
public:
    QString name;
    int storyRole = 3;
    QString oneSentenceDescription;
    QString longDescription;
    Domain::DocumentImage mainPhoto;
    //
    QString age;
    QString gender;
};


// ****


CharacterModel::CharacterModel(QObject* _parent)
    : AbstractModel({ kDocumentKey, kNameKey },
                    _parent),
      d(new Implementation)
{
    connect(this, &CharacterModel::nameChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::storyRoleChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::oneSentenceDescriptionChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::longDescriptionChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::mainPhotoChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::ageChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::genderChanged, this, &CharacterModel::updateDocumentContent);
}

CharacterModel::~CharacterModel() = default;

const QString& CharacterModel::name() const
{
    return d->name;
}

void CharacterModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    const auto oldName = d->name;
    d->name = _name;
    emit nameChanged(d->name, oldName);
    emit documentNameChanged(d->name);
}

void CharacterModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

int CharacterModel::storyRole() const
{
    return d->storyRole;
}

void CharacterModel::setStoryRole(int _role)
{
    if (d->storyRole == _role) {
        return;
    }

    d->storyRole = _role;
    emit storyRoleChanged(d->storyRole);
}

QString CharacterModel::oneSentenceDescription() const
{
    return d->oneSentenceDescription;
}

void CharacterModel::setOneSentenceDescription(const QString& _text)
{
    if (d->oneSentenceDescription == _text) {
        return;
    }

    d->oneSentenceDescription = _text;
    emit oneSentenceDescriptionChanged(d->oneSentenceDescription);
}

QString CharacterModel::longDescription() const
{
    return d->longDescription;
}

void CharacterModel::setLongDescription(const QString& _text)
{
    if (d->longDescription == _text) {
        return;
    }

    d->longDescription = _text;
    emit longDescriptionChanged(d->longDescription);
}

const QPixmap& CharacterModel::mainPhoto() const
{
    return d->mainPhoto.image;
}

void CharacterModel::setMainPhoto(const QPixmap& _photo)
{
    d->mainPhoto.image = _photo;
    d->mainPhoto.uuid = imageWrapper()->save(d->mainPhoto.image);
    emit mainPhotoChanged(d->mainPhoto.image);
}

const QString& CharacterModel::age() const
{
    return d->age;
}

void CharacterModel::setAge(const QString& _text)
{
    if (d->age == _text) {
        return;
    }

    d->age = _text;
    emit ageChanged(d->age);
}

const QString& CharacterModel::gender() const
{
    return d->gender;
}

void CharacterModel::setGender(const QString& _text)
{
    if (d->gender == _text) {
        return;
    }

    d->gender = _text;
    emit genderChanged(d->gender);
}

void CharacterModel::initDocument()
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
    d->age = load(kAgeKey);
    d->gender = load(kGenderKey);
}

void CharacterModel::clearDocument()
{
    QSignalBlocker signalBlocker(this);

    setName({});
    setOneSentenceDescription({});
    setLongDescription({});
    setMainPhoto({});
    //
    setAge({});
    setGender({});
}

QByteArray CharacterModel::toXml() const
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
    save(kAgeKey, d->age);
    save(kGenderKey, d->gender);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer

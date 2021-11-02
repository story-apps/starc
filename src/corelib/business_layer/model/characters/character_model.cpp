#include "character_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QString kDocumentKey = QStringLiteral("document");
const QString kNameKey = QStringLiteral("name");
const QString kColorKey = QStringLiteral("color");
const QString kStoryRoleKey = QStringLiteral("story_role");
const QString kAgeKey = QStringLiteral("age");
const QString kGenderKey = QStringLiteral("gender");
const QString kOneSentenceDescriptionKey = QStringLiteral("one_sentence_description");
const QString kLongDescriptionKey = QStringLiteral("long_description");
const QString kMainPhotoKey = QStringLiteral("main_photo");
} // namespace

class CharacterModel::Implementation
{
public:
    QString name;
    QColor color;
    CharacterStoryRole storyRole = CharacterStoryRole::Undefined;
    QString age;
    int gender = 3;
    QString oneSentenceDescription;
    QString longDescription;
    Domain::DocumentImage mainPhoto;
};

// ****

CharacterModel::CharacterModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kNameKey,
            kColorKey,
            kStoryRoleKey,
            kOneSentenceDescriptionKey,
            kLongDescriptionKey,
            kMainPhotoKey,
            kAgeKey,
            kGenderKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &CharacterModel::nameChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::colorChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::storyRoleChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::oneSentenceDescriptionChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::longDescriptionChanged, this,
            &CharacterModel::updateDocumentContent);
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

QColor CharacterModel::color() const
{
    return d->color;
}

void CharacterModel::setColor(const QColor& _color)
{
    if (d->color == _color) {
        return;
    }

    d->color = _color;
    emit colorChanged(d->color);
    emit documentColorChanged(d->color);
}

CharacterStoryRole CharacterModel::storyRole() const
{
    return d->storyRole;
}

void CharacterModel::setStoryRole(CharacterStoryRole _role)
{
    if (d->storyRole == _role) {
        return;
    }

    d->storyRole = _role;
    emit storyRoleChanged(d->storyRole);
}

const QString& CharacterModel::age() const
{
    return d->age;
}

void CharacterModel::setAge(const QString& _age)
{
    if (d->age == _age) {
        return;
    }

    d->age = _age;
    emit ageChanged(d->age);
}

int CharacterModel::gender() const
{
    return d->gender;
}

void CharacterModel::setGender(int _gender)
{
    if (d->gender == _gender) {
        return;
    }

    d->gender = _gender;
    emit genderChanged(d->gender);
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

void CharacterModel::initDocument()
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
    if (contains(kColorKey)) {
        d->color = load(kColorKey);
    }
    if (contains(kStoryRoleKey)) {
        d->storyRole = static_cast<CharacterStoryRole>(load(kStoryRoleKey).toInt());
    }
    d->age = load(kAgeKey);
    if (contains(kGenderKey)) {
        d->gender = load(kGenderKey).toInt();
    }
    d->oneSentenceDescription = load(kOneSentenceDescriptionKey);
    d->longDescription = load(kLongDescriptionKey);
    d->mainPhoto.uuid = load(kMainPhotoKey);
    d->mainPhoto.image = imageWrapper()->load(d->mainPhoto.uuid);
}

void CharacterModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray CharacterModel::toXml() const
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
    if (d->color.isValid()) {
        save(kColorKey, d->color.name());
    }
    save(kStoryRoleKey, QString::number(static_cast<int>(d->storyRole)));
    save(kAgeKey, d->age);
    save(kGenderKey, QString::number(d->gender));
    save(kOneSentenceDescriptionKey, d->oneSentenceDescription);
    save(kLongDescriptionKey, d->longDescription);
    save(kMainPhotoKey, d->mainPhoto.uuid.toString());
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer

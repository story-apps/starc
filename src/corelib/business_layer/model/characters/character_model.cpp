#include "character_model.h"

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
const QLatin1String kAgeKey("age");
const QLatin1String kGenderKey("gender");
const QLatin1String kOneSentenceDescriptionKey("one_sentence_description");
const QLatin1String kLongDescriptionKey("long_description");
const QLatin1String kMainPhotoKey("main_photo");
const QLatin1String kRoutesKey("relations");
const QLatin1String kRouteKey("relation");
const QLatin1String kRelationWithCharacterKey("with");
const QLatin1String kLineTypeKey("line_type");
const QLatin1String kFeelingKey("feeling");
const QLatin1String kDetailsKey("details");
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
    QVector<CharacterRelation> relations;
};


// ****


bool CharacterRelation::isValid() const
{
    return !character.isNull();
}

bool CharacterRelation::operator==(const CharacterRelation& _other) const
{
    return character == _other.character && lineType == _other.lineType && color == _other.color
        && feeling == _other.feeling && details == _other.details;
}

bool CharacterRelation::operator!=(const CharacterRelation& _other) const
{
    return !(*this == _other);
}

CharacterModel::CharacterModel(QObject* _parent)
    : AbstractModel(
        {
            kDocumentKey,
            kNameKey,
            kColorKey,
            kStoryRoleKey,
            kAgeKey,
            kGenderKey,
            kOneSentenceDescriptionKey,
            kLongDescriptionKey,
            kMainPhotoKey,
            kRoutesKey,
            kRouteKey,
        },
        _parent)
    , d(new Implementation)
{
    connect(this, &CharacterModel::nameChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::colorChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::storyRoleChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::ageChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::genderChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::oneSentenceDescriptionChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::longDescriptionChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::mainPhotoChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::relationAdded, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::relationChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::relationRemoved, this, &CharacterModel::updateDocumentContent);
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

void CharacterModel::createRelation(const QUuid& _withCharacter)
{
    for (const auto& relation : std::as_const(d->relations)) {
        if (relation.character == _withCharacter) {
            return;
        }
    }

    d->relations.append({ _withCharacter });
    emit relationAdded(d->relations.constLast());
}

void CharacterModel::updateRelation(const CharacterRelation& _relation)
{
    for (auto& relation : d->relations) {
        if (relation.character != _relation.character) {
            continue;
        }

        if (relation != _relation) {
            relation = _relation;
            emit relationChanged(relation);
        }
        return;
    }
}

void CharacterModel::removeRelationWith(QUuid _character)
{
    for (int index = 0; index < d->relations.size(); ++index) {
        if (d->relations[index].character != _character) {
            continue;
        }

        auto relation = d->relations.takeAt(index);
        emit relationRemoved(relation);
        return;
    }
}

CharacterRelation CharacterModel::relation(const QUuid& _withCharacter)
{
    for (auto& relation : d->relations) {
        if (relation.character == _withCharacter) {
            return relation;
        }
    }

    return {};
}

CharacterRelation CharacterModel::relation(CharacterModel* _withCharacter)
{
    return relation(_withCharacter->document()->uuid());
}

QVector<CharacterRelation> CharacterModel::relations() const
{
    return d->relations;
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
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
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
    d->mainPhoto.uuid = QUuid::fromString(load(kMainPhotoKey));
    d->mainPhoto.image = imageWrapper()->load(d->mainPhoto.uuid);
    auto relationsNode = documentNode.firstChildElement(kRoutesKey);
    if (!relationsNode.isNull()) {
        auto relationNode = relationsNode.firstChildElement(kRouteKey);
        while (!relationNode.isNull()) {
            CharacterRelation relation;
            relation.character = QUuid::fromString(
                relationNode.firstChildElement(kRelationWithCharacterKey).text());
            relation.lineType = relationNode.firstChildElement(kLineTypeKey).text().toInt();
            relation.color
                = ColorHelper::fromString(relationNode.firstChildElement(kColorKey).text());
            relation.feeling
                = TextHelper::fromHtmlEscaped(relationNode.firstChildElement(kFeelingKey).text());
            relation.details
                = TextHelper::fromHtmlEscaped(relationNode.firstChildElement(kDetailsKey).text());
            d->relations.append(relation);

            relationNode = relationNode.nextSiblingElement();
        }
    }
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
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(_key, TextHelper::toHtmlEscaped(_value))
                   .toUtf8();
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
    if (!d->relations.isEmpty()) {
        xml += QString("<%1>\n").arg(kRoutesKey).toUtf8();
        for (const auto& relation : std::as_const(d->relations)) {
            xml += QString("<%1>\n").arg(kRouteKey).toUtf8();
            save(kRelationWithCharacterKey, relation.character.toString());
            save(kLineTypeKey, QString::number(relation.lineType));
            if (relation.color.isValid()) {
                save(kColorKey, relation.color.name());
            }
            save(kFeelingKey, relation.feeling);
            save(kDetailsKey, relation.details);
            xml += QString("</%1>\n").arg(kRouteKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kRoutesKey).toUtf8();
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer

#include "character_model.h"

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
const QLatin1String kAgeKey("age");
const QLatin1String kGenderKey("gender");
const QLatin1String kOneSentenceDescriptionKey("one_sentence_description");
const QLatin1String kLongDescriptionKey("long_description");
const QLatin1String kMainPhotoKey("main_photo");
const QLatin1String kPhotosKey("photos");
const QLatin1String kPhotoKey("photo");
const QLatin1String kRelationsKey("relations");
const QLatin1String kRelationKey("relation");
const QLatin1String kRelationWithCharacterKey("with");
const QLatin1String kLineTypeKey("line_type");
const QLatin1String kFeelingKey("feeling");
const QLatin1String kDetailsKey("details");

const QLatin1String kNicknameKey("nickname");
const QLatin1String kDateOfBirthKey("date_of_birth");
const QLatin1String kPlaceOfBirthKey("place_of_birth");
const QLatin1String kEthnicityKey("ethnicity");
const QLatin1String kFamilyKey("family");
const QLatin1String kHeightKey("height");
const QLatin1String kWeightKey("weight");
const QLatin1String kBodyKey("body");
const QLatin1String kSkinToneKey("skin_tone");
const QLatin1String kHairStyleKey("hair_style");
const QLatin1String kHairColorKey("hair_color");
const QLatin1String kEyeShapeKey("eye_shape");
const QLatin1String kEyeColorKey("eye_color");
const QLatin1String kFacialShapeKey("facial_shape");
const QLatin1String kDistinguishFeatureKey("distinguish_feature");
const QLatin1String kOtherFacialFeaturesKey("other_facial_features");
const QLatin1String kPostureKey("posture");
const QLatin1String kOtherPhysicalAppearanceKey("other_phisical_appearance");
const QLatin1String kSkillsKey("skills");
const QLatin1String kHowItDevelopedKey("how_it_developed");
const QLatin1String kIncompetenceKey("incompetence");
const QLatin1String kStrengthKey("strength");
const QLatin1String kWeaknessKey("weakness");
const QLatin1String kHobbiesKey("hobbies");
const QLatin1String kHabitsKey("habits");
const QLatin1String kHealthKey("health");
const QLatin1String kSpeechKey("speech");
const QLatin1String kPetKey("pet");
const QLatin1String kDressKey("dress");
const QLatin1String kSomethingAlwaysCarriedKey("something_always_carried");
const QLatin1String kAccessoriesKey("accessories");
const QLatin1String kAreaOfResidenceKey("area_of_residence");
const QLatin1String kHomeDescriptionKey("home_description");
const QLatin1String kNeighborhoodKey("neighborhood");
const QLatin1String kOrganizationInvolvedKey("organization_involved");
const QLatin1String kIncomeKey("income");
const QLatin1String kJobOccupationKey("job_occupation");
const QLatin1String kJobRankKey("job_rank");
const QLatin1String kJobSatisfactionKey("job_satisfaction");
const QLatin1String kPersonalityKey("personality");
const QLatin1String kMoralKey("moral");
const QLatin1String kMotivationKey("motivation");
const QLatin1String kDiscouragementKey("discouragement");
const QLatin1String kPhilosophyKey("philosophy");
const QLatin1String kGreatestFearKey("greatest_fear");
const QLatin1String kSelfControlKey("self_control");
const QLatin1String kIntelligenceLevelKey("intelligence_level");
const QLatin1String kConfidenceLevelKey("confidence_level");
const QLatin1String kChildhoodKey("childhood");
const QLatin1String kImportantPastEventKey("important_past_event");
const QLatin1String kBestAccomplishmentKey("best_accomplishment");
const QLatin1String kOtherAccomplishmentKey("other_accomplishment");
const QLatin1String kWorstMomentKey("worst_moment");
const QLatin1String kFailureKey("failure");
const QLatin1String kSecretsKey("secrets");
const QLatin1String kBestMemoriesKey("best_memories");
const QLatin1String kWorstMemoriesKey("worst_memories");
const QLatin1String kShortTermGoalKey("short_term_goal");
const QLatin1String kLongTermGoalKey("long_term_goal");
const QLatin1String kInitialBeliefsKey("initial_beliefs");
const QLatin1String kChangedBeliefsKey("changed_beliefs");
const QLatin1String kWhatLeadsToChangeKey("what_leads_to_change");
const QLatin1String kFirstAppearanceKey("first_appearance");
const QLatin1String kPlotInvolvementKey("plot_involvement");
const QLatin1String kConflictKey("conflict");
const QLatin1String kMostDefiningMomentKey("mostDefiningMoment");
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
    QVector<Domain::DocumentImage> photos;
    QVector<CharacterRelation> relations;

    QString nickname;
    QString dateOfBirth;
    QString placeOfBirth;
    QString ethnicity;
    QString family;
    QString height;
    QString weight;
    QString body;
    QString skinTone;
    QString hairStyle;
    QString hairColor;
    QString eyeShape;
    QString eyeColor;
    QString facialShape;
    QString distinguishFeature;
    QString otherFacialFeatures;
    QString posture;
    QString otherPhysicalAppearance;
    QString skills;
    QString howItDeveloped;
    QString incompetence;
    QString strength;
    QString weakness;
    QString hobbies;
    QString habits;
    QString health;
    QString speech;
    QString pet;
    QString dress;
    QString somethingAlwaysCarried;
    QString accessories;
    QString areaOfResidence;
    QString homeDescription;
    QString neighborhood;
    QString organizationInvolved;
    QString income;
    QString jobOccupation;
    QString jobRank;
    QString jobSatisfaction;
    QString personality;
    QString moral;
    QString motivation;
    QString discouragement;
    QString philosophy;
    QString greatestFear;
    QString selfControl;
    QString intelligenceLevel;
    QString confidenceLevel;
    QString childhood;
    QString importantPastEvent;
    QString bestAccomplishment;
    QString otherAccomplishment;
    QString worstMoment;
    QString failure;
    QString secrets;
    QString bestMemories;
    QString worstMemories;
    QString shortTermGoal;
    QString longTermGoal;
    QString initialBeliefs;
    QString changedBeliefs;
    QString whatLeadsToChange;
    QString firstAppearance;
    QString plotInvolvement;
    QString conflict;
    QString mostDefiningMoment;

    //
    // Ридонли параметры
    //

    QVector<CharacterDialogues> dialogues;
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


// ****


bool CharacterDialogues::operator==(const CharacterDialogues& _other) const
{
    return documentUuid == _other.documentUuid && documentName == _other.documentName
        && dialoguesIndexes == _other.dialoguesIndexes;
}


// ****


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
            kRelationsKey,
            kRelationKey,
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
    connect(this, &CharacterModel::photosChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::relationAdded, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::relationChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::relationRemoved, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::nicknameChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::dateOfBirthChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::placeOfBirthChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::ethnicityChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::familyChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::heightChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::weightChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::bodyChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::skinToneChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::hairStyleChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::hairColorChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::eyeShapeChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::eyeColorChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::facialShapeChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::distinguishFeatureChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::otherFacialFeaturesChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::postureChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::otherPhysicalAppearanceChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::skillsChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::howItDevelopedChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::incompetenceChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::strengthChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::weaknessChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::hobbiesChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::habitsChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::healthChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::speechChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::petChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::dressChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::somethingAlwaysCarriedChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::accessoriesChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::areaOfResidenceChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::homeDescriptionChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::neighborhoodChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::organizationInvolvedChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::incomeChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::jobOccupationChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::jobRankChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::jobSatisfactionChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::personalityChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::moralChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::motivationChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::discouragementChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::philosophyChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::greatestFearChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::selfControlChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::intelligenceLevelChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::confidenceLevelChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::childhoodChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::importantPastEventChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::bestAccomplishmentChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::otherAccomplishmentChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::worstMomentChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::failureChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::secretsChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::bestMemoriesChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::worstMemoriesChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::shortTermGoalChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::longTermGoalChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::initialBeliefsChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::changedBeliefsChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::whatLeadsToChangeChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::firstAppearanceChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::plotInvolvementChanged, this,
            &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::conflictChanged, this, &CharacterModel::updateDocumentContent);
    connect(this, &CharacterModel::mostDefiningMomentChanged, this,
            &CharacterModel::updateDocumentContent);
}

CharacterModel::~CharacterModel() = default;

QString CharacterModel::name() const
{
    return d->name;
}

void CharacterModel::setName(const QString& _name)
{
    const auto newName = _name.simplified();
    if (d->name == newName) {
        return;
    }

    const auto oldName = d->name;
    d->name = newName;

    emit documentNameChanged(d->name);
    emit nameChanged(d->name, oldName);
}

QString CharacterModel::documentName() const
{
    return name();
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

QString CharacterModel::age() const
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

Domain::DocumentImage CharacterModel::mainPhoto() const
{
    if (d->photos.isEmpty()) {
        return {};
    }

    return d->photos.constFirst();
}

void CharacterModel::setMainPhoto(const QPixmap& _photo)
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

QVector<Domain::DocumentImage> CharacterModel::photos() const
{
    return d->photos;
}

void CharacterModel::addPhoto(const Domain::DocumentImage& _photo)
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

void CharacterModel::addPhotos(const QVector<QPixmap>& _photos)
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

void CharacterModel::removePhoto(const QUuid& _photoUuid)
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

QString CharacterModel::nickname() const
{
    return d->nickname;
}
void CharacterModel::setNickname(const QString& _text)
{
    if (d->nickname == _text) {
        return;
    }
    d->nickname = _text;
    emit nicknameChanged(d->nickname);
}

QString CharacterModel::dateOfBirth() const
{
    return d->dateOfBirth;
}
void CharacterModel::setDateOfBirth(const QString& _text)
{
    if (d->dateOfBirth == _text) {
        return;
    }
    d->dateOfBirth = _text;
    emit dateOfBirthChanged(d->dateOfBirth);
}
QString CharacterModel::placeOfBirth() const
{
    return d->placeOfBirth;
}
void CharacterModel::setPlaceOfBirth(const QString& _text)
{
    if (d->placeOfBirth == _text) {
        return;
    }
    d->placeOfBirth = _text;
    emit placeOfBirthChanged(d->placeOfBirth);
}
QString CharacterModel::ethnicity() const
{
    return d->ethnicity;
}
void CharacterModel::setEthnicity(const QString& _text)
{
    if (d->ethnicity == _text) {
        return;
    }
    d->ethnicity = _text;
    emit ethnicityChanged(d->ethnicity);
}

QString CharacterModel::family() const
{
    return d->family;
}
void CharacterModel::setFamily(const QString& _text)
{
    if (d->family == _text) {
        return;
    }
    d->family = _text;
    emit familyChanged(d->family);
}
QString CharacterModel::height() const
{
    return d->height;
}
void CharacterModel::setHeight(const QString& _text)
{
    if (d->height == _text) {
        return;
    }
    d->height = _text;
    emit heightChanged(d->height);
}
QString CharacterModel::weight() const
{
    return d->weight;
}
void CharacterModel::setWeight(const QString& _text)
{
    if (d->weight == _text) {
        return;
    }
    d->weight = _text;
    emit weightChanged(d->weight);
}
QString CharacterModel::body() const
{
    return d->body;
}
void CharacterModel::setBody(const QString& _text)
{
    if (d->body == _text) {
        return;
    }
    d->body = _text;
    emit bodyChanged(d->body);
}
QString CharacterModel::skinTone() const
{
    return d->skinTone;
}
void CharacterModel::setSkinTone(const QString& _text)
{
    if (d->skinTone == _text) {
        return;
    }
    d->skinTone = _text;
    emit skinToneChanged(d->skinTone);
}
QString CharacterModel::hairStyle() const
{
    return d->hairStyle;
}
void CharacterModel::setHairStyle(const QString& _text)
{
    if (d->hairStyle == _text) {
        return;
    }
    d->hairStyle = _text;
    emit hairStyleChanged(d->hairStyle);
}
QString CharacterModel::hairColor() const
{
    return d->hairColor;
}
void CharacterModel::setHairColor(const QString& _text)
{
    if (d->hairColor == _text) {
        return;
    }
    d->hairColor = _text;
    emit hairColorChanged(d->hairColor);
}
QString CharacterModel::eyeShape() const
{
    return d->eyeShape;
}
void CharacterModel::setEyeShape(const QString& _text)
{
    if (d->eyeShape == _text) {
        return;
    }
    d->eyeShape = _text;
    emit eyeShapeChanged(d->eyeShape);
}
QString CharacterModel::eyeColor() const
{
    return d->eyeColor;
}
void CharacterModel::setEyeColor(const QString& _text)
{
    if (d->eyeColor == _text) {
        return;
    }
    d->eyeColor = _text;
    emit eyeColorChanged(d->eyeColor);
}
QString CharacterModel::facialShape() const
{
    return d->facialShape;
}
void CharacterModel::setFacialShape(const QString& _text)
{
    if (d->facialShape == _text) {
        return;
    }
    d->facialShape = _text;
    emit facialShapeChanged(d->facialShape);
}
QString CharacterModel::distinguishFeature() const
{
    return d->distinguishFeature;
}
void CharacterModel::setDistinguishFeature(const QString& _text)
{
    if (d->distinguishFeature == _text) {
        return;
    }
    d->distinguishFeature = _text;
    emit distinguishFeatureChanged(d->distinguishFeature);
}
QString CharacterModel::otherFacialFeatures() const
{
    return d->otherFacialFeatures;
}
void CharacterModel::setOtherFacialFeatures(const QString& _text)
{
    if (d->otherFacialFeatures == _text) {
        return;
    }
    d->otherFacialFeatures = _text;
    emit otherFacialFeaturesChanged(d->otherFacialFeatures);
}
QString CharacterModel::posture() const
{
    return d->posture;
}
void CharacterModel::setPosture(const QString& _text)
{
    if (d->posture == _text) {
        return;
    }
    d->posture = _text;
    emit postureChanged(d->posture);
}
QString CharacterModel::otherPhysicalAppearance() const
{
    return d->otherPhysicalAppearance;
}
void CharacterModel::setOtherPhysicalAppearance(const QString& _text)
{
    if (d->otherPhysicalAppearance == _text) {
        return;
    }
    d->otherPhysicalAppearance = _text;
    emit otherPhysicalAppearanceChanged(d->otherPhysicalAppearance);
}
QString CharacterModel::skills() const
{
    return d->skills;
}
void CharacterModel::setSkills(const QString& _text)
{
    if (d->skills == _text) {
        return;
    }
    d->skills = _text;
    emit skillsChanged(d->skills);
}
QString CharacterModel::howItDeveloped() const
{
    return d->howItDeveloped;
}
void CharacterModel::setHowItDeveloped(const QString& _text)
{
    if (d->howItDeveloped == _text) {
        return;
    }
    d->howItDeveloped = _text;
    emit howItDevelopedChanged(d->howItDeveloped);
}
QString CharacterModel::incompetence() const
{
    return d->incompetence;
}
void CharacterModel::setIncompetence(const QString& _text)
{
    if (d->incompetence == _text) {
        return;
    }
    d->incompetence = _text;
    emit incompetenceChanged(d->incompetence);
}
QString CharacterModel::strength() const
{
    return d->strength;
}
void CharacterModel::setStrength(const QString& _text)
{
    if (d->strength == _text) {
        return;
    }
    d->strength = _text;
    emit strengthChanged(d->strength);
}
QString CharacterModel::weakness() const
{
    return d->weakness;
}
void CharacterModel::setWeakness(const QString& _text)
{
    if (d->weakness == _text) {
        return;
    }
    d->weakness = _text;
    emit weaknessChanged(d->weakness);
}
QString CharacterModel::hobbies() const
{
    return d->hobbies;
}
void CharacterModel::setHobbies(const QString& _text)
{
    if (d->hobbies == _text) {
        return;
    }
    d->hobbies = _text;
    emit hobbiesChanged(d->hobbies);
}
QString CharacterModel::habits() const
{
    return d->habits;
}
void CharacterModel::setHabits(const QString& _text)
{
    if (d->habits == _text) {
        return;
    }
    d->habits = _text;
    emit habitsChanged(d->habits);
}
QString CharacterModel::health() const
{
    return d->health;
}
void CharacterModel::setHealth(const QString& _text)
{
    if (d->health == _text) {
        return;
    }
    d->health = _text;
    emit healthChanged(d->health);
}

QString CharacterModel::speech() const
{
    return d->speech;
}
void CharacterModel::setSpeech(const QString& _text)
{
    if (d->speech == _text) {
        return;
    }
    d->speech = _text;
    emit speechChanged(d->speech);
}

QString CharacterModel::pet() const
{
    return d->pet;
}
void CharacterModel::setPet(const QString& _text)
{
    if (d->pet == _text) {
        return;
    }
    d->pet = _text;
    emit petChanged(d->pet);
}
QString CharacterModel::dress() const
{
    return d->dress;
}
void CharacterModel::setDress(const QString& _text)
{
    if (d->dress == _text) {
        return;
    }
    d->dress = _text;
    emit dressChanged(d->dress);
}
QString CharacterModel::somethingAlwaysCarried() const
{
    return d->somethingAlwaysCarried;
}
void CharacterModel::setSomethingAlwaysCarried(const QString& _text)
{
    if (d->somethingAlwaysCarried == _text) {
        return;
    }
    d->somethingAlwaysCarried = _text;
    emit somethingAlwaysCarriedChanged(d->somethingAlwaysCarried);
}
QString CharacterModel::accessories() const
{
    return d->accessories;
}
void CharacterModel::setAccessories(const QString& _text)
{
    if (d->accessories == _text) {
        return;
    }
    d->accessories = _text;
    emit accessoriesChanged(d->accessories);
}
QString CharacterModel::areaOfResidence() const
{
    return d->areaOfResidence;
}
void CharacterModel::setAreaOfResidence(const QString& _text)
{
    if (d->areaOfResidence == _text) {
        return;
    }
    d->areaOfResidence = _text;
    emit areaOfResidenceChanged(d->areaOfResidence);
}
QString CharacterModel::homeDescription() const
{
    return d->homeDescription;
}
void CharacterModel::setHomeDescription(const QString& _text)
{
    if (d->homeDescription == _text) {
        return;
    }
    d->homeDescription = _text;
    emit homeDescriptionChanged(d->homeDescription);
}
QString CharacterModel::neighborhood() const
{
    return d->neighborhood;
}
void CharacterModel::setNeighborhood(const QString& _text)
{
    if (d->neighborhood == _text) {
        return;
    }
    d->neighborhood = _text;
    emit neighborhoodChanged(d->neighborhood);
}
QString CharacterModel::organizationInvolved() const
{
    return d->organizationInvolved;
}
void CharacterModel::setOrganizationInvolved(const QString& _text)
{
    if (d->organizationInvolved == _text) {
        return;
    }
    d->organizationInvolved = _text;
    emit organizationInvolvedChanged(d->organizationInvolved);
}
QString CharacterModel::income() const
{
    return d->income;
}
void CharacterModel::setIncome(const QString& _text)
{
    if (d->income == _text) {
        return;
    }
    d->income = _text;
    emit incomeChanged(d->income);
}
QString CharacterModel::jobOccupation() const
{
    return d->jobOccupation;
}
void CharacterModel::setJobOccupation(const QString& _text)
{
    if (d->jobOccupation == _text) {
        return;
    }
    d->jobOccupation = _text;
    emit jobOccupationChanged(d->jobOccupation);
}
QString CharacterModel::jobRank() const
{
    return d->jobRank;
}
void CharacterModel::setJobRank(const QString& _text)
{
    if (d->jobRank == _text) {
        return;
    }
    d->jobRank = _text;
    emit jobRankChanged(d->jobRank);
}
QString CharacterModel::jobSatisfaction() const
{
    return d->jobSatisfaction;
}
void CharacterModel::setJobSatisfaction(const QString& _text)
{
    if (d->jobSatisfaction == _text) {
        return;
    }
    d->jobSatisfaction = _text;
    emit jobSatisfactionChanged(d->jobSatisfaction);
}
QString CharacterModel::personality() const
{
    return d->personality;
}
void CharacterModel::setPersonality(const QString& _text)
{
    if (d->personality == _text) {
        return;
    }
    d->personality = _text;
    emit personalityChanged(d->personality);
}
QString CharacterModel::moral() const
{
    return d->moral;
}
void CharacterModel::setMoral(const QString& _text)
{
    if (d->moral == _text) {
        return;
    }
    d->moral = _text;
    emit moralChanged(d->moral);
}
QString CharacterModel::motivation() const
{
    return d->motivation;
}
void CharacterModel::setMotivation(const QString& _text)
{
    if (d->motivation == _text) {
        return;
    }
    d->motivation = _text;
    emit motivationChanged(d->motivation);
}
QString CharacterModel::discouragement() const
{
    return d->discouragement;
}
void CharacterModel::setDiscouragement(const QString& _text)
{
    if (d->discouragement == _text) {
        return;
    }
    d->discouragement = _text;
    emit discouragementChanged(d->discouragement);
}
QString CharacterModel::philosophy() const
{
    return d->philosophy;
}
void CharacterModel::setPhilosophy(const QString& _text)
{
    if (d->philosophy == _text) {
        return;
    }
    d->philosophy = _text;
    emit philosophyChanged(d->philosophy);
}
QString CharacterModel::greatestFear() const
{
    return d->greatestFear;
}
void CharacterModel::setGreatestFear(const QString& _text)
{
    if (d->greatestFear == _text) {
        return;
    }
    d->greatestFear = _text;
    emit greatestFearChanged(d->greatestFear);
}
QString CharacterModel::selfControl() const
{
    return d->selfControl;
}
void CharacterModel::setSelfControl(const QString& _text)
{
    if (d->selfControl == _text) {
        return;
    }
    d->selfControl = _text;
    emit selfControlChanged(d->selfControl);
}
QString CharacterModel::intelligenceLevel() const
{
    return d->intelligenceLevel;
}
void CharacterModel::setIntelligenceLevel(const QString& _text)
{
    if (d->intelligenceLevel == _text) {
        return;
    }
    d->intelligenceLevel = _text;
    emit intelligenceLevelChanged(d->intelligenceLevel);
}
QString CharacterModel::confidenceLevel() const
{
    return d->confidenceLevel;
}
void CharacterModel::setConfidenceLevel(const QString& _text)
{
    if (d->confidenceLevel == _text) {
        return;
    }
    d->confidenceLevel = _text;
    emit confidenceLevelChanged(d->confidenceLevel);
}
QString CharacterModel::childhood() const
{
    return d->childhood;
}
void CharacterModel::setChildhood(const QString& _text)
{
    if (d->childhood == _text) {
        return;
    }
    d->childhood = _text;
    emit childhoodChanged(d->childhood);
}
QString CharacterModel::importantPastEvent() const
{
    return d->importantPastEvent;
}
void CharacterModel::setImportantPastEvent(const QString& _text)
{
    if (d->importantPastEvent == _text) {
        return;
    }
    d->importantPastEvent = _text;
    emit importantPastEventChanged(d->importantPastEvent);
}
QString CharacterModel::bestAccomplishment() const
{
    return d->bestAccomplishment;
}
void CharacterModel::setBestAccomplishment(const QString& _text)
{
    if (d->bestAccomplishment == _text) {
        return;
    }
    d->bestAccomplishment = _text;
    emit bestAccomplishmentChanged(d->bestAccomplishment);
}
QString CharacterModel::otherAccomplishment() const
{
    return d->otherAccomplishment;
}
void CharacterModel::setOtherAccomplishment(const QString& _text)
{
    if (d->otherAccomplishment == _text) {
        return;
    }
    d->otherAccomplishment = _text;
    emit otherAccomplishmentChanged(d->otherAccomplishment);
}
QString CharacterModel::worstMoment() const
{
    return d->worstMoment;
}
void CharacterModel::setWorstMoment(const QString& _text)
{
    if (d->worstMoment == _text) {
        return;
    }
    d->worstMoment = _text;
    emit worstMomentChanged(d->worstMoment);
}
QString CharacterModel::failure() const
{
    return d->failure;
}
void CharacterModel::setFailure(const QString& _text)
{
    if (d->failure == _text) {
        return;
    }
    d->failure = _text;
    emit failureChanged(d->failure);
}
QString CharacterModel::secrets() const
{
    return d->secrets;
}
void CharacterModel::setSecrets(const QString& _text)
{
    if (d->secrets == _text) {
        return;
    }
    d->secrets = _text;
    emit secretsChanged(d->secrets);
}
QString CharacterModel::bestMemories() const
{
    return d->bestMemories;
}
void CharacterModel::setBestMemories(const QString& _text)
{
    if (d->bestMemories == _text) {
        return;
    }
    d->bestMemories = _text;
    emit bestMemoriesChanged(d->bestMemories);
}
QString CharacterModel::worstMemories() const
{
    return d->worstMemories;
}
void CharacterModel::setWorstMemories(const QString& _text)
{
    if (d->worstMemories == _text) {
        return;
    }
    d->worstMemories = _text;
    emit worstMemoriesChanged(d->worstMemories);
}
QString CharacterModel::shortTermGoal() const
{
    return d->shortTermGoal;
}
void CharacterModel::setShortTermGoal(const QString& _text)
{
    if (d->shortTermGoal == _text) {
        return;
    }
    d->shortTermGoal = _text;
    emit shortTermGoalChanged(d->shortTermGoal);
}
QString CharacterModel::longTermGoal() const
{
    return d->longTermGoal;
}
void CharacterModel::setLongTermGoal(const QString& _text)
{
    if (d->longTermGoal == _text) {
        return;
    }
    d->longTermGoal = _text;
    emit longTermGoalChanged(d->longTermGoal);
}

QString CharacterModel::initialBeliefs() const
{
    return d->initialBeliefs;
}

void CharacterModel::setInitialBeliefs(const QString& _text)
{
    if (d->initialBeliefs == _text) {
        return;
    }
    d->initialBeliefs = _text;
    emit initialBeliefsChanged(d->initialBeliefs);
}

QString CharacterModel::changedBeliefs() const
{
    return d->changedBeliefs;
}

void CharacterModel::setChangedBeliefs(const QString& _text)
{
    if (d->changedBeliefs == _text) {
        return;
    }
    d->changedBeliefs = _text;
    emit changedBeliefsChanged(d->changedBeliefs);
}

QString CharacterModel::whatLeadsToChange() const
{
    return d->whatLeadsToChange;
}

void CharacterModel::setWhatLeadsToChange(const QString& _text)
{
    if (d->whatLeadsToChange == _text) {
        return;
    }
    d->whatLeadsToChange = _text;
    emit whatLeadsToChangeChanged(d->whatLeadsToChange);
}
QString CharacterModel::firstAppearance() const
{
    return d->firstAppearance;
}
void CharacterModel::setFirstAppearance(const QString& _text)
{
    if (d->firstAppearance == _text) {
        return;
    }
    d->firstAppearance = _text;
    emit firstAppearanceChanged(d->firstAppearance);
}
QString CharacterModel::plotInvolvement() const
{
    return d->plotInvolvement;
}
void CharacterModel::setPlotInvolvement(const QString& _text)
{
    if (d->plotInvolvement == _text) {
        return;
    }
    d->plotInvolvement = _text;
    emit plotInvolvementChanged(d->plotInvolvement);
}
QString CharacterModel::conflict() const
{
    return d->conflict;
}
void CharacterModel::setConflict(const QString& _text)
{
    if (d->conflict == _text) {
        return;
    }
    d->conflict = _text;
    emit conflictChanged(d->conflict);
}
QString CharacterModel::mostDefiningMoment() const
{
    return d->mostDefiningMoment;
}
void CharacterModel::setMostDefiningMoment(const QString& _text)
{
    if (d->mostDefiningMoment == _text) {
        return;
    }
    d->mostDefiningMoment = _text;
    emit mostDefiningMomentChanged(d->mostDefiningMoment);
}

QVector<CharacterDialogues> CharacterModel::dialogues() const
{
    return d->dialogues;
}

void CharacterModel::setDialogues(const QVector<CharacterDialogues>& _dialogues)
{
    if (d->dialogues == _dialogues) {
        return;
    }

    d->dialogues = _dialogues;
    emit dialoguesChanged(d->dialogues);
}
void CharacterModel::updateDialogues()
{
    emit dialoguesUpdateRequested();
}
void CharacterModel::initImageWrapper()
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
    const auto relationsNode = documentNode.firstChildElement(kRelationsKey);
    if (!relationsNode.isNull()) {
        auto relationNode = relationsNode.firstChildElement(kRelationKey);
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
    d->nickname = load(kNicknameKey);
    d->dateOfBirth = load(kDateOfBirthKey);
    d->placeOfBirth = load(kPlaceOfBirthKey);
    d->ethnicity = load(kEthnicityKey);
    d->family = load(kFamilyKey);
    d->height = load(kHeightKey);
    d->weight = load(kWeightKey);
    d->body = load(kBodyKey);
    d->skinTone = load(kSkinToneKey);
    d->hairStyle = load(kHairStyleKey);
    d->hairColor = load(kHairColorKey);
    d->eyeShape = load(kEyeShapeKey);
    d->eyeColor = load(kEyeColorKey);
    d->facialShape = load(kFacialShapeKey);
    d->distinguishFeature = load(kDistinguishFeatureKey);
    d->otherFacialFeatures = load(kOtherFacialFeaturesKey);
    d->posture = load(kPostureKey);
    d->otherPhysicalAppearance = load(kOtherPhysicalAppearanceKey);
    d->skills = load(kSkillsKey);
    d->howItDeveloped = load(kHowItDevelopedKey);
    d->incompetence = load(kIncompetenceKey);
    d->strength = load(kStrengthKey);
    d->weakness = load(kWeaknessKey);
    d->hobbies = load(kHobbiesKey);
    d->habits = load(kHabitsKey);
    d->health = load(kHealthKey);
    d->speech = load(kSpeechKey);
    d->pet = load(kPetKey);
    d->dress = load(kDressKey);
    d->somethingAlwaysCarried = load(kSomethingAlwaysCarriedKey);
    d->accessories = load(kAccessoriesKey);
    d->areaOfResidence = load(kAreaOfResidenceKey);
    d->homeDescription = load(kHomeDescriptionKey);
    d->neighborhood = load(kNeighborhoodKey);
    d->organizationInvolved = load(kOrganizationInvolvedKey);
    d->income = load(kIncomeKey);
    d->jobOccupation = load(kJobOccupationKey);
    d->jobRank = load(kJobRankKey);
    d->jobSatisfaction = load(kJobSatisfactionKey);
    d->personality = load(kPersonalityKey);
    d->moral = load(kMoralKey);
    d->motivation = load(kMotivationKey);
    d->discouragement = load(kDiscouragementKey);
    d->philosophy = load(kPhilosophyKey);
    d->greatestFear = load(kGreatestFearKey);
    d->selfControl = load(kSelfControlKey);
    d->intelligenceLevel = load(kIntelligenceLevelKey);
    d->confidenceLevel = load(kConfidenceLevelKey);
    d->childhood = load(kChildhoodKey);
    d->importantPastEvent = load(kImportantPastEventKey);
    d->bestAccomplishment = load(kBestAccomplishmentKey);
    d->otherAccomplishment = load(kOtherAccomplishmentKey);
    d->worstMoment = load(kWorstMomentKey);
    d->failure = load(kFailureKey);
    d->secrets = load(kSecretsKey);
    d->bestMemories = load(kBestMemoriesKey);
    d->worstMemories = load(kWorstMemoriesKey);
    d->shortTermGoal = load(kShortTermGoalKey);
    d->longTermGoal = load(kLongTermGoalKey);
    d->initialBeliefs = load(kInitialBeliefsKey);
    d->changedBeliefs = load(kChangedBeliefsKey);
    d->whatLeadsToChange = load(kWhatLeadsToChangeKey);
    d->firstAppearance = load(kFirstAppearanceKey);
    d->plotInvolvement = load(kPlotInvolvementKey);
    d->conflict = load(kConflictKey);
    d->mostDefiningMoment = load(kMostDefiningMomentKey);
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
    if (!d->photos.isEmpty()) {
        xml += QString("<%1>\n").arg(kPhotosKey).toUtf8();
        for (const auto& photo : std::as_const(d->photos)) {
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(photo.uuid.toString()))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(kPhotosKey).toUtf8();
    }
    if (!d->relations.isEmpty()) {
        xml += QString("<%1>\n").arg(kRelationsKey).toUtf8();
        for (const auto& relation : std::as_const(d->relations)) {
            xml += QString("<%1>\n").arg(kRelationKey).toUtf8();
            save(kRelationWithCharacterKey, relation.character.toString());
            save(kLineTypeKey, QString::number(relation.lineType));
            if (relation.color.isValid()) {
                save(kColorKey, relation.color.name());
            }
            save(kFeelingKey, relation.feeling);
            save(kDetailsKey, relation.details);
            xml += QString("</%1>\n").arg(kRelationKey).toUtf8();
        }
        xml += QString("</%1>\n").arg(kRelationsKey).toUtf8();
    }
    save(kNicknameKey, d->nickname);
    save(kDateOfBirthKey, d->dateOfBirth);
    save(kPlaceOfBirthKey, d->placeOfBirth);
    save(kEthnicityKey, d->ethnicity);
    save(kFamilyKey, d->family);
    save(kHeightKey, d->height);
    save(kWeightKey, d->weight);
    save(kBodyKey, d->body);
    save(kSkinToneKey, d->skinTone);
    save(kHairStyleKey, d->hairStyle);
    save(kHairColorKey, d->hairColor);
    save(kEyeShapeKey, d->eyeShape);
    save(kEyeColorKey, d->eyeColor);
    save(kFacialShapeKey, d->facialShape);
    save(kDistinguishFeatureKey, d->distinguishFeature);
    save(kOtherFacialFeaturesKey, d->otherFacialFeatures);
    save(kPostureKey, d->posture);
    save(kOtherPhysicalAppearanceKey, d->otherPhysicalAppearance);
    save(kSkillsKey, d->skills);
    save(kHowItDevelopedKey, d->howItDeveloped);
    save(kIncompetenceKey, d->incompetence);
    save(kStrengthKey, d->strength);
    save(kWeaknessKey, d->weakness);
    save(kHobbiesKey, d->hobbies);
    save(kHabitsKey, d->habits);
    save(kHealthKey, d->health);
    save(kSpeechKey, d->speech);
    save(kPetKey, d->pet);
    save(kDressKey, d->dress);
    save(kSomethingAlwaysCarriedKey, d->somethingAlwaysCarried);
    save(kAccessoriesKey, d->accessories);
    save(kAreaOfResidenceKey, d->areaOfResidence);
    save(kHomeDescriptionKey, d->homeDescription);
    save(kNeighborhoodKey, d->neighborhood);
    save(kOrganizationInvolvedKey, d->organizationInvolved);
    save(kIncomeKey, d->income);
    save(kJobOccupationKey, d->jobOccupation);
    save(kJobRankKey, d->jobRank);
    save(kJobSatisfactionKey, d->jobSatisfaction);
    save(kPersonalityKey, d->personality);
    save(kMoralKey, d->moral);
    save(kMotivationKey, d->motivation);
    save(kDiscouragementKey, d->discouragement);
    save(kPhilosophyKey, d->philosophy);
    save(kGreatestFearKey, d->greatestFear);
    save(kSelfControlKey, d->selfControl);
    save(kIntelligenceLevelKey, d->intelligenceLevel);
    save(kConfidenceLevelKey, d->confidenceLevel);
    save(kChildhoodKey, d->childhood);
    save(kImportantPastEventKey, d->importantPastEvent);
    save(kBestAccomplishmentKey, d->bestAccomplishment);
    save(kOtherAccomplishmentKey, d->otherAccomplishment);
    save(kWorstMomentKey, d->worstMoment);
    save(kFailureKey, d->failure);
    save(kSecretsKey, d->secrets);
    save(kBestMemoriesKey, d->bestMemories);
    save(kWorstMemoriesKey, d->worstMemories);
    save(kShortTermGoalKey, d->shortTermGoal);
    save(kLongTermGoalKey, d->longTermGoal);
    save(kInitialBeliefsKey, d->initialBeliefs);
    save(kChangedBeliefsKey, d->changedBeliefs);
    save(kWhatLeadsToChangeKey, d->whatLeadsToChange);
    save(kFirstAppearanceKey, d->firstAppearance);
    save(kPlotInvolvementKey, d->plotInvolvement);
    save(kConflictKey, d->conflict);
    save(kMostDefiningMomentKey, d->mostDefiningMoment);
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor CharacterModel::applyPatch(const QByteArray& _patch)
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
    if (contains(kColorKey)) {
        setColor(load(kColorKey));
    }
    if (contains(kStoryRoleKey)) {
        setStoryRole(static_cast<CharacterStoryRole>(load(kStoryRoleKey).toInt()));
    }
    setAge(load(kAgeKey));
    if (contains(kGenderKey)) {
        setGender(load(kGenderKey).toInt());
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
    auto relationsNode = documentNode.firstChildElement(kRelationsKey);
    QVector<CharacterRelation> newRelations;
    if (!relationsNode.isNull()) {
        auto relationNode = relationsNode.firstChildElement(kRelationKey);
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
            newRelations.append(relation);

            relationNode = relationNode.nextSiblingElement();
        }
    }
    //
    // ... корректируем текущие отношения персонажа
    //
    for (int relationIndex = 0; relationIndex < d->relations.size(); ++relationIndex) {
        const auto& relation = d->relations.at(relationIndex);
        //
        // ... если такое отношение осталось актуальным, то оставим его в списке текущих
        //     и удалим из списка новых
        //
        if (newRelations.contains(relation)) {
            newRelations.removeAll(relation);
        }
        //
        // ... если такого отношения нет в списке новых, то удалим его из списка текущих
        //
        else {
            removeRelationWith(relation.character);
            --relationIndex;
        }
    }
    //
    // ... добавляем новые отношения к персонажу
    //
    for (const auto& relation : newRelations) {
        createRelation(relation.character);
        updateRelation(relation);
    }
    //
    setNickname(load(kNicknameKey));
    setDateOfBirth(load(kDateOfBirthKey));
    setPlaceOfBirth(load(kPlaceOfBirthKey));
    setEthnicity(load(kEthnicityKey));
    setFamily(load(kFamilyKey));
    setHeight(load(kHeightKey));
    setWeight(load(kWeightKey));
    setBody(load(kBodyKey));
    setSkinTone(load(kSkinToneKey));
    setHairStyle(load(kHairStyleKey));
    setHairColor(load(kHairColorKey));
    setEyeShape(load(kEyeShapeKey));
    setEyeColor(load(kEyeColorKey));
    setFacialShape(load(kFacialShapeKey));
    setDistinguishFeature(load(kDistinguishFeatureKey));
    setOtherFacialFeatures(load(kOtherFacialFeaturesKey));
    setPosture(load(kPostureKey));
    setOtherPhysicalAppearance(load(kOtherPhysicalAppearanceKey));
    setSkills(load(kSkillsKey));
    setHowItDeveloped(load(kHowItDevelopedKey));
    setIncompetence(load(kIncompetenceKey));
    setStrength(load(kStrengthKey));
    setWeakness(load(kWeaknessKey));
    setHobbies(load(kHobbiesKey));
    setHabits(load(kHabitsKey));
    setHealth(load(kHealthKey));
    setSpeech(load(kSpeechKey));
    setPet(load(kPetKey));
    setDress(load(kDressKey));
    setSomethingAlwaysCarried(load(kSomethingAlwaysCarriedKey));
    setAccessories(load(kAccessoriesKey));
    setAreaOfResidence(load(kAreaOfResidenceKey));
    setHomeDescription(load(kHomeDescriptionKey));
    setNeighborhood(load(kNeighborhoodKey));
    setOrganizationInvolved(load(kOrganizationInvolvedKey));
    setIncome(load(kIncomeKey));
    setJobOccupation(load(kJobOccupationKey));
    setJobRank(load(kJobRankKey));
    setJobSatisfaction(load(kJobSatisfactionKey));
    setPersonality(load(kPersonalityKey));
    setMoral(load(kMoralKey));
    setMotivation(load(kMotivationKey));
    setDiscouragement(load(kDiscouragementKey));
    setPhilosophy(load(kPhilosophyKey));
    setGreatestFear(load(kGreatestFearKey));
    setSelfControl(load(kSelfControlKey));
    setIntelligenceLevel(load(kIntelligenceLevelKey));
    setConfidenceLevel(load(kConfidenceLevelKey));
    setChildhood(load(kChildhoodKey));
    setImportantPastEvent(load(kImportantPastEventKey));
    setBestAccomplishment(load(kBestAccomplishmentKey));
    setOtherAccomplishment(load(kOtherAccomplishmentKey));
    setWorstMoment(load(kWorstMomentKey));
    setFailure(load(kFailureKey));
    setSecrets(load(kSecretsKey));
    setBestMemories(load(kBestMemoriesKey));
    setWorstMemories(load(kWorstMemoriesKey));
    setShortTermGoal(load(kShortTermGoalKey));
    setLongTermGoal(load(kLongTermGoalKey));
    setInitialBeliefs(load(kInitialBeliefsKey));
    setChangedBeliefs(load(kChangedBeliefsKey));
    setWhatLeadsToChange(load(kWhatLeadsToChangeKey));
    setFirstAppearance(load(kFirstAppearanceKey));
    setPlotInvolvement(load(kPlotInvolvementKey));
    setConflict(load(kConflictKey));
    setMostDefiningMoment(load(kMostDefiningMomentKey));

    return {};
}

} // namespace BusinessLayer

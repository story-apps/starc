#pragma once

#include "../abstract_model.h"

#include <QColor>
#include <QUuid>

namespace Domain {
struct DocumentImage;
}


namespace BusinessLayer {

/**
 * @brief Роль персонажа в истории
 */
enum class CharacterStoryRole {
    Primary,
    Secondary,
    Tertiary,
    Undefined,
};

/**
 * @brief Отношения с другим персонажем
 */
class CORE_LIBRARY_EXPORT CharacterRelation
{
public:
    bool isValid() const;

    bool operator==(const CharacterRelation& _other) const;
    bool operator!=(const CharacterRelation& _other) const;

    QUuid character;
    int lineType = Qt::SolidLine;
    QColor color = {};
    QString feeling = {};
    QString details = {};
};


class CORE_LIBRARY_EXPORT CharacterDialogues
{
public:
    bool operator==(const CharacterDialogues& _other) const;

    QUuid documentUuid;
    QString documentName;
    QVector<QModelIndex> dialoguesIndexes;
};

/**
 * @brief Модель данных персонажа
 */
class CORE_LIBRARY_EXPORT CharacterModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit CharacterModel(QObject* _parent = nullptr);
    ~CharacterModel() override;

    QString name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _newName, const QString& _oldName);
    void setDocumentName(const QString& _name) override;

    QColor color() const;
    void setColor(const QColor& _color);
    Q_SIGNAL void colorChanged(const QColor& _color);

    CharacterStoryRole storyRole() const;
    void setStoryRole(CharacterStoryRole _role);
    Q_SIGNAL void storyRoleChanged(BusinessLayer::CharacterStoryRole _role);

    QString age() const;
    void setAge(const QString& _age);
    Q_SIGNAL void ageChanged(const QString& _age);

    int gender() const;
    void setGender(int _gender);
    Q_SIGNAL void genderChanged(int _gender);

    QString oneSentenceDescription() const;
    void setOneSentenceDescription(const QString& _text);
    Q_SIGNAL void oneSentenceDescriptionChanged(const QString& _text);

    QString longDescription() const;
    void setLongDescription(const QString& _text);
    Q_SIGNAL void longDescriptionChanged(const QString& _text);

    Domain::DocumentImage mainPhoto() const;
    void setMainPhoto(const QPixmap& _photo);
    Q_SIGNAL void mainPhotoChanged(const Domain::DocumentImage& _photo);

    QVector<Domain::DocumentImage> photos() const;
    void addPhoto(const Domain::DocumentImage& _photo);
    void addPhotos(const QVector<QPixmap>& _photos);
    void removePhoto(const QUuid& _photoUuid);
    Q_SIGNAL void photosChanged(const QVector<Domain::DocumentImage>& _images);

    void createRelation(const QUuid& _withCharacter);
    void updateRelation(const CharacterRelation& _relation);
    void removeRelationWith(QUuid _character);
    CharacterRelation relation(const QUuid& _withCharacter);
    CharacterRelation relation(CharacterModel* _withCharacter);
    QVector<CharacterRelation> relations() const;
    Q_SIGNAL void relationAdded(const BusinessLayer::CharacterRelation& _relation);
    Q_SIGNAL void relationChanged(const BusinessLayer::CharacterRelation& _relation);
    Q_SIGNAL void relationRemoved(const BusinessLayer::CharacterRelation& _relation);

    //
    // BASIC
    //

    QString nickname() const;
    void setNickname(const QString& _text);
    Q_SIGNAL void nicknameChanged(const QString& _text);

    QString dateOfBirth() const;
    void setDateOfBirth(const QString& _text);
    Q_SIGNAL void dateOfBirthChanged(const QString& _text);

    QString placeOfBirth() const;
    void setPlaceOfBirth(const QString& _text);
    Q_SIGNAL void placeOfBirthChanged(const QString& _text);

    QString ethnicity() const;
    void setEthnicity(const QString& _text);
    Q_SIGNAL void ethnicityChanged(const QString& _text);

    QString family() const;
    void setFamily(const QString& _text);
    Q_SIGNAL void familyChanged(const QString& _text);

    //
    // PHISIQUE
    //

    QString height() const;
    void setHeight(const QString& _text);
    Q_SIGNAL void heightChanged(const QString& _text);

    QString weight() const;
    void setWeight(const QString& _text);
    Q_SIGNAL void weightChanged(const QString& _text);

    QString body() const;
    void setBody(const QString& _text);
    Q_SIGNAL void bodyChanged(const QString& _text);

    QString skinTone() const;
    void setSkinTone(const QString& _text);
    Q_SIGNAL void skinToneChanged(const QString& _text);

    QString hairStyle() const;
    void setHairStyle(const QString& _text);
    Q_SIGNAL void hairStyleChanged(const QString& _text);

    QString hairColor() const;
    void setHairColor(const QString& _text);
    Q_SIGNAL void hairColorChanged(const QString& _text);

    QString eyeShape() const;
    void setEyeShape(const QString& _text);
    Q_SIGNAL void eyeShapeChanged(const QString& _text);

    QString eyeColor() const;
    void setEyeColor(const QString& _text);
    Q_SIGNAL void eyeColorChanged(const QString& _text);

    QString facialShape() const;
    void setFacialShape(const QString& _text);
    Q_SIGNAL void facialShapeChanged(const QString& _text);

    QString distinguishFeature() const;
    void setDistinguishFeature(const QString& _text);
    Q_SIGNAL void distinguishFeatureChanged(const QString& _text);

    QString otherFacialFeatures() const;
    void setOtherFacialFeatures(const QString& _text);
    Q_SIGNAL void otherFacialFeaturesChanged(const QString& _text);

    QString posture() const;
    void setPosture(const QString& _text);
    Q_SIGNAL void postureChanged(const QString& _text);

    QString otherPhysicalAppearance() const;
    void setOtherPhysicalAppearance(const QString& _text);
    Q_SIGNAL void otherPhysicalAppearanceChanged(const QString& _text);

    //
    // LIFE
    //

    QString skills() const;
    void setSkills(const QString& _text);
    Q_SIGNAL void skillsChanged(const QString& _text);

    QString howItDeveloped() const;
    void setHowItDeveloped(const QString& _text);
    Q_SIGNAL void howItDevelopedChanged(const QString& _text);

    QString incompetence() const;
    void setIncompetence(const QString& _text);
    Q_SIGNAL void incompetenceChanged(const QString& _text);

    QString strength() const;
    void setStrength(const QString& _text);
    Q_SIGNAL void strengthChanged(const QString& _text);

    QString weakness() const;
    void setWeakness(const QString& _text);
    Q_SIGNAL void weaknessChanged(const QString& _text);

    QString hobbies() const;
    void setHobbies(const QString& _text);
    Q_SIGNAL void hobbiesChanged(const QString& _text);

    QString habits() const;
    void setHabits(const QString& _text);
    Q_SIGNAL void habitsChanged(const QString& _text);

    QString health() const;
    void setHealth(const QString& _text);
    Q_SIGNAL void healthChanged(const QString& _text);

    QString pet() const;
    void setPet(const QString& _text);
    Q_SIGNAL void petChanged(const QString& _text);

    QString dress() const;
    void setDress(const QString& _text);
    Q_SIGNAL void dressChanged(const QString& _text);

    QString somethingAlwaysCarried() const;
    void setSomethingAlwaysCarried(const QString& _text);
    Q_SIGNAL void somethingAlwaysCarriedChanged(const QString& _text);

    QString accessories() const;
    void setAccessories(const QString& _text);
    Q_SIGNAL void accessoriesChanged(const QString& _text);

    QString areaOfResidence() const;
    void setAreaOfResidence(const QString& _text);
    Q_SIGNAL void areaOfResidenceChanged(const QString& _text);

    QString homeDescription() const;
    void setHomeDescription(const QString& _text);
    Q_SIGNAL void homeDescriptionChanged(const QString& _text);

    QString neighborhood() const;
    void setNeighborhood(const QString& _text);
    Q_SIGNAL void neighborhoodChanged(const QString& _text);

    QString organizationInvolved() const;
    void setOrganizationInvolved(const QString& _text);
    Q_SIGNAL void organizationInvolvedChanged(const QString& _text);

    QString income() const;
    void setIncome(const QString& _text);
    Q_SIGNAL void incomeChanged(const QString& _text);

    QString jobOccupation() const;
    void setJobOccupation(const QString& _text);
    Q_SIGNAL void jobOccupationChanged(const QString& _text);

    QString jobRank() const;
    void setJobRank(const QString& _text);
    Q_SIGNAL void jobRankChanged(const QString& _text);

    QString jobSatisfaction() const;
    void setJobSatisfaction(const QString& _text);
    Q_SIGNAL void jobSatisfactionChanged(const QString& _text);

    //
    // ATTITUDE
    //

    QString personality() const;
    void setPersonality(const QString& _text);
    Q_SIGNAL void personalityChanged(const QString& _text);

    QString moral() const;
    void setMoral(const QString& _text);
    Q_SIGNAL void moralChanged(const QString& _text);

    QString motivation() const;
    void setMotivation(const QString& _text);
    Q_SIGNAL void motivationChanged(const QString& _text);

    QString discouragement() const;
    void setDiscouragement(const QString& _text);
    Q_SIGNAL void discouragementChanged(const QString& _text);

    QString philosophy() const;
    void setPhilosophy(const QString& _text);
    Q_SIGNAL void philosophyChanged(const QString& _text);

    QString greatestFear() const;
    void setGreatestFear(const QString& _text);
    Q_SIGNAL void greatestFearChanged(const QString& _text);

    QString selfControl() const;
    void setSelfControl(const QString& _text);
    Q_SIGNAL void selfControlChanged(const QString& _text);

    QString intelligenceLevel() const;
    void setIntelligenceLevel(const QString& _text);
    Q_SIGNAL void intelligenceLevelChanged(const QString& _text);

    QString confidenceLevel() const;
    void setConfidenceLevel(const QString& _text);
    Q_SIGNAL void confidenceLevelChanged(const QString& _text);

    //
    // BIOGRAPHY
    //

    QString childhood() const;
    void setChildhood(const QString& _text);
    Q_SIGNAL void childhoodChanged(const QString& _text);

    QString importantPastEvent() const;
    void setImportantPastEvent(const QString& _text);
    Q_SIGNAL void importantPastEventChanged(const QString& _text);

    QString bestAccomplishment() const;
    void setBestAccomplishment(const QString& _text);
    Q_SIGNAL void bestAccomplishmentChanged(const QString& _text);

    QString otherAccomplishment() const;
    void setOtherAccomplishment(const QString& _text);
    Q_SIGNAL void otherAccomplishmentChanged(const QString& _text);

    QString worstMoment() const;
    void setWorstMoment(const QString& _text);
    Q_SIGNAL void worstMomentChanged(const QString& _text);

    QString failure() const;
    void setFailure(const QString& _text);
    Q_SIGNAL void failureChanged(const QString& _text);

    QString secrets() const;
    void setSecrets(const QString& _text);
    Q_SIGNAL void secretsChanged(const QString& _text);

    QString bestMemories() const;
    void setBestMemories(const QString& _text);
    Q_SIGNAL void bestMemoriesChanged(const QString& _text);

    QString worstMemories() const;
    void setWorstMemories(const QString& _text);
    Q_SIGNAL void worstMemoriesChanged(const QString& _text);

    //
    // STORY
    //

    QString shortTermGoal() const;
    void setShortTermGoal(const QString& _text);
    Q_SIGNAL void shortTermGoalChanged(const QString& _text);

    QString longTermGoal() const;
    void setLongTermGoal(const QString& _text);
    Q_SIGNAL void longTermGoalChanged(const QString& _text);

    QString initialBeliefs() const;
    void setInitialBeliefs(const QString& _text);
    Q_SIGNAL void initialBeliefsChanged(const QString& _text);

    QString changedBeliefs() const;
    void setChangedBeliefs(const QString& _text);
    Q_SIGNAL void changedBeliefsChanged(const QString& _text);

    QString whatLeadsToChange() const;
    void setWhatLeadsToChange(const QString& _text);
    Q_SIGNAL void whatLeadsToChangeChanged(const QString& _text);

    QString firstAppearance() const;
    void setFirstAppearance(const QString& _text);
    Q_SIGNAL void firstAppearanceChanged(const QString& _text);

    QString plotInvolvement() const;
    void setPlotInvolvement(const QString& _text);
    Q_SIGNAL void plotInvolvementChanged(const QString& _text);

    QString conflict() const;
    void setConflict(const QString& _text);
    Q_SIGNAL void conflictChanged(const QString& _text);

    QString mostDefiningMoment() const;
    void setMostDefiningMoment(const QString& _text);
    Q_SIGNAL void mostDefiningMomentChanged(const QString& _text);

    //
    // Диалоги
    //
    QVector<CharacterDialogues> dialogues() const;
    void setDialogues(const QVector<CharacterDialogues>& _dialogues);
    Q_SIGNAL void dialoguesChanged(const QVector<BusinessLayer::CharacterDialogues>& _dialogues);
    //
    void updateDialogues();
    Q_SIGNAL void dialoguesUpdateRequested();

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initImageWrapper() override;
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    void applyPatch(const QByteArray& _patch) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

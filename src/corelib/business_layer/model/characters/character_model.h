#pragma once

#include "../abstract_model.h"

#include <QColor>
#include <QUuid>


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

/**
 * @brief Модель данных персонажа
 */
class CORE_LIBRARY_EXPORT CharacterModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit CharacterModel(QObject* _parent = nullptr);
    ~CharacterModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _newName, const QString& _oldName);
    void setDocumentName(const QString& _name) override;

    QColor color() const;
    void setColor(const QColor& _color);
    Q_SIGNAL void colorChanged(const QColor& _color);

    CharacterStoryRole storyRole() const;
    void setStoryRole(CharacterStoryRole _role);
    Q_SIGNAL void storyRoleChanged(BusinessLayer::CharacterStoryRole _role);

    const QString& age() const;
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

    const QPixmap& mainPhoto() const;
    void setMainPhoto(const QPixmap& _photo);
    void setMainPhoto(const QUuid& _uuid, const QPixmap& _photo);
    Q_SIGNAL void mainPhotoChanged(const QPixmap& _photo);

    void createRelation(const QUuid& _withCharacter);
    void updateRelation(const CharacterRelation& _relation);
    void removeRelationWith(QUuid _character);
    CharacterRelation relation(const QUuid& _withCharacter);
    CharacterRelation relation(CharacterModel* _withCharacter);
    QVector<CharacterRelation> relations() const;
    Q_SIGNAL void relationAdded(const BusinessLayer::CharacterRelation& _relation);
    Q_SIGNAL void relationChanged(const BusinessLayer::CharacterRelation& _relation);
    Q_SIGNAL void relationRemoved(const BusinessLayer::CharacterRelation& _relation);

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

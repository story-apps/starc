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
 * @brief Модель данных персонажа
 */
class CORE_LIBRARY_EXPORT CharacterModel : public AbstractModel
{
    Q_OBJECT

public:
    /**
     * @brief Отношения с другим персонажем
     */
    struct Relation {
        QUuid character;
        QColor color;
        QString name;
        QString description;
    };

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
    Q_SIGNAL void storyRoleChanged(CharacterStoryRole _role);

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
    Q_SIGNAL void mainPhotoChanged(const QPixmap& _photo);

    void setRelationWith(QUuid _character, const QColor& _color, const QString& _title,
                         const QString& _description);
    void removeRelationWith(QUuid _character);
    QVector<Relation> relations() const;
    Q_SIGNAL void relationAdded(const Relation& _relation);
    Q_SIGNAL void relationChanged(const Relation& _relation);
    Q_SIGNAL void relationRemoved(const Relation& _relation);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

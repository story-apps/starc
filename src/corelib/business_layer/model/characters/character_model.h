#pragma once

#include "../abstract_model.h"


namespace BusinessLayer
{

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
    Q_SIGNAL void nameChanged(const QString& _name);
    void setDocumentName(const QString &_name) override;

    int storyRole() const;
    void setStoryRole(int _role);
    Q_SIGNAL void storyRoleChanged(int _role);

    QString oneSentenceDescription() const;
    void setOneSentenceDescription(const QString& _text);
    Q_SIGNAL void oneSentenceDescriptionChanged(const QString& _text);

    QString longDescription() const;
    void setLongDescription(const QString& _text);
    Q_SIGNAL void longDescriptionChanged(const QString& _text);

    const QPixmap& mainPhoto() const;
    void setMainPhoto(const QPixmap& _photo);
    Q_SIGNAL void mainPhotoChanged(const QPixmap& _photo);

    const QString& age() const;
    void setAge(const QString& _text);
    Q_SIGNAL void ageChanged(const QString& _text);

    const QString& gender() const;
    void setGender(const QString& _text);
    Q_SIGNAL void genderChanged(const QString& _text);

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

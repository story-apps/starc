#pragma once

#include "../abstract_model.h"


namespace BusinessLayer {

/**
 * @brief Модель данных локации
 */
class CORE_LIBRARY_EXPORT LocationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit LocationModel(QObject* _parent = nullptr);
    ~LocationModel() override;

    const QString& name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _newName, const QString& _oldName);
    void setDocumentName(const QString& _name) override;

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

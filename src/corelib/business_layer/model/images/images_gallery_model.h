#pragma once

#include "../abstract_model.h"

#include <domain/document_object.h>


namespace BusinessLayer {

/**
 * @brief Модель галереи изображений
 */
class CORE_LIBRARY_EXPORT ImagesGalleryModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit ImagesGalleryModel(QObject* _parent = nullptr);
    ~ImagesGalleryModel() override;

    QString name() const;
    void setName(const QString& _name);
    Q_SIGNAL void nameChanged(const QString& _name);
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;

    QString description() const;
    void setDescription(const QString& _description);
    Q_SIGNAL void descriptionChanged(const QString& _description);

    QVector<Domain::DocumentImage> photos() const;
    void addPhoto(const Domain::DocumentImage& _photo);
    void addPhotos(const QVector<QPixmap>& _photos);
    void removePhoto(const QUuid& _photoUuid);
    Q_SIGNAL void photosChanged(const QVector<Domain::DocumentImage>& _images);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initImageWrapper() override;
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    ChangeCursor applyPatch(const QByteArray& _patch) override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

#pragma once

#include "../abstract_model.h"

namespace BusinessLayer {

/**
 * @brief Модель презентации
 */
class CORE_LIBRARY_EXPORT PresentationModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit PresentationModel(QObject* _parent = nullptr);
    ~PresentationModel() override;

    /**
     * @brief Имя презентации
     */
    QString documentName() const override;
    void setDocumentName(const QString& _name) override;
    Q_SIGNAL void nameChanged(const QString& _name);

    /**
     * @brief Описание презентации
     */
    QString description() const;
    void setDescription(const QString& _description);
    Q_SIGNAL void descriptionChanged(const QString& _description);

    /**
     * @brief Изображения презентации
     */
    QVector<QUuid> imageUuids() const;
    QVector<QSize> imageSizes() const;
    Q_SIGNAL void imagesChanged();

    /**
     * @brief Загрузить изображения
     */
    QUuid loadImagesAsync(const QVector<QUuid>& _imageUuids);
    Q_SIGNAL void imagesLoaded(const QUuid& _queryUuid,
                               const QVector<QPair<QUuid, QImage>>& _images);
    Q_SIGNAL void imagesLoadingFailed();

    /**
     * @brief Разместить изображения из zip-архива во временные файлы
     */
    void storeToTempFiles(const QByteArray& _zipArchive);
    Q_SIGNAL void tempFilesStored();

    /**
     * @brief Изображение было обновлено
     */
    Q_SIGNAL void imageUpdated(const QUuid& _uuid, const QPixmap& _image);

    /**
     * @brief Запрос на загрузку презентации
     */
    Q_SIGNAL void downloadPresentationRequested(const QString& _filePath);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initImageWrapper() override;
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

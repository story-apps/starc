#pragma once

#include "../abstract_model.h"

#include <QPixmap>

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

    QString documentName() const override;
    void setDocumentName(const QString& _name) override;
    Q_SIGNAL void nameChanged(const QString& _name);

    QString description() const;
    void setDescription(const QString& _description);
    Q_SIGNAL void descriptionChanged(const QString& _description);

    /**
     * @brief Размеры изображений
     */
    QVector<QSize> imageSizes() const;

    /**
     * @brief Uuid документа zip-архива с изображениями
     */
    QUuid imagesArchiveUuid() const;

    /**
     * @brief Установить контент презентации (zip-врхив с изображениями и размеры этих изображений)
     */
    void setContent(const QByteArray& _imagesArchive, const QVector<QSize>& _sizes);

    /**
     * @brief Загрузить изображения из zip-архива
     */
    QUuid loadImages(int _from, int _amount);
    Q_SIGNAL void imagesLoaded(const QUuid& _queryUuid, const QVector<QPixmap>& _images);
    Q_SIGNAL void imagesLoadingFailed(const QUuid& _queryUuid);

    /**
     * @brief Запрос на загрузку презентации
     */
    Q_SIGNAL void downloadPresentationRequested(const QString& _filePath);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initRawDataWrapper() override;
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

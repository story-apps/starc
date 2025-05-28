#pragma once

#include "temp_image_file.h"

#include <QImage>
#include <QObject>

#include <corelib_global.h>

namespace ManagementLayer {

class CORE_LIBRARY_EXPORT TempImagesManager : public QObject
{
    Q_OBJECT

public:
    explicit TempImagesManager(QObject* _parent = nullptr);
    ~TempImagesManager();

    /**
     * @brief Асинхронно разместить содержимое zip-архива во временных файлах
     * @return Гуид запроса
     */
    QUuid storeAsync(const QByteArray& _zipArchive);

    /**
     * @brief Асинхронно загрузить изображения из файлов
     * @return Гуид запроса
     */
    QUuid loadAsync(const QVector<TempImagesLayer::TempImageFile>& _tempFiles);

signals:
    void requestedToStore(const QUuid& _queryUuid, const QByteArray& _zipArchive);
    void filesStored(const QUuid& _queryUuid,
                     const QVector<TempImagesLayer::TempImageFile>& _tempFiles);
    void requestedToLoad(const QUuid& _queryUuid,
                         const QVector<TempImagesLayer::TempImageFile>& _tempFiles);
    void imagesLoaded(const QUuid& _queryUuid, const QVector<QImage>& _images);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

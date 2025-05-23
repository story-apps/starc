#pragma once

#include "temp_image_file.h"

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
     */
    void storeAsync(const QByteArray& _zipArchive);

    /**
     * @brief Асинхронно загрузить изображения из файлов
     */
    void loadAsync(const QUuid& _queryUuid,
                   const QVector<TempImagesLayer::TempImageFile>& _tempFiles);

signals:
    void storeToTempFilesAsync(const QByteArray& _zipArchive);
    void filesStored(const QVector<TempImagesLayer::TempImageFile>& _tempFiles);
    void loadFromTempFilesAsync(const QUuid& _queryUuid,
                                const QVector<TempImagesLayer::TempImageFile>& _tempFiles);
    void imagesLoaded(const QUuid& _queryUuid, const QVector<QByteArray>& _images);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

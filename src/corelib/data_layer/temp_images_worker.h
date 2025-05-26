#pragma once

#include "temp_image_file.h"

#include <QObject>
#include <QTemporaryDir>
#include <QImage>

namespace TempImagesLayer {

class TempImagesWorker : public QObject
{
    Q_OBJECT

public:
    explicit TempImagesWorker(QObject* _parent = nullptr);

public slots:
    void storeToTempFiles(const QByteArray& _zipArchive);
    void loadFromTempFiles(const QUuid& _queryUuid, const QVector<TempImageFile>& _imageUuids);

signals:
    void filesStored(const QVector<TempImageFile>& _tempFiles);
    void imagesLoaded(const QUuid& _queryUuid, const QVector<QImage>& _images);

private:
    static QTemporaryDir m_tempImagesDir;
};

} // namespace TempImagesLayer

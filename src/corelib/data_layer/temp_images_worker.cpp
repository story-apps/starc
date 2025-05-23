#include "temp_images_worker.h"

#include <private/qzipreader_p.h>
#include <utils/helpers/image_helper.h>

#include <QBuffer>
#include <QFileInfo>
#include <QPixmap>

namespace TempImagesLayer {

TempImagesWorker::TempImagesWorker(QObject* _parent)
    : QObject(_parent)
{
    if (QMetaType::type("TempImageFile") == QMetaType::UnknownType) {
        qRegisterMetaType<TempImageFile>("TempImageFile");
    }
}

void TempImagesWorker::storeToTempFiles(const QByteArray& _zipArchive)
{
    QVector<TempImageFile> tempFilesInfo;

    QByteArray data = _zipArchive;
    QBuffer buffer(&data);
    if (buffer.open(QIODevice::ReadOnly)) {
        QZipReader zip(&buffer);
        if (zip.isReadable()) {
            auto fileInfoList = zip.fileInfoList();
            //
            // Сортируем по имени
            //
            std::sort(fileInfoList.begin(), fileInfoList.end(),
                      [](const QZipReader::FileInfo& a, const QZipReader::FileInfo& b) {
                          return a.filePath < b.filePath;
                      });

            //
            // Будем сохранять во временную директорию
            //
            QString path;
            if (m_tempImagesDir.isValid()) {
                path = m_tempImagesDir.path();
            }

            for (const auto& fileInfo : fileInfoList) {
                QByteArray data = zip.fileData(fileInfo.filePath);
                if (data.isEmpty()) {
                    continue;
                }

                QPixmap image;
                image.loadFromData(data);

                //
                // Сжимаем изображение, если его размер больше максимального
                //
                const auto maxSize = ImageHelper::maxSize();
                if (image.width() > maxSize.width() || image.height() > maxSize.height()) {
                    image = image.scaled(maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                }

                //
                // Формируем шаблон имени временного файла
                //
                QFileInfo fileInfoObj(fileInfo.filePath);
                const auto baseName = fileInfoObj.completeBaseName();
                const auto extension = fileInfoObj.suffix();
                const auto tempFileName = QString("%1/%2_XXXXXX.%3").arg(path, baseName, extension);

                //
                // Пишем данные во временный файл
                //
                auto tempFile = QSharedPointer<QTemporaryFile>::create(tempFileName);
                if (tempFile->open()) {
                    image.save(tempFile.data());
                    tempFile->close();
                    tempFilesInfo.append({ tempFile, image.size(), QUuid::createUuid() });
                }
            }
        }
        buffer.close();
    }
    emit filesStored(tempFilesInfo);
}

void TempImagesWorker::loadFromTempFiles(const QUuid& _queryUuid,
                                         const QVector<TempImageFile>& _tempFiles)
{
    QVector<QByteArray> images;
    for (const auto& file : _tempFiles) {
        if (file.tempFile.data()->open()) {
            images.append(file.tempFile.data()->readAll());
            file.tempFile.data()->close();
        }
    }
    emit imagesLoaded(_queryUuid, images);
}

QTemporaryDir TempImagesWorker::m_tempImagesDir("temp_images-XXXXXX");

} // namespace TempImagesLayer

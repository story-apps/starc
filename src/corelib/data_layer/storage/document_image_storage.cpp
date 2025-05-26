#include "document_image_storage.h"

#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <data_layer/temp_image_file.h>
#include <data_layer/temp_images_manager.h>
#include <domain/document_object.h>
#include <utils/helpers/image_helper.h>

#include <QCache>
#include <QHash>
#include <QPixmap>


namespace DataStorageLayer {

class DocumentImageStorage::Implementation
{
public:
    explicit Implementation(DocumentImageStorage* _q);

    /**
     * @brief Запросить изображение
     */
    void notifyImageRequested(const QUuid& _uuid) const;


    DocumentImageStorage* q = nullptr;

    /**
     * @brief Кэш загруженных изображений
     */
    mutable QCache<QUuid, QPixmap> cachedImages;

    /**
     * @brief Список новых изображений
     */
    mutable QHash<QUuid, QPixmap> newImages;

    /**
     * @brief Список изображений на удаление
     */
    QList<QUuid> imagesToRemove;

    /**
     * @brief Менеджер изображений, сохраненных во временные файлы
     */
    ManagementLayer::TempImagesManager* tempImagesManager = nullptr;

    /**
     * @brief Список новых изображений, сохраненных во временные файлы
     */
    QHash<QUuid, TempImagesLayer::TempImageFile> imagesInTempFiles;
};

DocumentImageStorage::Implementation::Implementation(DocumentImageStorage* _q)
    : q(_q)
    , tempImagesManager(new ManagementLayer::TempImagesManager(_q))
{
}

void DocumentImageStorage::Implementation::notifyImageRequested(const QUuid& _uuid) const
{
    QMetaObject::invokeMethod(
        q, [this, _uuid] { emit q->imageRequested(_uuid); }, Qt::QueuedConnection);
}


// ****


DocumentImageStorage::DocumentImageStorage(QObject* _parent)
    : AbstractImageWrapper(_parent)
    , d(new Implementation(this))
{
    connect(StorageFacade::documentStorage(), &DocumentStorage::documentsLoaded, this,
            [this](const QUuid& _queryUuid, QVector<Domain::DocumentObject*> _documents) {
                QVector<QImage> images;
                for (const auto& imageDocument : _documents) {
                    //
                    // ... подписываемся на обновления изображения (и загружаем, если не был ещё
                    // загружен из облака)
                    //
                    d->notifyImageRequested(imageDocument->uuid());
                    if (imageDocument == nullptr) {
                        images.append(QImage());
                    } else {
                        Q_ASSERT(imageDocument->type() == Domain::DocumentObjectType::ImageData);
                        QImage image;
                        image.loadFromData(imageDocument->content());
                        images.append(image);
                    }
                }
                emit imagesLoaded(_queryUuid, images);
            });
    connect(d->tempImagesManager, &ManagementLayer::TempImagesManager::imagesLoaded, this,
            [this](const QUuid& _queryUuid, const QVector<QImage>& _images) {
                emit imagesLoaded(_queryUuid, _images);
            });
    connect(d->tempImagesManager, &ManagementLayer::TempImagesManager::filesStored, this,
            [this](const QVector<TempImagesLayer::TempImageFile>& _tempFiles) {
                for (const auto& file : _tempFiles) {
                    d->imagesInTempFiles.insert(file.uuid, file);
                }
                emit tempFilesStored(_tempFiles);
            });
}

DocumentImageStorage::~DocumentImageStorage() = default;

QPixmap DocumentImageStorage::load(const QUuid& _uuid) const
{
    if (_uuid.isNull()) {
        return {};
    }

    if (auto imageIter = d->newImages.find(_uuid); imageIter != d->newImages.end()) {
        return imageIter.value();
    }

    if (d->cachedImages.contains(_uuid)) {
        auto cachedImage = d->cachedImages[_uuid];
        if (cachedImage == nullptr) {
            return {};
        }
        return *cachedImage;
    }

    //
    // Загружаем изображение
    //
    auto imageDocument = StorageFacade::documentStorage()->document(_uuid);
    //
    // ... подписываемся на его обновления (и загружаем, если не был ещё загружен из облака)
    //
    d->notifyImageRequested(_uuid);
    //
    // ... если изображения пока нет в базе, то поставим в кэш заглушку для него
    //
    if (imageDocument == nullptr) {
        d->cachedImages.insert(_uuid, {});
        return {};
    }

    Q_ASSERT(imageDocument->type() == Domain::DocumentObjectType::ImageData);

    //
    // NOTE: тут грузим вручную, а не через Imagehelper т.к. в кэш нужен именно указатель
    //
    QPixmap* image = new QPixmap;
    image->loadFromData(imageDocument->content());
    d->cachedImages.insert(_uuid, image);
    return *image;
}

void DocumentImageStorage::loadAsync(const QUuid& _queryUuid,
                                     const QVector<QUuid>& _imageUuids) const
{
    QVector<TempImagesLayer::TempImageFile> filesToLoad;
    for (const auto& uuid : _imageUuids) {
        //
        // Сперва ищем во временных файлах
        //
        const auto it = d->imagesInTempFiles.find(uuid);
        if (it != d->imagesInTempFiles.end()) {
            filesToLoad.append(it.value());
        } else {
            //
            // Если хотя бы одного файла нет — грузим из БД
            //
            StorageFacade::documentStorage()->loadDocumentsAsync(_queryUuid, _imageUuids);
            return;
        }
    }
    //
    // Если все изображения во временных файлах, грузим из них
    //
    d->tempImagesManager->loadAsync(_queryUuid, filesToLoad);
}

QUuid DocumentImageStorage::save(const QPixmap& _image)
{
    if (_image.isNull()) {
        return {};
    }

    //
    // Сохраним изображение во временный буфер
    //
    const QUuid uuid = QUuid::createUuid();
    d->newImages.insert(uuid, _image);
    //
    // ... положим в хранилище
    //
    auto document = StorageFacade::documentStorage()->createDocument(
        uuid, Domain::DocumentObjectType::ImageData);
    document->setContent(ImageHelper::bytesFromImage(_image));
    //
    // ... уведомляем о добавленном изображении
    //
    emit imageAdded(uuid);

    return uuid;
}

void DocumentImageStorage::save(const QUuid& _uuid, const QPixmap& _image)
{
    save(_uuid, ImageHelper::bytesFromImage(_image));
}

void DocumentImageStorage::save(const QUuid& _uuid, const QByteArray& _imageData)
{
    if (_uuid.isNull()) {
        return;
    }

    //
    // Сохраним изображение во временный буфер
    //
    const auto image = ImageHelper::imageFromBytes(_imageData);
    d->newImages.insert(_uuid, image);
    //
    // ... уберём заглушку из кэша, если она там была
    //
    if (d->cachedImages.contains(_uuid)) {
        d->cachedImages.remove(_uuid);
    }
    //
    // ... положим в хранилище, если ещё не был сохранён
    //
    auto document = StorageFacade::documentStorage()->document(_uuid);
    if (document == nullptr) {
        document = StorageFacade::documentStorage()->createDocument(
            _uuid, Domain::DocumentObjectType::ImageData);
    }
    //
    // ... а если был, проверим, нужно ли его обновлять
    //
    else if (document->content() == _imageData) {
        return;
    }
    document->setContent(_imageData);
    //
    // ... уведомляем об обновлении изображения
    //
    emit imageUpdated(_uuid, image);
}

void DocumentImageStorage::storeToTempAsync(const QByteArray& _zipArchive)
{
    d->tempImagesManager->storeAsync(_zipArchive);
}

void DocumentImageStorage::remove(const QUuid& _uuid)
{
    //
    // Если изображение новое, просто удаляем его из списка новых, иначе добавляем в список на
    // удаление из БД
    //
    if (!d->newImages.remove(_uuid)) {
        d->imagesToRemove.append(_uuid);
    }
    emit imageRemoved(_uuid);
}

void DocumentImageStorage::clear()
{
    //
    // Кэш специально не очищаем, т.к. есть вероятность, что пользователь опять откроет
    // тот же проект, в противном же случае айдишники картинок будут уникальны в любом случае
    //

    d->newImages.clear();
    d->imagesToRemove.clear();
}

void DocumentImageStorage::saveChanges()
{
    //
    // Сохраняем изображения, хранящиеся в памяти
    //
    for (auto imageIter = d->newImages.begin(); imageIter != d->newImages.end(); ++imageIter) {
        StorageFacade::documentStorage()->saveDocument(imageIter.key());
    }
    d->newImages.clear();

    //
    // Сохраняем изображения, хранящиеся во временных файлах
    //
    for (const auto& tempFile : d->imagesInTempFiles) {
        auto document = StorageFacade::documentStorage()->createDocument(
            tempFile.uuid, Domain::DocumentObjectType::ImageData);
        if (tempFile.tempFile->open()) {
            document->setContent(tempFile.tempFile->readAll());
            tempFile.tempFile->close();
        }
        StorageFacade::documentStorage()->saveDocument(document);
    }
    d->imagesInTempFiles.clear();

    while (!d->imagesToRemove.isEmpty()) {
        StorageFacade::documentStorage()->removeDocument(
            StorageFacade::documentStorage()->document(d->imagesToRemove.takeFirst()));
    }
}

void DocumentImageStorage::saveChangesAsync()
{
    //
    // Сохраняем изображения, хранящиеся в памяти
    //
    for (auto imageIter = d->newImages.begin(); imageIter != d->newImages.end(); ++imageIter) {
        StorageFacade::documentStorage()->saveDocumentAsync({}, imageIter.key());
    }
    d->newImages.clear();

    //
    // Сохраняем изображения, хранящиеся во временных файлах
    //
    for (const auto& tempFile : d->imagesInTempFiles) {
        auto document = StorageFacade::documentStorage()->createDocument(
            tempFile.uuid, Domain::DocumentObjectType::ImageData);
        if (tempFile.tempFile->open()) {
            document->setContent(tempFile.tempFile->readAll());
            tempFile.tempFile->close();
        }
        StorageFacade::documentStorage()->saveDocumentAsync({}, document);
    }
    d->imagesInTempFiles.clear();

    while (!d->imagesToRemove.isEmpty()) {
        StorageFacade::documentStorage()->removeDocumentAsync(
            {}, StorageFacade::documentStorage()->document(d->imagesToRemove.takeFirst()));
    }
}

} // namespace DataStorageLayer

#include "document_image_storage.h"

#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
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
};

DocumentImageStorage::Implementation::Implementation(DocumentImageStorage* _q)
    : q(_q)
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

    auto imageDocument = StorageFacade::documentStorage()->document(_uuid);
    if (imageDocument == nullptr) {
        d->cachedImages.insert(_uuid, {});
        d->notifyImageRequested(_uuid);
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
    if (_uuid.isNull() || _imageData.isNull()) {
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
    // ... положим или обновим в хранилище
    //
    auto document = StorageFacade::documentStorage()->document(_uuid);
    if (document == nullptr) {
        document = StorageFacade::documentStorage()->createDocument(
            _uuid, Domain::DocumentObjectType::ImageData);
    }
    document->setContent(_imageData);
    //
    // ... уведомляем об обновлении изображения
    //
    emit imageUpdated(_uuid, image);
}

void DocumentImageStorage::clear()
{
    //
    // Кэш специально не очищаем, т.к. есть вероятность, что пользователь опять откроет
    // тот же проект, в противном же случае айдишники картинок будут уникальны в любом случае
    //

    d->newImages.clear();
}

void DocumentImageStorage::saveChanges()
{
    for (auto imageIter = d->newImages.begin(); imageIter != d->newImages.end(); ++imageIter) {
        StorageFacade::documentStorage()->saveDocument(imageIter.key());
    }
    d->newImages.clear();
}

} // namespace DataStorageLayer

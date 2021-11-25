#include "document_data_storage.h"

#include <data_layer/mapper/document_mapper.h>
#include <data_layer/mapper/mapper_facade.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>
#include <utils/helpers/image_helper.h>

#include <QCache>
#include <QHash>
#include <QPixmap>


namespace DataStorageLayer {

class DocumentImageStorage::Implementation
{
public: /**
         * @brief Кэш загруженных изображений
         */
    mutable QCache<QUuid, QPixmap> cachedImages;

    /**
     * @brief Список новых изображений
     */
    mutable QHash<QUuid, QPixmap> newImages;
};

DocumentImageStorage::DocumentImageStorage()
    : d(new Implementation)
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
        return *d->cachedImages[_uuid];
    }

    auto imageDocument = DataMappingLayer::MapperFacade::documentMapper()->find(_uuid);
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

QUuid DocumentImageStorage::save(const QPixmap& _image)
{
    if (_image.isNull()) {
        return {};
    }

    const QUuid uuid = QUuid::createUuid();
    d->newImages.insert(uuid, _image);
    return uuid;
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
        auto newDocument = Domain::ObjectsBuilder::createDocument(
            {}, imageIter.key(), Domain::DocumentObjectType::ImageData,
            ImageHelper::bytesFromImage(imageIter.value()));
        DataMappingLayer::MapperFacade::documentMapper()->insert(newDocument);
    }

    d->newImages.clear();
}

} // namespace DataStorageLayer

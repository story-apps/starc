#include "images_gallery_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kPhotosKey("photos");
const QLatin1String kPhotoKey("photo");
} // namespace


class ImagesGalleryModel::Implementation
{
public:
    QString name;
    QVector<Domain::DocumentImage> photos;
};


// ****


ImagesGalleryModel::ImagesGalleryModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{

    connect(this, &ImagesGalleryModel::nameChanged, this,
            &ImagesGalleryModel::updateDocumentContent);
    connect(this, &ImagesGalleryModel::photosChanged, this,
            &ImagesGalleryModel::updateDocumentContent);
}

ImagesGalleryModel::~ImagesGalleryModel() = default;

QString ImagesGalleryModel::name() const
{
    return d->name;
}

void ImagesGalleryModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString ImagesGalleryModel::documentName() const
{
    return name();
}

void ImagesGalleryModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

QVector<Domain::DocumentImage> ImagesGalleryModel::photos() const
{
    return d->photos;
}

void ImagesGalleryModel::addPhoto(const Domain::DocumentImage& _photo)
{
    if (_photo.uuid.isNull() && _photo.image.isNull()) {
        return;
    }

    d->photos.append(_photo);
    emit photosChanged(d->photos);
}

void ImagesGalleryModel::addPhotos(const QVector<QPixmap>& _photos)
{
    if (_photos.isEmpty()) {
        return;
    }

    for (const auto& photo : _photos) {
        d->photos.append({ imageWrapper()->save(photo), photo });
    }
    emit photosChanged(d->photos);
}

void ImagesGalleryModel::removePhoto(const QUuid& _photoUuid)
{
    for (int index = 0; index < d->photos.size(); ++index) {
        if (d->photos.at(index).uuid != _photoUuid) {
            continue;
        }

        imageWrapper()->remove(_photoUuid);
        d->photos.removeAt(index);
        emit photosChanged(d->photos);
        break;
    }
}

void ImagesGalleryModel::initImageWrapper()
{
    connect(imageWrapper(), &AbstractImageWrapper::imageUpdated, this,
            [this](const QUuid& _uuid, const QPixmap& _image) {
                for (auto& photo : d->photos) {
                    if (photo.uuid == _uuid) {
                        photo.image = _image;
                        emit photosChanged(d->photos);
                        break;
                    }
                }
            });
}

void ImagesGalleryModel::initDocument()
{
    if (document() == nullptr) {
        return;
    }

    QDomDocument domDocument;
    domDocument.setContent(document()->content());
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto load = [&documentNode](const QString& _key) {
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
    };
    d->name = load(kNameKey);
    const auto photosNode = documentNode.firstChildElement(kPhotosKey);
    if (!photosNode.isNull()) {
        auto photoNode = photosNode.firstChildElement(kPhotoKey);
        while (!photoNode.isNull()) {
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            if (!uuid.isNull()) {
                d->photos.append({ uuid, imageWrapper()->load(uuid) });
            }

            photoNode = photoNode.nextSiblingElement();
        }
    }
}

void ImagesGalleryModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray ImagesGalleryModel::toXml() const
{
    if (document() == nullptr) {
        return {};
    }

    QByteArray xml = "<?xml version=\"1.0\"?>\n";
    xml += QString("<%1 mime-type=\"%2\" version=\"1.0\">\n")
               .arg(kDocumentKey, Domain::mimeTypeFor(document()->type()))
               .toUtf8();
    auto save = [&xml](const QString& _key, const QString& _value) {
        xml += QString("<%1><![CDATA[%2]]></%1>\n")
                   .arg(_key, TextHelper::toHtmlEscaped(_value))
                   .toUtf8();
    };
    save(kNameKey, d->name);
    if (!d->photos.isEmpty()) {
        xml += QString("<%1>\n").arg(kPhotosKey).toUtf8();
        for (const auto& photo : std::as_const(d->photos)) {
            xml += QString("<%1><![CDATA[%2]]></%1>\n")
                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(photo.uuid.toString()))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(kPhotosKey).toUtf8();
    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

void ImagesGalleryModel::applyPatch(const QByteArray& _patch)
{
    //
    // Применяем изменения
    //
    const auto newContent = dmpController().applyPatch(toXml(), _patch);

    //
    // Cчитываем изменённые данные
    //
    QDomDocument domDocument;
    domDocument.setContent(newContent);
    const auto documentNode = domDocument.firstChildElement(kDocumentKey);
    auto load = [&documentNode](const QString& _key) {
        return TextHelper::fromHtmlEscaped(documentNode.firstChildElement(_key).text());
    };
    setName(load(kNameKey));
    //
    // Считываем фотографии
    //
    auto photosNode = documentNode.firstChildElement(kPhotosKey);
    QVector<QUuid> newPhotosUuids;
    if (!photosNode.isNull()) {
        auto photoNode = photosNode.firstChildElement(kPhotoKey);
        while (!photoNode.isNull()) {
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
            newPhotosUuids.append(uuid);

            photoNode = photoNode.nextSiblingElement();
        }
    }
    //
    // ... корректируем текущие фотографии персонажа
    //
    for (int photoIndex = 0; photoIndex < d->photos.size(); ++photoIndex) {
        const auto& photo = d->photos.at(photoIndex);
        //
        // ... если такое отношение осталось актуальным, то оставим его в списке текущих
        //     и удалим из списка новых
        //
        if (newPhotosUuids.contains(photo.uuid)) {
            newPhotosUuids.removeAll(photo.uuid);
        }
        //
        // ... если такого отношения нет в списке новых, то удалим его из списка текущих
        //
        else {
            removePhoto(photo.uuid);
            --photoIndex;
        }
    }
    //
    // ... добавляем новые фотографии к персонажу
    //
    for (const auto& photoUuid : newPhotosUuids) {
        addPhoto({ photoUuid });
        imageWrapper()->load(photoUuid);
    }
}

} // namespace BusinessLayer

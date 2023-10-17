#include "mind_map_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <domain/document_object.h>
#include <utils/diff_match_patch/diff_match_patch_controller.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>


namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kDescriptionKey("description");
// const QLatin1String kPhotosKey("photos");
// const QLatin1String kPhotoKey("photo");
} // namespace


class MindMapModel::Implementation
{
public:
    QString name;
    QString description;
    //    QVector<Domain::DocumentImage> photos;
};


// ****


MindMapModel::MindMapModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{

    connect(this, &MindMapModel::nameChanged, this, &MindMapModel::updateDocumentContent);
    connect(this, &MindMapModel::descriptionChanged, this, &MindMapModel::updateDocumentContent);
}

MindMapModel::~MindMapModel() = default;

QString MindMapModel::name() const
{
    return d->name;
}

void MindMapModel::setName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString MindMapModel::documentName() const
{
    return name();
}

void MindMapModel::setDocumentName(const QString& _name)
{
    setName(_name);
}

QString MindMapModel::description() const
{
    return d->description;
}

void MindMapModel::setDescription(const QString& _description)
{
    if (d->description == _description) {
        return;
    }

    d->description = _description;
    emit descriptionChanged(d->description);
}

void MindMapModel::initImageWrapper()
{
    //    connect(imageWrapper(), &AbstractImageWrapper::imageUpdated, this,
    //            [this](const QUuid& _uuid, const QPixmap& _image) {
    //                for (auto& photo : d->photos) {
    //                    if (photo.uuid == _uuid) {
    //                        photo.image = _image;
    //                        emit photosChanged(d->photos);
    //                        break;
    //                    }
    //                }
    //            });
}

void MindMapModel::initDocument()
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
    d->description = load(kDescriptionKey);
    //    const auto photosNode = documentNode.firstChildElement(kPhotosKey);
    //    if (!photosNode.isNull()) {
    //        auto photoNode = photosNode.firstChildElement(kPhotoKey);
    //        while (!photoNode.isNull()) {
    //            const auto uuid =
    //            QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text())); if
    //            (!uuid.isNull()) {
    //                d->photos.append({ uuid, imageWrapper()->load(uuid) });
    //            }

    //            photoNode = photoNode.nextSiblingElement();
    //        }
    //    }
}

void MindMapModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray MindMapModel::toXml() const
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
    save(kDescriptionKey, d->description);
    //    if (!d->photos.isEmpty()) {
    //        xml += QString("<%1>\n").arg(kPhotosKey).toUtf8();
    //        for (const auto& photo : std::as_const(d->photos)) {
    //            xml += QString("<%1><![CDATA[%2]]></%1>\n")
    //                       .arg(kPhotoKey, TextHelper::toHtmlEscaped(photo.uuid.toString()))
    //                       .toUtf8();
    //        }
    //        xml += QString("</%1>\n").arg(kPhotosKey).toUtf8();
    //    }
    xml += QString("</%1>").arg(kDocumentKey).toUtf8();
    return xml;
}

ChangeCursor MindMapModel::applyPatch(const QByteArray& _patch)
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
    setDescription(load(kDescriptionKey));
    //    //
    //    // Считываем фотографии
    //    //
    //    auto photosNode = documentNode.firstChildElement(kPhotosKey);
    //    QVector<QUuid> newPhotosUuids;
    //    if (!photosNode.isNull()) {
    //        auto photoNode = photosNode.firstChildElement(kPhotoKey);
    //        while (!photoNode.isNull()) {
    //            const auto uuid =
    //            QUuid::fromString(TextHelper::fromHtmlEscaped(photoNode.text()));
    //            newPhotosUuids.append(uuid);

    //            photoNode = photoNode.nextSiblingElement();
    //        }
    //    }
    //    //
    //    // ... корректируем текущие фотографии персонажа
    //    //
    //    for (int photoIndex = 0; photoIndex < d->photos.size(); ++photoIndex) {
    //        const auto& photo = d->photos.at(photoIndex);
    //        //
    //        // ... если такое отношение осталось актуальным, то оставим его в списке текущих
    //        //     и удалим из списка новых
    //        //
    //        if (newPhotosUuids.contains(photo.uuid)) {
    //            newPhotosUuids.removeAll(photo.uuid);
    //        }
    //        //
    //        // ... если такого отношения нет в списке новых, то удалим его из списка текущих
    //        //
    //        else {
    //            removePhoto(photo.uuid);
    //            --photoIndex;
    //        }
    //    }
    //    //
    //    // ... добавляем новые фотографии к персонажу
    //    //
    //    for (const auto& photoUuid : newPhotosUuids) {
    //        addPhoto({ photoUuid });
    //        imageWrapper()->load(photoUuid);
    //    }

    return {};
}

} // namespace BusinessLayer

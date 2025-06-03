#include "presentation_model.h"

#include <business_layer/model/abstract_image_wrapper.h>
#include <data_layer/temp_image_file.h>
#include <domain/document_object.h>
#include <utils/helpers/text_helper.h>

#include <QDomDocument>

namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kDescriptionKey("description");
const QLatin1String kImagesKey("images");
const QLatin1String kImageKey("image");
const QLatin1String kImageWidthKey("width");
const QLatin1String kImageHeightKey("height");
} // namespace


class PresentationModel::Implementation
{
public:
    QString name;
    QString description;

    QVector<QSize> imageSizes;
    QVector<QUuid> imageUuids;

    //
    // Последний запрос на загрузку изображений (uuid самого запроса и соответсвтующие ему uuid'ы
    // изображений)
    //
    struct {
        QUuid queryUuid;
        QVector<QUuid> imageUuids;
    } lastQueryToLoad;

    //
    // Uuid запроса на размещение изображений во временные файлы
    //
    QUuid queryToStore;
};

PresentationModel::PresentationModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
    connect(this, &PresentationModel::nameChanged, this, &PresentationModel::updateDocumentContent);
    connect(this, &PresentationModel::descriptionChanged, this,
            &PresentationModel::updateDocumentContent);
    connect(this, &PresentationModel::imagesChanged, this,
            &PresentationModel::updateDocumentContent);
}

PresentationModel::~PresentationModel() = default;

QString PresentationModel::documentName() const
{
    return d->name;
}

void PresentationModel::setDocumentName(const QString& _name)
{
    if (d->name == _name) {
        return;
    }

    d->name = _name;
    emit nameChanged(d->name);
    emit documentNameChanged(d->name);
}

QString PresentationModel::description() const
{
    return d->description;
}

void PresentationModel::setDescription(const QString& _description)
{
    if (d->description == _description) {
        return;
    }

    d->description = _description;
    emit descriptionChanged(d->description);
}

QVector<QUuid> PresentationModel::imageUuids() const
{
    return d->imageUuids;
}

QVector<QSize> PresentationModel::imageSizes() const
{
    return d->imageSizes;
}

QUuid PresentationModel::loadImagesAsync(const QVector<QUuid>& _imageUuids)
{
    const auto queryUuid = imageWrapper()->loadAsync(_imageUuids);
    d->lastQueryToLoad = { queryUuid, _imageUuids };
    return queryUuid;
}

void PresentationModel::storeToTempFiles(const QByteArray& _zipArchive)
{
    d->queryToStore = imageWrapper()->storeToTempAsync(_zipArchive);
}

void PresentationModel::initImageWrapper()
{
    connect(imageWrapper(), &AbstractImageWrapper::imageUpdated, this,
            &PresentationModel::imageUpdated);
    connect(imageWrapper(), &AbstractImageWrapper::imagesLoaded, this,
            [this](const QUuid& _queryUuid, const QVector<QImage>& _images) {
                //
                // Обрабатываем только последний запрос
                //
                if (d->lastQueryToLoad.queryUuid != _queryUuid) {
                    return;
                }

                if (_images.size() == d->lastQueryToLoad.imageUuids.size()) {
                    //
                    // Объединяем изображения с их uuid'ами
                    //
                    QVector<QPair<QUuid, QImage>> loadedImages;
                    for (int i = 0; i < _images.size(); ++i) {
                        loadedImages.append({ d->lastQueryToLoad.imageUuids[i], _images[i] });
                    }
                    emit imagesLoaded(_queryUuid, loadedImages);
                } else {
                    Q_ASSERT(false);
                    emit imagesLoadingFailed();
                }
            });
    connect(
        imageWrapper(), &AbstractImageWrapper::tempFilesStored, this,
        [this](const QUuid& _queryUuid, const QVector<TempImagesLayer::TempImageFile>& _tempFiles) {
            if (d->queryToStore != _queryUuid) {
                return;
            }

            //
            // Устанавливаем в модель размеры изображений и их uuid'ы
            //
            d->imageUuids.clear();
            d->imageSizes.clear();
            for (const auto& file : _tempFiles) {
                d->imageSizes.append(file.imageSize);
                d->imageUuids.append(file.uuid);
            }

            emit imagesChanged();
            emit tempFilesStored();
        });
}

void PresentationModel::initDocument()
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
    const auto imagesNode = documentNode.firstChildElement(kImagesKey);
    if (!imagesNode.isNull()) {
        auto imageNode = imagesNode.firstChildElement(kImageKey);
        while (!imageNode.isNull()) {
            const auto uuid = QUuid::fromString(TextHelper::fromHtmlEscaped(imageNode.text()));
            if (!uuid.isNull()) {
                QSize size;
                size.setWidth(imageNode.attribute(kImageWidthKey, "0").toInt());
                size.setHeight(imageNode.attribute(kImageHeightKey, "0").toInt());
                d->imageSizes.append(size);
                d->imageUuids.append(uuid);
            } else {
                Q_ASSERT(false);
            }
            imageNode = imageNode.nextSiblingElement();
        }
    }
}

void PresentationModel::clearDocument()
{
    d.reset(new Implementation);
}

QByteArray PresentationModel::toXml() const
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
    if (!d->imageSizes.isEmpty() || !d->imageUuids.isEmpty()) {
        if (d->imageUuids.size() == d->imageSizes.size()) {
            xml += QString("<%1>\n").arg(kImagesKey).toUtf8();
            for (int i = 0; i < d->imageSizes.size(); ++i) {
                xml += QString("<%1 %2=\"%3\" %4=\"%5\"><![CDATA[%6]]></%1>\n")
                           .arg(kImageKey, kImageWidthKey,
                                QString::number(d->imageSizes[i].width()), kImageHeightKey,
                                QString::number(d->imageSizes[i].height()),
                                TextHelper::toHtmlEscaped(d->imageUuids[i].toString()))
                           .toUtf8();
            }
            xml += QString("</%1>\n").arg(kImagesKey).toUtf8();
        } else {
            Q_ASSERT(false);
        }
    }
    xml += QString("</%1>\n").arg(kDocumentKey).toUtf8();
    return xml;
}

} // namespace BusinessLayer

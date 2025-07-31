#include "presentation_model.h"

#include <business_layer/model/abstract_raw_data_wrapper.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <private/qzipreader_p.h>
#include <utils/helpers/text_helper.h>

#include <QBuffer>
#include <QDomDocument>
#include <QtConcurrent>

namespace BusinessLayer {

namespace {
const QLatin1String kDocumentKey("document");
const QLatin1String kNameKey("name");
const QLatin1String kDescriptionKey("description");
const QLatin1String kZipArchiveKey("zip_archive");
const QLatin1String kImageSizesKey("image_sizes");
const QLatin1String kImageSizeKey("size");
const QLatin1String kImageWidthKey("width");
const QLatin1String kImageHeightKey("height");

} // namespace


class PresentationModel::Implementation
{
public:
    QString name;
    QString description;

    /**
     * @brief Размеры изображений в архиве
     */
    QVector<QSize> imageSizes;

    /**
     * @brief Архив с изображениями
     */
    QByteArray imagesArchive;
    QUuid imagesArchiveUuid;
};


// ****


PresentationModel::PresentationModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
    connect(this, &PresentationModel::nameChanged, this, &PresentationModel::updateDocumentContent);
    connect(this, &PresentationModel::descriptionChanged, this,
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

QVector<QSize> PresentationModel::imageSizes() const
{
    return d->imageSizes;
}

QUuid PresentationModel::imagesArchiveUuid() const
{
    return d->imagesArchiveUuid;
}

void PresentationModel::setContent(const QByteArray& _imagesArchive, const QVector<QSize>& _sizes)
{
    //
    // Если в модели уже был установлен архив, то удалим его
    //
    if (!d->imagesArchiveUuid.isNull()) {
        rawDataWrapper()->remove(d->imagesArchiveUuid);
        d->imagesArchiveUuid = QUuid();
        d->imagesArchive = {};
    }

    //
    // Кладем архив в хранилище и запоминаем его uuid
    //
    d->imagesArchiveUuid = rawDataWrapper()->save(_imagesArchive);

    d->imagesArchive = _imagesArchive;
    d->imageSizes = _sizes;

    updateDocumentContent();
}

QUuid PresentationModel::loadImages(int _from, int _amount)
{
    const auto queryUuid = QUuid::createUuid();
    QtConcurrent::run([this, queryUuid, _from, _amount]() {
        //
        // Отправить сигнал об ошибке из основного потока
        // TODO: передавать инфу о конкретной ошибке наружу
        //
        auto notifyFailed = [this, queryUuid]() {
            QMetaObject::invokeMethod(
                this, [this, queryUuid]() { emit imagesLoadingFailed(queryUuid); },
                Qt::QueuedConnection);
        };
        if (d->imagesArchive.isEmpty()) {
            notifyFailed();
            return;
        }

        QBuffer buffer(&d->imagesArchive);
        if (!buffer.open(QIODevice::ReadOnly)) {
            notifyFailed();
            return;
        }

        QZipReader zip(&buffer);
        if (!zip.isReadable()) {
            notifyFailed();
            return;
        }

        auto fileInfoList = zip.fileInfoList();
        const bool outOfRange = fileInfoList.size() < _from + _amount;
        if (outOfRange) {
            Q_ASSERT(false);
            notifyFailed();
            return;
        }

        //
        // Сортируем по имени
        //
        std::sort(fileInfoList.begin(), fileInfoList.end(),
                  [](const QZipReader::FileInfo& a, const QZipReader::FileInfo& b) {
                      return a.filePath < b.filePath;
                  });

        QVector<QImage> images;
        for (int i = _from; i < _from + _amount; ++i) {
            QByteArray data = zip.fileData(fileInfoList[i].filePath);
            if (data.isEmpty()) {
                continue;
            }
            QImage image;
            image.loadFromData(data);
            images.append(image);
        }

        //
        // Из основного потока отправляем сигнал об окончании загрузки
        //
        QMetaObject::invokeMethod(
            this,
            [this, queryUuid, images]() {
                QVector<QPixmap> pixmaps;
                for (const auto& image : images) {
                    pixmaps.append(QPixmap::fromImage(image));
                }
                emit imagesLoaded(queryUuid, pixmaps);
            },
            Qt::QueuedConnection);
    });

    return queryUuid;
}

void PresentationModel::initRawDataWrapper()
{
    connect(rawDataWrapper(), &AbstractRawDataWrapper::rawDataUpdated, this,
            [this](const QUuid& _uuid, const QByteArray& _data) {
                if (d->imagesArchiveUuid == _uuid) {
                    d->imagesArchive = _data;

                    updateDocumentContent();
                }
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
    d->imagesArchiveUuid = QUuid(load(kZipArchiveKey));
    d->imagesArchive = rawDataWrapper()->load(d->imagesArchiveUuid);

    const auto imageSizesNode = documentNode.firstChildElement(kImageSizesKey);
    if (imageSizesNode.isNull()) {
        return;
    }

    auto imageSizeNode = imageSizesNode.firstChildElement(kImageSizeKey);
    while (!imageSizeNode.isNull()) {
        QSize size;
        size.setWidth(imageSizeNode.attribute(kImageWidthKey, "0").toInt());
        size.setHeight(imageSizeNode.attribute(kImageHeightKey, "0").toInt());
        d->imageSizes.append(size);
        imageSizeNode = imageSizeNode.nextSiblingElement();
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
    save(kZipArchiveKey, d->imagesArchiveUuid.toString());
    if (!d->imageSizes.isEmpty()) {
        xml += QString("<%1>\n").arg(kImageSizesKey).toUtf8();
        for (int i = 0; i < d->imageSizes.size(); ++i) {
            xml += QString("<%1 %2=\"%3\" %4=\"%5\"/>\n")
                       .arg(kImageSizeKey, kImageWidthKey,
                            QString::number(d->imageSizes[i].width()), kImageHeightKey,
                            QString::number(d->imageSizes[i].height()))
                       .toUtf8();
        }
        xml += QString("</%1>\n").arg(kImageSizesKey).toUtf8();
    }
    xml += QString("</%1>\n").arg(kDocumentKey).toUtf8();

    return xml;
}

} // namespace BusinessLayer

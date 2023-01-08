#include "unsplash_images_view.h"

#include <3rd_party/webloader/src/NetworkRequestLoader.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPaintEvent>
#include <QPainter>
#include <QVariantAnimation>


namespace Ui {

namespace {

/**
 * @brief Размер изображений для отображения
 */
constexpr int kImageWidth = 200;

} // namespace

/**
 * @brief Данные об изображении
 */
class UnsplashImageInfo
{
public:
    QRectF previewRect() const;

    QString author;
    QString previewUrl;
    QString downloadUrl;

    qreal opacity = 0.0;
    QPixmap previewImage = {};
};

QRectF UnsplashImageInfo::previewRect() const
{
    if (previewImage.isNull()) {
        return {};
    }

    if (previewImage.height() > previewImage.width()) {
        return QRectF(0.0, (previewImage.height() - previewImage.width()) / 2.0,
                      previewImage.width(), previewImage.width());
    } else {
        return QRectF((previewImage.width() - previewImage.height()) / 2.0, 0.0,
                      previewImage.height(), previewImage.height());
    }
}


// ****


class UnsplashImagesView::Implementation
{
public:
    explicit Implementation(UnsplashImagesView* _q);

    /**
     * @brief Загрузить изображения текущей страницы
     */
    void loadCurrentPage();

    /**
     * @brief Обработать список изображений
     */
    void processImagesJson(const QByteArray& _json);

    /**
     * @brief Получить информацию об изображении в заданной точке, а также его область
     */
    QPair<UnsplashImageInfo, QRectF> imageInfoFor(const QPointF& _position) const;


    UnsplashImagesView* q = nullptr;

    /**
     * @brief Запущена ли в данный момент загрузка изображений
     */
    bool isLoadingActive = false;

    /**
     * @brief Ключевики последнего запроса
     */
    QString keywords;

    /**
     * @brief Текущая страница изображений
     */
    int currentImagesPage = 0;

    /**
     * @brief Докрутили ли до последней страницы
     */
    bool isEndPageReached = false;

    /**
     * @brief Ссылка на превьюшку - инфо об изображении
     */
    QHash<QString, UnsplashImageInfo> images;

    /**
     * @brief Сортированный список изображений
     */
    QVector<QString> imagesUrlsOrdered;

    /**
     * @brief  Декорации изображения при клике
     */
    ClickAnimation decorationAnimation;
};

UnsplashImagesView::Implementation::Implementation(UnsplashImagesView* _q)
    : q(_q)
{
    decorationAnimation.setFast(false);
}

void UnsplashImagesView::Implementation::loadCurrentPage()
{
    if (isEndPageReached) {
        return;
    }

    isLoadingActive = true;

    NetworkRequest* request = new NetworkRequest;
    connect(request, &NetworkRequest::downloadProgress, q,
            [this](int _progress) { emit q->imagesLoadingProgressChanged(_progress / 100.0); });
    connect(
        request,
        static_cast<void (NetworkRequest::*)(QByteArray, QUrl)>(&NetworkRequest::downloadComplete),
        q, [this](const QByteArray& _response) { processImagesJson(_response); });
    connect(request, &NetworkRequest::finished, request, &NetworkRequest::deleteLater);

    const QUrl url(QString("https://starc.app/api/services/unsplash/search?text=%1&page=%2")
                       .arg(keywords)
                       .arg(currentImagesPage)
                       .replace(' ', ','));
    request->loadAsync(url);
    emit q->imagesLoadingProgressChanged(0.0);
}

void UnsplashImagesView::Implementation::processImagesJson(const QByteArray& _json)
{
    isLoadingActive = false;

    const auto results = QJsonDocument::fromJson(_json).object()["results"].toArray();
    if (!results.isEmpty()) {
        for (const auto& result : results) {
            const auto imageData = result.toObject();
            UnsplashImageInfo imageInfo;
            imageInfo.author = imageData["user"].toObject()["name"].toString();
            imageInfo.previewUrl = imageData["urls"].toObject()["small"].toString();
            imageInfo.downloadUrl = imageData["links"].toObject()["download_location"].toString();
            images.insert(imageInfo.previewUrl, imageInfo);
            imagesUrlsOrdered.append(imageInfo.previewUrl);

            NetworkRequestLoader::loadAsync(
                imageInfo.previewUrl, q, [this](const QByteArray& _imageData, const QUrl& _url) {
                    auto& imageInfo = images[_url.toString()];
                    imageInfo.previewImage.loadFromData(_imageData);

                    auto opacityAnimation = new QVariantAnimation(q);
                    opacityAnimation->setStartValue(0.0);
                    opacityAnimation->setEndValue(1.0);
                    opacityAnimation->setEasingCurve(QEasingCurve::OutQuad);
                    opacityAnimation->setDuration(360);
                    connect(opacityAnimation, &QVariantAnimation::valueChanged, q,
                            [this, _url](const QVariant& _value) {
                                if (!images.contains(_url.toString())) {
                                    return;
                                }
                                auto& imageInfo = images[_url.toString()];
                                imageInfo.opacity = _value.toReal();
                                q->update();
                            });
                    opacityAnimation->start(QAbstractAnimation::DeleteWhenStopped);
                });
        }

        q->updateGeometry();
    } else {
        isEndPageReached = true;
    }

    emit q->imagesLoaded();
}

QPair<UnsplashImageInfo, QRectF> UnsplashImagesView::Implementation::imageInfoFor(
    const QPointF& _position) const
{
    const int columns = q->width() / kImageWidth + (q->width() % kImageWidth > 0 ? 1 : 0);
    const auto imageSize = q->width() / static_cast<qreal>(columns);
    qreal x = 0.0;
    qreal y = 0.0;
    for (const auto& imageUrl : std::as_const(imagesUrlsOrdered)) {
        const auto& imageInfo = images[imageUrl];
        if (!imageInfo.previewImage.isNull()) {
            const QRectF imageRect(x, y, imageSize, imageSize);
            if (imageRect.contains(_position)) {
                return { imageInfo, imageRect };
            }
        }

        x += imageSize;
        if (x >= q->width()) {
            x = 0.0;
            y += imageSize;
        }
    }

    return {};
}


// ****


UnsplashImagesView::UnsplashImagesView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    auto sizePolicy = this->sizePolicy();
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&UnsplashImagesView::update));
}

UnsplashImagesView::~UnsplashImagesView() = default;

bool UnsplashImagesView::loadImages(const QString& _keywords)
{
    if (d->keywords == _keywords || d->isLoadingActive) {
        return false;
    }

    d->keywords = _keywords;
    d->currentImagesPage = 1;
    d->images.clear();
    d->imagesUrlsOrdered.clear();
    d->isEndPageReached = false;
    d->loadCurrentPage();

    return true;
}

bool UnsplashImagesView::loadNextImagesPage()
{
    if (d->isLoadingActive) {
        return false;
    }

    ++d->currentImagesPage;
    d->loadCurrentPage();

    return true;
}

QSize UnsplashImagesView::sizeHint() const
{
    if (d->images.isEmpty()) {
        return QSize(kImageWidth, kImageWidth);
    }

    return QSize(d->images.size() * kImageWidth, kImageWidth);
}

int UnsplashImagesView::heightForWidth(int _width) const
{
    if (d->images.isEmpty()) {
        return kImageWidth;
    }

    const int columns = _width / kImageWidth + (_width % kImageWidth > 0 ? 1 : 0);
    const int rows = d->images.size() / columns + (d->images.size() % columns > 0 ? 1 : 0);
    const auto imageSize = _width / static_cast<qreal>(columns);
    return rows * imageSize;
}

void UnsplashImagesView::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.fillRect(_event->rect(), backgroundColor());

    if (d->images.isEmpty()) {
        return;
    }

    const int columns = width() / kImageWidth + (width() % kImageWidth > 0 ? 1 : 0);
    const auto imageSize = width() / static_cast<qreal>(columns);
    qreal x = 0.0;
    qreal y = 0.0;
    for (const auto& imageUrl : std::as_const(d->imagesUrlsOrdered)) {
        const auto& imageInfo = d->images[imageUrl];
        if (!imageInfo.previewImage.isNull()) {
            const QRectF imageRect(x, y, imageSize, imageSize);
            if (imageRect.intersects(_event->rect())) {
                painter.setOpacity(imageInfo.opacity);
                painter.drawPixmap(imageRect, imageInfo.previewImage, imageInfo.previewRect());

                if (imageRect == d->decorationAnimation.clipRect()
                    && d->decorationAnimation.state() == ClickAnimation::Running) {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(Ui::DesignSystem::color().accent());
                    painter.setClipRect(d->decorationAnimation.clipRect());
                    painter.setOpacity(d->decorationAnimation.opacity());
                    const auto radius = d->decorationAnimation.radius();
                    painter.drawEllipse(d->decorationAnimation.clickPosition(), radius, radius);
                    painter.setClipRect(QRectF(), Qt::NoClip);
                    painter.setOpacity(1.0);
                }
            }
        }

        x += imageSize;
        if (x >= width()) {
            x = 0.0;
            y += imageSize;
        }
    }
}

void UnsplashImagesView::mousePressEvent(QMouseEvent* _event)
{
    Widget::mousePressEvent(_event);

    const auto imageRect = d->imageInfoFor(_event->pos()).second;
    if (!imageRect.isEmpty()) {
        d->decorationAnimation.setClickPosition(_event->pos());
        d->decorationAnimation.setClipRect(imageRect);
        d->decorationAnimation.setRadiusInterval(0.0, imageRect.width());
        d->decorationAnimation.start();
    }
}

void UnsplashImagesView::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    const auto imageInfo = d->imageInfoFor(_event->pos()).first;
    if (!imageInfo.downloadUrl.isEmpty()) {
        const auto url = QString("https://starc.app/api/services/unsplash/download?url=%1")
                             .arg(imageInfo.downloadUrl);
        NetworkRequestLoader::loadAsync(url, this, [this, imageInfo](const QByteArray& _data) {
            emit imageSelected(QJsonDocument::fromJson(_data).object()["url"].toString(),
                               tr("Photo by %1 on Unsplash.com").arg(imageInfo.author));
        });
    }
}

} // namespace Ui

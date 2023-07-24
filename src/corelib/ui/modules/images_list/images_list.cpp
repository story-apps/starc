#include "images_list.h"

#include "images_list_preview.h"

#include <3rd_party/webloader/src/NetworkRequestLoader.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QFileDialog>
#include <QMimeData>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QSettings>
#include <QVariantAnimation>
#include <QVector>
#include <QtMath>


namespace Ui {

namespace {

const QLatin1String kImagesPathKey("widgets/image-files-path");

constexpr int kInvalidImageIndex = -1;
constexpr int kAddImageIndex = std::numeric_limits<int>::max();

qreal finalImageSize(qreal _size)
{
    return _size > 0 ? _size : DesignSystem::layout().px(90);
}

qreal finalImageSpacing(qreal _spacing)
{
    return _spacing > 0 ? _spacing : DesignSystem::layout().px16();
}

} // namespace


class ImagesList::Implementation
{
public:
    explicit Implementation(ImagesList* _q);

    /**
     * @brief Сколько всего элементов нужно отображать (включая кнопку добавления)
     */
    int totalImages() const;

    /**
     * @brief Шрифт кнопки очистки
     */
    QFont clearButtonFont() const;

    /**
     * @brief Область кнопку удаления в заданной области изображения
     */
    QRectF clearButtonRect(const QRectF& _buttonRect) const;

    /**
     * @brief Получить информацию о кнопке над которой находится курсор мыши
     */
    struct ButtonInfo {
        bool isValid = false;
        bool isAddButton = false;
        bool isRemoveButton = false;
        int imageIndex = kInvalidImageIndex;
        QRectF imageRect = {};
    };
    ButtonInfo buttonInfo(const QPoint& _position) const;

    /**
     * @brief Область отрисовки изображения с заданным индексом
     */
    QRectF imageRect(int _imageIndex) const;

    /**
     * @brief Обновить список изображений для отрисовки
     */
    void updateDisplayImages();

    /**
     * @brief Обновить анимации изображений в соответствии с текущим выбранным изображением
     */
    void updateImagesAnimations();


    ImagesList* q = nullptr;

    /**
     * @brief Возможно ли редактировать изображения
     */
    bool isReadOnly = false;

    /**
     * @brief Видна ли кнопка добавления изображений
     */
    bool isAddButtonVisible = true;

    /**
     * @brief Параметры внешнего вида изображений
     */
    qreal imageSize = 0.0;
    qreal imageSpacing = 0.0;

    /**
     * @brief Список изображений для отображения
     */
    QVector<Domain::DocumentImage> images;
    QVector<QPixmap> displayImages;

    /**
     * @brief Анимации наведения на изображение
     * @note kInvalidImageIndex - анимация для кнопки добавления фотки
     */
    int currentImageIndex = kInvalidImageIndex;
    QHash<int, QVariantAnimation*> imageToOverlayAnimation;

    /**
     * @brief Параметры визуализации затаскивания картинок на виджет мышкой
     */
    bool isDragActive = false;
    QVariantAnimation dragIndicationOpacityAnimation;

    /**
     * @brief Виджет для предспросмотра фотографий
     */
    ImagesListPreview* preview = nullptr;
};

ImagesList::Implementation::Implementation(ImagesList* _q)
    : q(_q)
    , preview(new ImagesListPreview(q->topLevelWidget()))
{
    dragIndicationOpacityAnimation.setStartValue(0.0);
    dragIndicationOpacityAnimation.setEndValue(1.0);
    dragIndicationOpacityAnimation.setDuration(240);
    dragIndicationOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);

    preview->hide();
}

int ImagesList::Implementation::totalImages() const
{
    return images.size() + (isReadOnly || !isAddButtonVisible ? 0 : 1);
}

QFont ImagesList::Implementation::clearButtonFont() const
{
    return finalImageSize(imageSize) > Ui::DesignSystem::layout().px(100)
        ? Ui::DesignSystem::font().iconsMid()
        : Ui::DesignSystem::font().iconsSmall();
}

QRectF ImagesList::Implementation::clearButtonRect(const QRectF& _buttonRect) const
{
    const qreal iconMargin = Ui::DesignSystem::layout().px4();
    const qreal iconSize = finalImageSize(imageSize) > Ui::DesignSystem::layout().px(100)
        ? Ui::DesignSystem::layout().px24()
        : Ui::DesignSystem::layout().px16();
    const qreal left = _buttonRect.right() - iconSize - iconMargin;
    const qreal top = _buttonRect.top() + iconMargin;
    return { left, top, iconSize, iconSize };
}

ImagesList::Implementation::ButtonInfo ImagesList::Implementation::buttonInfo(
    const QPoint& _position) const
{
    const auto size = finalImageSize(imageSize);
    const auto spacing = finalImageSpacing(imageSpacing);
    auto x = q->contentsRect().x();
    auto y = q->contentsRect().y();
    for (int index = 0; index < images.size(); ++index) {
        const QRectF buttonRect(x, y, size, size);
        if (buttonRect.contains(_position)) {
            return { true, false, clearButtonRect(buttonRect).contains(_position), index,
                     buttonRect };
        }

        if (x + size + spacing + size < q->contentsRect().right()) {
            x += size + spacing;
        } else {
            x = q->contentsRect().x();
            y += size + spacing;
        }
    }
    if (!isReadOnly && isAddButtonVisible && QRectF(x, y, size, size).contains(_position)) {
        return { true, true };
    }

    return {};
}

QRectF ImagesList::Implementation::imageRect(int _imageIndex) const
{
    const auto size = finalImageSize(imageSize);
    const auto spacing = finalImageSpacing(imageSpacing);
    auto x = q->contentsRect().x();
    auto y = q->contentsRect().y();
    for (int index = 0; index < images.size(); ++index) {
        if (index == _imageIndex) {
            return QRectF(x, y, size, size);
        }

        if (x + size + spacing + size < q->contentsRect().right()) {
            x += size + spacing;
        } else {
            x = q->contentsRect().x();
            y += size + spacing;
        }
    }

    return {};
}

void ImagesList::Implementation::updateDisplayImages()
{
    displayImages.clear();
    const auto size = finalImageSize(imageSize);
    for (const auto& image : std::as_const(images)) {
        const auto scaledImage = image.image.scaled(size, size, Qt::KeepAspectRatioByExpanding,
                                                    Qt::SmoothTransformation);
        displayImages.append(scaledImage.copy((scaledImage.width() - size) / 2,
                                              (scaledImage.height() - size) / 2, size, size));
    }

    q->update();
}

void ImagesList::Implementation::updateImagesAnimations()
{
    for (auto iter = imageToOverlayAnimation.begin(); iter != imageToOverlayAnimation.end();
         ++iter) {
        if (iter.key() == currentImageIndex) {
            continue;
        }

        if (iter.value()->state() == QVariantAnimation::Running) {
            iter.value()->pause();
            iter.value()->setDirection(QVariantAnimation::Backward);
            iter.value()->resume();
        } else {
            iter.value()->setDirection(QVariantAnimation::Backward);
            iter.value()->start();
        }
    }
}


// ****


ImagesList::ImagesList(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setAcceptDrops(true);
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);

    connect(&d->dragIndicationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImagesList::update));
    connect(d->preview, &ImagesListPreview::currentItemIndexChanged, this, [this](int _imageIndex) {
        const auto imageRect = d->imageRect(_imageIndex);
        d->preview->setCurrentImageSourceRect(
            QRectF(mapTo(topLevelWidget(), imageRect.topLeft().toPoint()), imageRect.size()));
    });
}

ImagesList::~ImagesList() = default;

void ImagesList::setAddButtonVisible(bool _visible)
{
    if (d->isAddButtonVisible == _visible) {
        return;
    }

    d->isAddButtonVisible = _visible;
    updateGeometry();
    update();
}

void ImagesList::setImageSize(qreal _size)
{
    if (qFuzzyCompare(d->imageSize, _size)) {
        return;
    }

    d->imageSize = _size;
    updateGeometry();
    update();
}

void ImagesList::setImageSpacing(qreal _spacing)
{
    if (qFuzzyCompare(d->imageSpacing, _spacing)) {
        return;
    }

    d->imageSpacing = _spacing;
    updateGeometry();
    update();
}

void ImagesList::setImages(const QVector<Domain::DocumentImage>& _images)
{
    d->preview->hidePreview();

    while (!d->imageToOverlayAnimation.isEmpty()) {
        auto animation = d->imageToOverlayAnimation.take(d->imageToOverlayAnimation.begin().key());
        animation->stop();
        animation->deleteLater();
    }

    d->images.clear();
    for (const auto& image : _images) {
        if (image.image.isNull()) {
            continue;
        }

        d->images.append(image);
    }
    d->updateDisplayImages();
    d->preview->setImages(d->images);

    updateGeometry();
    update();
}

void ImagesList::setReadOnly(bool _readOnly)
{
    if (d->isReadOnly == _readOnly) {
        return;
    }

    d->isReadOnly = _readOnly;
    updateGeometry();
    update();
}

void ImagesList::addImages()
{
    QSettings settings;
    const auto imagesFolder = settings.value(kImagesPathKey).toString();
    const auto images = QFileDialog::getOpenFileNames(
        window(), tr("Choose image"), imagesFolder,
        QString("%1 (*.png *.jpeg *.jpg *.bmp *.tiff *.tif *.gif)").arg(tr("Images")));
    if (images.isEmpty()) {
        return;
    }

    settings.setValue(kImagesPathKey, images.constLast());

    QVector<QPixmap> addedImages;
    for (const auto& imagePath : images) {
        QPixmap image(imagePath);
        if (image.isNull()) {
            continue;
        }

        addedImages.append(image);
    }

    if (addedImages.isEmpty()) {
        return;
    }

    emit imagesAdded(addedImages);
}

QSize ImagesList::sizeHint() const
{
    const auto size = finalImageSize(d->imageSize);
    const auto spacing = finalImageSpacing(d->imageSpacing);
    return QRect(0, 0, d->totalImages() * (size + spacing) - spacing, size)
        .marginsAdded(contentsMargins())
        .size();
}

int ImagesList::heightForWidth(int _width) const
{
    const int availableWidth = _width - contentsMargins().left() - contentsMargins().right();
    const auto totalImages = d->totalImages();
    const auto size = finalImageSize(d->imageSize);
    const auto spacing = finalImageSpacing(d->imageSpacing);
    auto x = 0.0;
    int imagesInRow = 1;
    for (; imagesInRow < totalImages; ++imagesInRow) {
        if (x + size + spacing + size < availableWidth) {
            x += size + spacing;
        } else {
            break;
        }
    }
    const auto rowsCount = qCeil(totalImages / static_cast<qreal>(imagesInRow));
    return contentsMargins().top() + rowsCount * (size + spacing) - spacing
        + contentsMargins().bottom();
}

void ImagesList::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем изображения
    //
    const auto size = finalImageSize(d->imageSize);
    const auto spacing = finalImageSpacing(d->imageSpacing);
    const auto radius = DesignSystem::button().borderRadius();
    auto x = contentsRect().x();
    auto y = contentsRect().y();
    for (int index = 0; index < d->images.size(); ++index) {
        const QRectF imageRect(x, y, size, size);
        ImageHelper::drawRoundedImage(painter, imageRect, d->displayImages.at(index), radius);

        const auto imageOverlayAnimationIter = d->imageToOverlayAnimation.find(index);
        if (imageOverlayAnimationIter != d->imageToOverlayAnimation.end()) {
            //
            // ... затемнение сверху изображения
            //
            painter.setOpacity(imageOverlayAnimationIter.value()->currentValue().toReal());
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().shadow());
            painter.drawRoundedRect(imageRect, radius, radius);

            //
            // ... кнопка очистки
            //
            if (!d->isReadOnly) {
                painter.setPen(Ui::DesignSystem::color().onShadow());
                painter.setFont(d->clearButtonFont());
                painter.drawText(d->clearButtonRect(imageRect), Qt::AlignCenter, u8"\U000F0156");
            }

            painter.setOpacity(1.0);
        }

        if (x + size + spacing + size < contentsRect().right()) {
            x += size + spacing;
        } else {
            x = contentsRect().x();
            y += size + spacing;
        }
    }

    //
    // Рисуем кнопку добавления изображений
    //
    if (d->isAddButtonVisible) {
        const QRectF addButtonRect(x, y, size, size);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ColorHelper::nearby(backgroundColor()));
        painter.drawRoundedRect(addButtonRect, radius, radius);
        painter.setPen(ColorHelper::transparent(textColor(),
                                                d->isReadOnly
                                                    ? DesignSystem::inactiveItemOpacity()
                                                    : DesignSystem::inactiveTextOpacity()));
        painter.setFont(DesignSystem::font().iconsBig());
        painter.drawText(addButtonRect, Qt::AlignCenter, u8"\U000F0EDB");

        const auto imageOverlayAnimationIter = d->imageToOverlayAnimation.find(kAddImageIndex);
        if (imageOverlayAnimationIter != d->imageToOverlayAnimation.end()) {
            painter.setOpacity(imageOverlayAnimationIter.value()->currentValue().toReal());
            painter.setPen(Ui::DesignSystem::color().accent());
            painter.drawText(addButtonRect, Qt::AlignCenter, u8"\U000F0EDB");
        }
    }

    //
    // Если в режиме вставки из буфера
    //
    if (!d->isReadOnly
        && (d->isDragActive
            || d->dragIndicationOpacityAnimation.state() == QVariantAnimation::Running)) {
        painter.setOpacity(d->dragIndicationOpacityAnimation.currentValue().toReal());
        //
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().accent());
        const auto cardRect = contentsRect();
        const auto borderRadius = Ui::DesignSystem::card().borderRadius();
        painter.drawRoundedRect(cardRect, borderRadius, borderRadius);
        //
        painter.setPen(Ui::DesignSystem::color().onAccent());
        painter.setBrush(Qt::NoBrush);
        auto iconFont = Ui::DesignSystem::font().iconsForEditors();
        iconFont.setPixelSize(Ui::DesignSystem::layout().px(82));
        if (TextHelper::fineLineSpacing(iconFont) > cardRect.height() / 2) {
            iconFont.setPixelSize(Ui::DesignSystem::layout().px48());
        }
        painter.setFont(iconFont);
        painter.drawText(cardRect, Qt::AlignCenter, u8"\U000F01DA");
        //
        painter.setOpacity(1.0);
        return;
    }
}

void ImagesList::leaveEvent(QEvent* _event)
{
    Widget::leaveEvent(_event);

    //
    // Завершаем анимации любого из выделенных изображений
    //
    d->currentImageIndex = kInvalidImageIndex;
    d->updateImagesAnimations();
}

void ImagesList::mousePressEvent(QMouseEvent* _event)
{
}

void ImagesList::mouseMoveEvent(QMouseEvent* _event)
{
    Widget::mouseMoveEvent(_event);

    //
    // Если навели на изображение или кнопку добавления
    //
    const auto buttonInfo = d->buttonInfo(_event->pos());
    if (buttonInfo.isValid) {
        //
        // ... если курсор наведён на другое изображение (в отличие от последней информации)
        //
        const auto imageIndex = buttonInfo.isAddButton ? kAddImageIndex : buttonInfo.imageIndex;
        if (d->currentImageIndex != imageIndex) {
            //
            // ... запустим анимацию отображения оверлея для выбранного изображения
            //
            d->currentImageIndex = imageIndex;
            auto imageAnimationIter = d->imageToOverlayAnimation.find(d->currentImageIndex);
            QVariantAnimation* animation = nullptr;
            if (imageAnimationIter != d->imageToOverlayAnimation.end()) {
                animation = imageAnimationIter.value();
            } else {
                animation = new QVariantAnimation(this);
                animation->setDuration(240);
                animation->setEasingCurve(QEasingCurve::OutQuad);
                animation->setStartValue(0.0);
                animation->setEndValue(1.0);
                connect(animation, &QVariantAnimation::valueChanged, this,
                        qOverload<>(&ImagesList::update));
                connect(animation, &QVariantAnimation::finished, this,
                        [this, imageIndex, animation] {
                            if (animation->direction() == QVariantAnimation::Forward) {
                                return;
                            }

                            d->imageToOverlayAnimation.remove(imageIndex);
                            animation->deleteLater();
                        });
                d->imageToOverlayAnimation.insert(d->currentImageIndex, animation);
            }

            if (animation->state() == QVariantAnimation::Running) {
                animation->pause();
                animation->setDirection(QVariantAnimation::Forward);
                animation->resume();
            } else {
                animation->start();
            }
        }
    } else {
        d->currentImageIndex = kInvalidImageIndex;
    }
    //
    // ... а для всех изображений, что не под курсором, скроем оверлеи
    //
    d->updateImagesAnimations();

    update();
}

void ImagesList::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    if (!contentsRect().contains(_event->pos())) {
        return;
    }

    const auto buttonInfo = d->buttonInfo(_event->pos());
    if (!buttonInfo.isValid) {
        return;
    }

    //
    // Нажата кнопка добалвения фотографий
    //
    if (buttonInfo.isAddButton) {
        addImages();
    }
    //
    // Нажато изображения
    //
    else {
        //
        // ... кнопка удаления изображения
        //
        if (buttonInfo.isRemoveButton) {
            emit imageRemoved(d->images.at(buttonInfo.imageIndex).uuid);
        }
        //
        // ... предспросмотр изображения
        //
        else {
            d->preview->setParent(topLevelWidget());
            d->preview->showPreview(
                buttonInfo.imageIndex,
                QRectF(mapTo(topLevelWidget(), buttonInfo.imageRect.topLeft().toPoint()),
                       buttonInfo.imageRect.size()));
        }
    }
}

void ImagesList::dragEnterEvent(QDragEnterEvent* _event)
{
    if (d->isReadOnly) {
        _event->ignore();
        return;
    }

    _event->acceptProposedAction();

    d->isDragActive = true;
    d->dragIndicationOpacityAnimation.setDirection(QVariantAnimation::Forward);
    d->dragIndicationOpacityAnimation.start();
}

void ImagesList::dragMoveEvent(QDragMoveEvent* _event)
{
    if (d->isReadOnly) {
        _event->ignore();
        return;
    }

    _event->acceptProposedAction();
}

void ImagesList::dragLeaveEvent(QDragLeaveEvent* _event)
{
    if (d->isReadOnly) {
        _event->ignore();
        return;
    }

    _event->accept();
    d->isDragActive = false;
    d->dragIndicationOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->dragIndicationOpacityAnimation.start();
}

void ImagesList::dropEvent(QDropEvent* _event)
{
    if (d->isReadOnly) {
        _event->ignore();
        return;
    }

    QVector<QPixmap> droppedImages;
    const QMimeData* mimeData = _event->mimeData();
    //
    // Первым делом проверяем картинку
    //
    if (mimeData->hasImage()) {
        droppedImages.append(qvariant_cast<QPixmap>(mimeData->imageData()));
    }
    //
    // Если картинки нет, то смотрим список ссылок, возможно выбраны сразу несколько фотогафий
    //
    else if (mimeData->hasUrls()) {
        for (const auto& url : mimeData->urls()) {
            //
            // ... локальные изображения
            //
            const QString urlString = url.toString().toLower();
            if ((urlString.contains(".png") || urlString.contains(".jpg")
                 || urlString.contains(".jpeg") || urlString.contains(".gif")
                 || urlString.contains(".tiff") || urlString.contains(".bmp")
                 || urlString.contains(".webp"))
                && url.isLocalFile()) {
                droppedImages.append(url.toLocalFile());
            }
            //
            // ... подгружаем картинки с инета
            //
            else {
                //
                // TODO: сделать асинхронно
                //
                const QByteArray pixmapData = NetworkRequestLoader::loadSync(url);
                QPixmap pixmap;
                pixmap.loadFromData(pixmapData);
                droppedImages.append(pixmap);
            }
        }
    }

    //
    // Удалим все обхекты, в которых по-факту нет картинок
    //
    droppedImages.removeAll(QPixmap());
    //
    // ... и если, что-то удалось подгрузить, уведомляем клиентов
    //
    if (!droppedImages.isEmpty()) {
        emit imagesAdded(droppedImages);
    }

    _event->acceptProposedAction();

    d->isDragActive = false;
    d->dragIndicationOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->dragIndicationOpacityAnimation.start();
}

void ImagesList::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->updateDisplayImages();
}

} // namespace Ui

#include "images_list.h"

#include "images_list_preview.h"

#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>

#include <QFileDialog>
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

QSizeF imageSize()
{
    qreal size = DesignSystem::layout().px(90);
    return { size, size };
}

qreal imagesSpacing()
{
    return DesignSystem::layout().px16();
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

    QRectF imageRect(int _imageIndex) const;

    /**
     * @brief Обновить список изображений для отрисовки
     */
    void updateDisplayImages();

    /**
     * @brief Открыть изображение
     */
    void showImage(int _index);

    /**
     * @brief Скрыть последнее изображение
     */
    void hideImage();


    ImagesList* q = nullptr;

    /**
     * @brief Возможно ли редактировать изображения
     */
    bool isReadOnly = false;

    /**
     * @brief Список изображений для отображения
     */
    QVector<Domain::DocumentImage> images;
    QVector<QPixmap> displayImages;

    /**
     * @brief Виджет для предспросмотра фотографий
     */
    ImagesListPreview* preview = nullptr;
};

ImagesList::Implementation::Implementation(ImagesList* _q)
    : q(_q)
    , preview(new ImagesListPreview(q->topLevelWidget()))
{
    preview->hide();
}

int ImagesList::Implementation::totalImages() const
{
    return images.size() + (isReadOnly ? 0 : 1);
}

QRectF ImagesList::Implementation::clearButtonRect(const QRectF& _buttonRect) const
{
    const qreal iconMargin = Ui::DesignSystem::layout().px4();
    const qreal iconSize = Ui::DesignSystem::layout().px16();
    const qreal left = _buttonRect.right() - iconSize - iconMargin;
    const qreal top = _buttonRect.top() + iconMargin;
    return { left, top, iconSize, iconSize };
}

ImagesList::Implementation::ButtonInfo ImagesList::Implementation::buttonInfo(
    const QPoint& _position) const
{
    const auto size = imageSize().width();
    const auto spacing = imagesSpacing();
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
    if (!isReadOnly && QRectF(x, y, size, size).contains(_position)) {
        return { true, true };
    }

    return {};
}

QRectF ImagesList::Implementation::imageRect(int _imageIndex) const
{
    const auto size = imageSize().width();
    const auto spacing = imagesSpacing();
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
    const auto size = imageSize().toSize();
    for (const auto& image : std::as_const(images)) {
        const auto scaledImage
            = image.image.scaled(size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        displayImages.append(scaledImage.copy((scaledImage.width() - size.width()) / 2,
                                              (scaledImage.height() - size.height()) / 2,
                                              size.width(), size.height()));
    }

    q->update();
}


// ****


ImagesList::ImagesList(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setAttribute(Qt::WA_Hover);
    setMouseTracking(true);
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);

    connect(d->preview, &ImagesListPreview::currentItemIndexChanged, this, [this](int _imageIndex) {
        const auto imageRect = d->imageRect(_imageIndex);
        d->preview->setCurrentImageSourceRect(
            QRectF(mapTo(topLevelWidget(), imageRect.topLeft().toPoint()), imageRect.size()));
    });
}

ImagesList::~ImagesList()
{
}

void ImagesList::setImages(const QVector<Domain::DocumentImage>& _images)
{
    d->preview->hidePreview();

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

QSize ImagesList::sizeHint() const
{
    const auto size = imageSize();
    return QRect(0, 0, d->totalImages() * (size.width() + imagesSpacing()) - imagesSpacing(),
                 size.height())
        .marginsAdded(contentsMargins())
        .size();
}

int ImagesList::heightForWidth(int _width) const
{
    const int availableWidth = _width - contentsMargins().left() - contentsMargins().right();
    const auto totalImages = d->totalImages();
    const auto size = imageSize().width();
    const auto spacing = imagesSpacing();
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
    const auto size = imageSize().width();
    const auto spacing = imagesSpacing();
    const auto radius = DesignSystem::button().borderRadius();
    const auto cursorPos = underMouse() ? mapFromGlobal(QCursor::pos()) : QPoint();
    auto x = contentsRect().x();
    auto y = contentsRect().y();
    for (int index = 0; index < d->images.size(); ++index) {
        const QRectF imageRect(x, y, size, size);
        ImageHelper::drawRoundedImage(painter, imageRect, d->displayImages.at(index), radius);

        if (imageRect.contains(cursorPos)) {
            //
            // ... затемнение сверху изображения
            //
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().shadow());
            painter.drawRoundedRect(imageRect, radius, radius);

            //
            // ... кнопка очистки
            //
            if (!d->isReadOnly) {
                painter.setPen(Ui::DesignSystem::color().onShadow());
                painter.setFont(Ui::DesignSystem::font().iconsSmall());
                painter.drawText(d->clearButtonRect(imageRect), Qt::AlignCenter, u8"\U000F0156");
            }
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
    if (!d->isReadOnly) {
        const QRectF addButtonRect(x, y, size, size);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ColorHelper::nearby(backgroundColor()));
        painter.drawRoundedRect(addButtonRect, radius, radius);
        painter.setPen(ColorHelper::transparent(textColor(), DesignSystem::inactiveTextOpacity()));
        painter.setFont(DesignSystem::font().iconsBig());
        painter.drawText(addButtonRect, Qt::AlignCenter, u8"\U000F0EDB");
    }
}

void ImagesList::mousePressEvent(QMouseEvent* _event)
{
}

void ImagesList::mouseMoveEvent(QMouseEvent* _event)
{
    Widget::mouseMoveEvent(_event);

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

void ImagesList::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    d->updateDisplayImages();
}

} // namespace Ui

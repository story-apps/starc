#include "images_list.h"

#include "images_list_preview.h"

#include <3rd_party/webloader/src/NetworkRequestLoader.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QApplication>
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
     * @brief Итоговый размер изображения
     */
    QSizeF finalImageSize() const;

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

    /**
     * @brief Плавно раздвинуть изображения по сторонам от текущего места вставки
     *
     * Исходное изображение при расчёте соседей не учитывается: его место остаётся свободным,
     * пока само изображение следует за курсором.
     */
    void updateInsertionAnimation();

    /**
     * @brief Запустить общую анимацию перемещения изображений к заданным позициям
     *
     * Текущие визуальные позиции используются как начальные, поэтому при смене места вставки
     * уже запущенная анимация продолжается без скачка.
     */
    void startImagesMoveAnimation(const QVector<QPointF>& _targetPositions);

    /**
     * @brief Зафиксировать новый порядок после завершения анимации отпускания
     */
    void finishReordering();


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
     * @brief Можно ли менять порядок изображений перетаскиванием мышью
     */
    bool isImagesReorderingEnabled = false;

    /**
     * @brief Параметры внешнего вида изображений
     */
    QSizeF imageSize;
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
     * @brief Параметры изменения порядка изображений мышью
     */
    QPoint dragStartPosition; //!< Точка нажатия для проверки системного порога перетаскивания
    QPoint dragPosition; //!< Текущее положение курсора внутри виджета
    QPointF dragImageOffset; //!< Смещение точки захвата относительно левого верхнего угла картинки
    int draggedImageIndex = kInvalidImageIndex; //!< Индекс захваченного изображения
    int targetImageIndex = kInvalidImageIndex; //!< Индекс изображения после будущей вставки
    bool isImageReorderingActive = false; //!< Идёт ли сейчас интерактивное перетаскивание
    bool isDropAnimationActive = false; //!< Летят ли изображения в финальные позиции
    bool isOrderChanged = false; //!< Отличается ли итоговый порядок от исходного

    /**
     * @brief Визуальные координаты изображений во время перетаскивания и анимации отпускания
     *
     * Все три вектора индексируются по исходному индексу изображения. Это позволяет не менять
     * модели @a images и @a displayImages до полного завершения анимации.
     */
    QVector<QPointF> imagePositions;
    QVector<QPointF> imageStartPositions;
    QVector<QPointF> imageTargetPositions;
    QVariantAnimation imagesMoveAnimation; //!< Общий прогресс перемещения всех изображений

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

    imagesMoveAnimation.setDuration(220);
    imagesMoveAnimation.setEasingCurve(QEasingCurve::OutQuad);
    imagesMoveAnimation.setStartValue(0.0);
    imagesMoveAnimation.setEndValue(1.0);
    connect(&imagesMoveAnimation, &QVariantAnimation::valueChanged, q,
            [this](const QVariant& _value) {
                const auto progress = _value.toReal();
                for (int index = 0; index < imagePositions.size(); ++index) {
                    //
                    // Пока кнопка мыши зажата, позицию захваченной картинки задаёт курсор. После
                    // отпускания флаг isDropAnimationActive разрешает анимировать и её тоже.
                    //
                    if (isImageReorderingActive && !isDropAnimationActive
                        && index == draggedImageIndex) {
                        continue;
                    }
                    imagePositions[index] = imageStartPositions[index]
                        + (imageTargetPositions[index] - imageStartPositions[index]) * progress;
                }
                q->update();
            });
    connect(&imagesMoveAnimation, &QVariantAnimation::finished, q, [this] {
        //
        // Перемещения при выборе новой точки вставки не меняют данные. Фиксируем порядок только
        // после отдельной анимации, запущенной отпусканием кнопки мыши.
        //
        if (isDropAnimationActive) {
            finishReordering();
        }
    });
}

QSizeF ImagesList::Implementation::finalImageSize() const
{
    const qreal finalWidth
        = imageSize.width() > 0 ? imageSize.width() : DesignSystem::layout().px(90);
    const qreal finalHeight
        = imageSize.height() > 0 ? imageSize.height() : DesignSystem::layout().px(90);
    return { finalWidth, finalHeight };
}

int ImagesList::Implementation::totalImages() const
{
    return images.size() + (isReadOnly || !isAddButtonVisible ? 0 : 1);
}

QFont ImagesList::Implementation::clearButtonFont() const
{
    return finalImageSize().width() > Ui::DesignSystem::layout().px(100)
            && finalImageSize().height() > Ui::DesignSystem::layout().px(100)
        ? Ui::DesignSystem::font().iconsMid()
        : Ui::DesignSystem::font().iconsSmall();
}

QRectF ImagesList::Implementation::clearButtonRect(const QRectF& _buttonRect) const
{
    const qreal iconMargin = Ui::DesignSystem::layout().px4();
    const qreal iconSize = finalImageSize().width() > Ui::DesignSystem::layout().px(100)
            && finalImageSize().height() > Ui::DesignSystem::layout().px(100)
        ? Ui::DesignSystem::layout().px24()
        : Ui::DesignSystem::layout().px16();
    const qreal left = _buttonRect.right() - iconSize - iconMargin;
    const qreal top = _buttonRect.top() + iconMargin;
    return { left, top, iconSize, iconSize };
}

ImagesList::Implementation::ButtonInfo ImagesList::Implementation::buttonInfo(
    const QPoint& _position) const
{
    const auto size = finalImageSize();
    const auto spacing = finalImageSpacing(imageSpacing);
    auto x = q->contentsRect().x();
    auto y = q->contentsRect().y();
    for (int index = 0; index < images.size(); ++index) {
        const QRectF buttonRect(x, y, size.width(), size.height());
        if (buttonRect.contains(_position)) {
            return { true, false, clearButtonRect(buttonRect).contains(_position), index,
                     buttonRect };
        }

        if (x + size.width() + spacing + size.width() < q->contentsRect().right()) {
            x += size.width() + spacing;
        } else {
            x = q->contentsRect().x();
            y += size.height() + spacing;
        }
    }
    if (!isReadOnly && isAddButtonVisible
        && QRectF(x, y, size.width(), size.height()).contains(_position)) {
        return { true, true };
    }

    return {};
}

QRectF ImagesList::Implementation::imageRect(int _imageIndex) const
{
    const auto size = finalImageSize();
    const auto spacing = finalImageSpacing(imageSpacing);
    auto x = q->contentsRect().x();
    auto y = q->contentsRect().y();
    for (int index = 0; index < images.size(); ++index) {
        if (index == _imageIndex) {
            return QRectF(x, y, size.width(), size.height());
        }

        if (x + size.width() + spacing + size.width() < q->contentsRect().right()) {
            x += size.width() + spacing;
        } else {
            x = q->contentsRect().x();
            y += size.height() + spacing;
        }
    }

    return {};
}

void ImagesList::Implementation::updateDisplayImages()
{
    displayImages.clear();
    const auto size = finalImageSize();
    for (const auto& image : std::as_const(images)) {
        const auto scaledImage = image.image.scaled(
            size.width(), size.height(), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        displayImages.append(scaledImage.copy((scaledImage.width() - size.width()) / 2,
                                              (scaledImage.height() - size.height()) / 2,
                                              size.width(), size.height()));
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

void ImagesList::Implementation::startImagesMoveAnimation(const QVector<QPointF>& _targetPositions)
{
    //
    // stop() сохраняет уже рассчитанные imagePositions. Они становятся началом следующего
    // перехода, благодаря чему частые перемещения курсора не возвращают картинки рывком назад.
    //
    imagesMoveAnimation.stop();
    imageStartPositions = imagePositions;
    imageTargetPositions = _targetPositions;
    imagesMoveAnimation.start();
}

void ImagesList::Implementation::updateInsertionAnimation()
{
    //
    // По умолчанию каждая картинка стремится вернуться в свою обычную ячейку списка.
    //
    QVector<QPointF> targetPositions;
    targetPositions.reserve(images.size());
    for (int index = 0; index < images.size(); ++index) {
        targetPositions.append(imageRect(index).topLeft());
    }

    //
    // Захваченную картинку исключаем из виртуального порядка. Так её исходная ячейка остаётся
    // пустой, а индексы previousIndex и nextIndex описывают именно границу будущей вставки.
    //
    QVector<int> remainingImages;
    for (int index = 0; index < images.size(); ++index) {
        if (index != draggedImageIndex) {
            remainingImages.append(index);
        }
    }

    const int previousIndex
        = targetImageIndex > 0 ? remainingImages.at(targetImageIndex - 1) : kInvalidImageIndex;
    const int nextIndex = targetImageIndex < remainingImages.size()
        ? remainingImages.at(targetImageIndex)
        : kInvalidImageIndex;
    //
    // Раздвигаем соседей вдоль линии между центрами их ячеек. Это работает и при переносе через
    // границу строки, где направление перестаёт быть строго горизонтальным.
    //
    QPointF direction(1.0, 0.0);
    if (previousIndex != kInvalidImageIndex && nextIndex != kInvalidImageIndex) {
        direction = imageRect(nextIndex).center() - imageRect(previousIndex).center();
        const auto length = std::hypot(direction.x(), direction.y());
        if (!qFuzzyIsNull(length)) {
            direction /= length;
        }
    }
    //
    // Небольшого отступа достаточно, чтобы обозначить место вставки, не создавая полноценную
    // пустую ячейку и не перестраивая весь список до отпускания мыши.
    //
    const auto insertionOffset = DesignSystem::layout().px8();
    if (previousIndex != kInvalidImageIndex) {
        targetPositions[previousIndex] -= direction * insertionOffset;
    }
    if (nextIndex != kInvalidImageIndex) {
        targetPositions[nextIndex] += direction * insertionOffset;
    }
    startImagesMoveAnimation(targetPositions);
}

void ImagesList::Implementation::finishReordering()
{
    //
    // До этой точки менялись только координаты отрисовки. Теперь визуальная анимация закончилась,
    // поэтому можно безопасно синхронно переставить исходные данные и готовые миниатюры.
    //
    const bool orderChanged = isOrderChanged;
    if (orderChanged) {
        images.move(draggedImageIndex, targetImageIndex);
        displayImages.move(draggedImageIndex, targetImageIndex);
    }
    preview->setImages(images);

    QVector<QUuid> imageOrder;
    for (const auto& image : std::as_const(images)) {
        imageOrder.append(image.uuid);
    }

    //
    // Возвращаем виджет в обычное состояние до отправки сигнала: обработчик сигнала может сразу
    // передать в setImages новый список, и он уже не должен считаться перетаскиваемым.
    //
    draggedImageIndex = kInvalidImageIndex;
    targetImageIndex = kInvalidImageIndex;
    isImageReorderingActive = false;
    isDropAnimationActive = false;
    isOrderChanged = false;
    imagePositions.clear();
    q->unsetCursor();
    q->update();

    if (orderChanged) {
        emit q->imagesOrderChanged(imageOrder);
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
    setImageSize(_size, _size);
}

void ImagesList::setImageSize(qreal _width, qreal _height)
{
    if (qFuzzyCompare(d->imageSize.width(), _width)
        && qFuzzyCompare(d->imageSize.height(), _height)) {
        return;
    }

    d->imageSize = { _width, _height };
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

void ImagesList::setImagesReorderingEnabled(bool _enabled)
{
    if (d->isImagesReorderingEnabled == _enabled) {
        return;
    }

    d->isImagesReorderingEnabled = _enabled;

    //
    // Настройка может измениться прямо во время перемещения. Поэтому останавливаем анимацию,
    // отбрасываем только временные координаты и оставляем исходный порядок неизменным
    //
    if (!_enabled) {
        d->draggedImageIndex = kInvalidImageIndex;
        d->targetImageIndex = kInvalidImageIndex;
        d->isImageReorderingActive = false;
        d->isDropAnimationActive = false;
        d->imagesMoveAnimation.stop();
        d->imagePositions.clear();
        unsetCursor();
        update();
    }
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
    const auto size = d->finalImageSize();
    const auto spacing = finalImageSpacing(d->imageSpacing);
    return QRect(0, 0, d->totalImages() * (size.width() + spacing) - spacing, size.height())
        .marginsAdded(contentsMargins())
        .size();
}

int ImagesList::heightForWidth(int _width) const
{
    const int availableWidth = _width - contentsMargins().left() - contentsMargins().right();
    const auto totalImages = d->totalImages();
    const auto size = d->finalImageSize();
    const auto spacing = finalImageSpacing(d->imageSpacing);
    auto x = 0.0;
    int imagesInRow = 1;
    for (; imagesInRow < totalImages; ++imagesInRow) {
        if (x + size.width() + spacing + size.width() < availableWidth) {
            x += size.width() + spacing;
        } else {
            break;
        }
    }
    const auto rowsCount = qCeil(totalImages / static_cast<qreal>(imagesInRow));
    return contentsMargins().top() + rowsCount * (size.height() + spacing) - spacing
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
    const auto size = d->finalImageSize();
    const auto spacing = finalImageSpacing(d->imageSpacing);
    const auto radius = DesignSystem::button().borderRadius();
    auto x = contentsRect().x();
    auto y = contentsRect().y();
    for (int imageIndex = 0; imageIndex < d->images.size(); ++imageIndex) {
        //
        // ... во время перемещения берём координаты из анимируемого массива. В обычном режиме
        //     продолжаем использовать последовательную раскладку, не добавляя накладных расходов
        //
        const bool isDraggedImage
            = d->isImageReorderingActive && imageIndex == d->draggedImageIndex;
        const QPointF imagePosition
            = d->isImageReorderingActive ? d->imagePositions.at(imageIndex) : QPointF(x, y);
        const QRectF imageRect(imagePosition, size);
        //
        // ... пока изображение находится под курсором, его исходная ячейка остаётся пустой, а после
        //     отпускания оно рисуется в общем цикле и летит к целевой ячейке вместе с соседями
        //
        if (!isDraggedImage || d->isDropAnimationActive) {
            ImageHelper::drawRoundedImage(painter, imageRect,
                                          d->displayImages.at(imageIndex)
                                              .scaled(imageRect.size().toSize(),
                                                      Qt::KeepAspectRatioByExpanding,
                                                      Qt::FastTransformation),
                                          radius);
        }

        const auto imageOverlayAnimationIter = d->imageToOverlayAnimation.find(imageIndex);
        if (!d->isImageReorderingActive
            && imageOverlayAnimationIter != d->imageToOverlayAnimation.end()) {

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

        if (x + size.width() + spacing + size.width() < contentsRect().right()) {
            x += size.width() + spacing;
        } else {
            x = contentsRect().x();
            y += size.height() + spacing;
        }
    }

    //
    // Рисуем кнопку добавления изображений
    //
    if (d->isAddButtonVisible) {
        const QRectF addButtonRect(x, y, size.width(), size.height());
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
        painter.setOpacity(1.0);
    }

    //
    // Перетаскиваемое изображение остаётся под курсором, пока остальные изображения
    // показывают место, в которое оно будет вставлено.
    //
    if (d->isImageReorderingActive && !d->isDropAnimationActive) {
        const QRectF draggedImageRect(QPointF(d->dragPosition) - d->dragImageOffset, size);
        painter.setPen(Qt::NoPen);
        painter.setBrush(ColorHelper::transparent(DesignSystem::color().shadow(), 0.4));
        painter.drawRoundedRect(draggedImageRect.translated(0, DesignSystem::layout().px4()),
                                radius, radius);
        ImageHelper::drawRoundedImage(painter, draggedImageRect,
                                      d->displayImages.at(d->draggedImageIndex)
                                          .scaled(draggedImageRect.size().toSize(),
                                                  Qt::KeepAspectRatioByExpanding,
                                                  Qt::FastTransformation),
                                      radius);
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
    Widget::mousePressEvent(_event);

    //
    // Пока предыдущая картинка долетает до нового места, не разрешаем начать новое перемещение
    //
    if (d->isDropAnimationActive) {
        return;
    }

    d->draggedImageIndex = kInvalidImageIndex;
    d->targetImageIndex = kInvalidImageIndex;
    d->isImageReorderingActive = false;
    if (!d->isImagesReorderingEnabled || d->isReadOnly || _event->button() != Qt::LeftButton) {
        return;
    }

    //
    // Кнопки добавления и удаления сохраняют обычное поведение и никогда не становятся
    // источником перестановки.
    //
    const auto buttonInfo = d->buttonInfo(_event->pos());
    if (!buttonInfo.isValid || buttonInfo.isAddButton || buttonInfo.isRemoveButton) {
        return;
    }

    //
    // Запоминаем не только изображение, но и локальную точку захвата, чтобы миниатюра не
    // перескакивала центром под курсор после начала движения
    //
    d->draggedImageIndex = buttonInfo.imageIndex;
    d->targetImageIndex = buttonInfo.imageIndex;
    d->dragStartPosition = _event->pos();
    d->dragPosition = _event->pos();
    d->dragImageOffset = _event->pos() - buttonInfo.imageRect.topLeft();
}

void ImagesList::mouseMoveEvent(QMouseEvent* _event)
{
    Widget::mouseMoveEvent(_event);

    if (d->isDropAnimationActive) {
        return;
    }

    if (d->draggedImageIndex != kInvalidImageIndex && (_event->buttons() & Qt::LeftButton)) {
        if (!d->isImageReorderingActive
            && (_event->pos() - d->dragStartPosition).manhattanLength()
                >= QApplication::startDragDistance()) {
            //
            // До преодоления startDragDistance событие остаётся обычным кликом и по отпусканию
            // откроет предпросмотр. Только здесь переключаем виджет в режим перестановки.
            //
            d->isImageReorderingActive = true;
            d->preview->hidePreview();
            d->currentImageIndex = kInvalidImageIndex;
            d->updateImagesAnimations();

            //
            // Начинаем с фактических координат текущей раскладки. Захваченное изображение сразу
            // переносим под курсор, а его исходная позиция благодаря paintEvent остаётся пустой.
            //
            d->imagePositions.clear();
            for (int index = 0; index < d->images.size(); ++index) {
                d->imagePositions.append(d->imageRect(index).topLeft());
            }
            d->imagePositions[d->draggedImageIndex] = QPointF(_event->pos()) - d->dragImageOffset;
            d->updateInsertionAnimation();
            setCursor(Qt::ClosedHandCursor);
        }

        if (d->isImageReorderingActive) {
            //
            // Захваченная миниатюра следует за каждым событием мыши напрямую, без запаздывания.
            // Анимация используется только для окружающих её изображений.
            //
            d->dragPosition = _event->pos();
            d->imagePositions[d->draggedImageIndex] = QPointF(_event->pos()) - d->dragImageOffset;

            //
            // Наведение на кнопку добавления трактуется как вставка в самый конец. Сама кнопка
            // при этом не участвует в перестановке и по-прежнему остаётся последним элементом.
            //
            const auto buttonInfo = d->buttonInfo(_event->pos());
            const int targetImageIndex
                = buttonInfo.isAddButton ? d->images.size() - 1 : buttonInfo.imageIndex;
            if (buttonInfo.isValid && d->targetImageIndex != targetImageIndex) {
                d->targetImageIndex = targetImageIndex;
                d->updateInsertionAnimation();
            }
            update();
            return;
        }
    }

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

    if (d->isImageReorderingActive) {
        //
        // Фиксируем последнюю позицию курсора: между последним mouseMove и mouseRelease могло быть
        // небольшое движение, которое тоже должно стать началом финального полёта.
        //
        d->imagePositions[d->draggedImageIndex] = QPointF(_event->pos()) - d->dragImageOffset;
        QVector<QPointF> targetPositions;
        targetPositions.reserve(d->images.size());
        for (int index = 0; index < d->images.size(); ++index) {
            //
            // Рассчитываем конечную ячейку для каждого элемента, не меняя сами массивы. Элементы
            // между исходной и целевой позициями сдвигаются на одну ячейку навстречу
            // освободившемуся месту, а захваченное изображение направляется непосредственно в
            // targetImageIndex.
            //
            int finalIndex = index;
            if (index == d->draggedImageIndex) {
                finalIndex = d->targetImageIndex;
            } else if (d->draggedImageIndex < d->targetImageIndex && index > d->draggedImageIndex
                       && index <= d->targetImageIndex) {
                --finalIndex;
            } else if (d->draggedImageIndex > d->targetImageIndex && index >= d->targetImageIndex
                       && index < d->draggedImageIndex) {
                ++finalIndex;
            }
            targetPositions.append(d->imageRect(finalIndex).topLeft());
        }

        //
        // Даже если пользователь вернул картинку на исходное место, проигрываем короткий возврат
        // из-под курсора в ячейку, но после него не переставляем данные и не отправляем сигнал.
        //
        d->isOrderChanged = d->targetImageIndex != d->draggedImageIndex;
        d->isDropAnimationActive = true;
        d->startImagesMoveAnimation(targetPositions);
        return;
    }

    d->draggedImageIndex = kInvalidImageIndex;
    d->targetImageIndex = kInvalidImageIndex;

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
    for (int index = droppedImages.size() - 1; index >= 0; --index) {
        if (droppedImages[index].isNull()) {
            droppedImages.removeAt(index);
        }
    }
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

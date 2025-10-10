#include "image_card.h"

#include "image_cropping_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/dialog/dialog.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QFileDialog>
#include <QHelpEvent>
#include <QMimeData>
#include <QPainter>
#include <QResizeEvent>
#include <QSettings>
#include <QToolTip>
#include <QVariantAnimation>

#include <NetworkRequestLoader.h>


namespace {
const QSizeF kCoverSize(5.4, 8);
const QString kImagesPathKey = QLatin1String("widgets/image-files-path");
} // namespace

class ImageCard::Implementation
{
public:
    explicit Implementation(ImageCard* _parent);

    /**
     * @brief Закешировать масштабированную версию изображения для отображения на виджете
     */
    void prepareImageForDisplaing(const QSize& _size);

    /**
     * @brief Выбрать изображение из файла на компьютере
     */
    void chooseImageFromFile();

    /**
     * @brief Обрезать выбранное изображение
     */
    void cropImage(const QPixmap& _image);

    /**
     * @brief Получить область отображения иконки очистки изображения
     */
    QRectF clearButtonRect() const;

    /**
     * @brief Находится ли мышка над кнопкой очистки изображения
     */
    bool isInsideClearButton(const QPoint _position) const;

    /**
     * @brief Подготовить контекстное меню к отображению
     */
    void prepareContextMenu();

    //
    // Данные
    //

    ImageCard* q = nullptr;

    bool isReadOnly = false;
    bool isDragActive = false;
    QString decorationIcon = u8"\U000F0513";
    QString emptyImageText;
    QString imageText;
    QString clearImageQuestion;
    QString imageCroppingText;
    struct {
        QPixmap source;
        QPixmap display;
    } image;

    ContextMenu* contextMenu = nullptr;
    QAction* changeImageAction = nullptr;
    QAction* clearImageAction = nullptr;
    QAction* copyImageAction = nullptr;
    QAction* pasteImageAction = nullptr;

    QVariantAnimation dragIndicationOpacityAnimation;
    QVariantAnimation decorationColorAnimation;
    QVariantAnimation overlayOpacityAnimation;
};

ImageCard::Implementation::Implementation(ImageCard* _parent)
    : q(_parent)
    , contextMenu(new ContextMenu(_parent))
    , changeImageAction(new QAction(contextMenu))
    , clearImageAction(new QAction(contextMenu))
    , copyImageAction(new QAction(contextMenu))
    , pasteImageAction(new QAction(contextMenu))
{
    changeImageAction->setIconText(u8"\U000F0770");
    copyImageAction->setIconText(u8"\U000F018F");
    copyImageAction->setSeparator(true);
    pasteImageAction->setIconText(u8"\U000F0192");
    clearImageAction->setIconText(u8"\U000F01B4");

    dragIndicationOpacityAnimation.setStartValue(0.0);
    dragIndicationOpacityAnimation.setEndValue(1.0);
    dragIndicationOpacityAnimation.setDuration(240);
    dragIndicationOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationColorAnimation.setDuration(240);
    decorationColorAnimation.setEasingCurve(QEasingCurve::OutQuad);
    overlayOpacityAnimation.setDuration(240);
    overlayOpacityAnimation.setEasingCurve(QEasingCurve::OutQuad);
    overlayOpacityAnimation.setStartValue(0.0);
    overlayOpacityAnimation.setEndValue(1.0);
}

void ImageCard::Implementation::prepareImageForDisplaing(const QSize& _size)
{
    if (image.source.isNull()) {
        return;
    }

    //
    // Добавляем небольшую дельту, чтобы постер занимал всё доступное пространство
    //
    const auto sizeCorrected
        = QRect({}, _size)
              .marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins())
              .size();
    if (image.display.size() == sizeCorrected) {
        return;
    }

    image.display = image.source.scaled(sizeCorrected, Qt::KeepAspectRatioByExpanding,
                                        Qt::SmoothTransformation);
}

void ImageCard::Implementation::chooseImageFromFile()
{
    QSettings settings;
    const auto imagesFolder = settings.value(kImagesPathKey).toString();
    const QString imagePath = QFileDialog::getOpenFileName(
        q->window(), tr("Choose image"), imagesFolder,
        QString("%1 (*.png *.jpeg *.jpg *.bmp *.tiff *.tif *.gif)").arg(tr("Images")));
    if (imagePath.isEmpty()) {
        return;
    }
    settings.setValue(kImagesPathKey, imagePath);

    QPixmap image(imagePath);
    if (image.isNull()) {
        return;
    }

    cropImage(image);
}

void ImageCard::Implementation::cropImage(const QPixmap& _image)
{
    auto dlg = new ImageCroppingDialog(q->window());
    dlg->setImage(_image);
    dlg->setImageProportion(q->size());
    dlg->setImageProportionFixed(true);
    dlg->setImageCroppingText(imageCroppingText);
    connect(dlg, &ImageCroppingDialog::disappeared, dlg, &ImageCroppingDialog::deleteLater);
    connect(dlg, &ImageCroppingDialog::imageSelected, q, &ImageCard::setImage);

    dlg->showDialog();
}

QRectF ImageCard::Implementation::clearButtonRect() const
{
    const qreal iconMargin = Ui::DesignSystem::layout().px12();
    const qreal iconSize = Ui::DesignSystem::layout().px24();
    const qreal left
        = q->width() - Ui::DesignSystem::card().shadowMargins().right() - iconSize - iconMargin;
    const qreal top = Ui::DesignSystem::card().shadowMargins().top() + iconMargin;
    return { left, top, iconSize, iconSize };
}

bool ImageCard::Implementation::isInsideClearButton(const QPoint _position) const
{
    return clearButtonRect().contains(_position);
}

void ImageCard::Implementation::prepareContextMenu()
{
    if (contextMenu->actions().isEmpty()) {
        contextMenu->setActions(q->contextMenuActions());
    }

    const auto hasImage = !image.source.isNull();
    copyImageAction->setVisible(hasImage);
    clearImageAction->setVisible(hasImage);
}


// ****


ImageCard::ImageCard(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    setAcceptDrops(true);
    setAttribute(Qt::WA_Hover);
    setContextMenuPolicy(Qt::CustomContextMenu);

    connect(this, &ImageCard::customContextMenuRequested, this, [this](const QPoint& _pos) {
        //
        // Настроим контекстное меню
        //
        d->prepareContextMenu();
        //
        // ... и отобразим
        //
        d->contextMenu->showContextMenu(mapToGlobal(_pos));

        //
        // При этом, запускаем анимацию ухода курсора из карточки изображения
        //
        d->decorationColorAnimation.setDirection(QVariantAnimation::Backward);
        d->decorationColorAnimation.start();
        d->overlayOpacityAnimation.setDirection(QVariantAnimation::Backward);
        d->overlayOpacityAnimation.start();
    });
    connect(d->changeImageAction, &QAction::triggered, this, [this] { d->chooseImageFromFile(); });
    connect(d->clearImageAction, &QAction::triggered, this, [this] {
        const int kNoButtonId = 0;
        const int kYesButtonId = 1;
        auto dialog = new Dialog(topLevelWidget());
        dialog->showDialog({}, d->clearImageQuestion,
                           { { kNoButtonId, tr("No"), Dialog::RejectButton },
                             { kYesButtonId, tr("Yes, delete"), Dialog::AcceptButton } });
        QObject::connect(dialog, &Dialog::finished,
                         [this, kYesButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
                             dialog->hideDialog();

                             if (_buttonInfo.id == kYesButtonId) {
                                 setImage({});
                             }
                         });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
    });
    connect(d->copyImageAction, &QAction::triggered, this,
            [this] { QApplication::clipboard()->setPixmap(d->image.source); });
    connect(d->pasteImageAction, &QAction::triggered, this, [this] {
        const auto image = QApplication::clipboard()->pixmap();
        if (!image.isNull()) {
            d->cropImage(image);
        }
    });
    connect(&d->dragIndicationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImageCard::update));
    connect(&d->decorationColorAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImageCard::update));
    connect(&d->overlayOpacityAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&ImageCard::update));
}

ImageCard::~ImageCard() = default;

void ImageCard::setDecorationIcon(const QString& _icon)
{
    if (d->decorationIcon == _icon) {
        return;
    }

    d->decorationIcon = _icon;
    if (!d->image.source.isNull()) {
        return;
    }

    update();
}

void ImageCard::setSupportingText(const QString& _emptyImageText, const QString& _imageText,
                                  const QString& _clearImageQuestion)
{
    if (d->emptyImageText == _emptyImageText && d->imageText == _imageText
        && d->clearImageQuestion == _clearImageQuestion) {
        return;
    }

    d->emptyImageText = _emptyImageText;
    d->imageText = _imageText;
    d->clearImageQuestion = _clearImageQuestion;

    if (d->image.source.isNull() || underMouse()) {
        update();
    }
}

void ImageCard::setImageCroppingText(const QString& _text)
{
    if (d->imageCroppingText == _text) {
        return;
    }

    d->imageCroppingText = _text;
}

QPixmap ImageCard::image() const
{
    return d->image.source;
}

void ImageCard::setImage(const QPixmap& _image)
{
    if ((d->image.source.isNull() && _image.isNull())
        || (!d->image.source.isNull() && !_image.isNull()
            && d->image.source.cacheKey() == _image.cacheKey())) {
        return;
    }

    d->image.source = _image;
    d->image.display = {};
    d->prepareImageForDisplaing(size());
    update();

    emit imageChanged(d->image.source);
}

void ImageCard::cropImage(const QPixmap& _image)
{
    d->cropImage(_image);
}

bool ImageCard::isReadOnly() const
{
    return d->isReadOnly;
}

void ImageCard::setReadOnly(bool _readOnly)
{
    if (d->isReadOnly == _readOnly) {
        return;
    }

    d->isReadOnly = _readOnly;
    d->changeImageAction->setEnabled(!d->isReadOnly);
    d->copyImageAction->setEnabled(!d->isReadOnly);
    d->pasteImageAction->setEnabled(!d->isReadOnly);
    d->clearImageAction->setEnabled(!d->isReadOnly);
    processReadOnlyChange();
}

QVector<QAction*> ImageCard::contextMenuActions() const
{
    return {
        d->changeImageAction,
        d->copyImageAction,
        d->pasteImageAction,
        d->clearImageAction,
    };
}

void ImageCard::processReadOnlyChange()
{
}

bool ImageCard::event(QEvent* _event)
{
    //
    // Показываем тултип над кнопкой очистки изображения, только если задано само изображение
    // и мы сейчас не находимся в режиме drag&drop
    //
    if (_event->type() == QEvent::ToolTip && !d->image.display.isNull() && !d->isDragActive
        && d->dragIndicationOpacityAnimation.state() != QVariantAnimation::Running) {
        QHelpEvent* event = static_cast<QHelpEvent*>(_event);
        if (d->isInsideClearButton(event->pos())) {
            QToolTip::showText(event->globalPos(), tr("Delete image"));
        } else {
            QToolTip::hideText();
        }
    }

    return Card::event(_event);
}

void ImageCard::paintEvent(QPaintEvent* _event)
{
    Card::paintEvent(_event);

    QPainter painter(this);

    //
    // Если не задано изображение
    //
    if (d->image.display.isNull()) {
        const auto decorationColor = d->decorationColorAnimation.currentValue().isValid()
            ? d->decorationColorAnimation.currentValue().value<QColor>()
            : d->decorationColorAnimation.startValue().value<QColor>();
        painter.setPen(decorationColor);
        painter.setBrush(Qt::NoBrush);
        auto iconFont = Ui::DesignSystem::font().iconsForEditors();
        iconFont.setPixelSize(Ui::DesignSystem::scaleFactor() * Ui::DesignSystem::layout().px48()
                              * 2);
        const auto iconHeight = TextHelper::fineLineSpacing(iconFont);
        const auto textHeight = TextHelper::fineLineSpacing(Ui::DesignSystem::font().button());
        const auto decorationHeight = iconHeight + textHeight;
        const auto iconRect = QRectF(0.0, (height() - decorationHeight) / 2.0, width(), iconHeight);
        painter.setFont(iconFont);
        painter.drawText(iconRect, Qt::AlignCenter, d->decorationIcon);
        //
        const auto textRect = QRectF(0.0, iconRect.bottom(), width(), textHeight);
        painter.setFont(Ui::DesignSystem::font().button());
        painter.drawText(textRect, Qt::AlignHCenter, d->emptyImageText);
    }
    //
    // Если задано изображение, то рисуем его
    //
    else {
        //
        // ... собственно изображение
        //
        const auto imageRect
            = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins());
        const auto borderRadius = Ui::DesignSystem::card().borderRadius();
        ImageHelper::drawRoundedImage(painter, imageRect, d->image.display, borderRadius);

        if (d->overlayOpacityAnimation.currentValue().toReal() > 0) {
            painter.setOpacity(d->overlayOpacityAnimation.currentValue().toReal());

            //
            // ... затемнение сверху изображения
            //
            painter.setPen(Qt::NoPen);
            painter.setBrush(Ui::DesignSystem::color().shadow());
            painter.drawRoundedRect(imageRect, borderRadius, borderRadius);

            //
            // ... вспомогательный текст
            //
            painter.setPen(Ui::DesignSystem::color().onShadow());
            painter.setFont(Ui::DesignSystem::font().button());
            painter.drawText(imageRect, Qt::AlignCenter, d->imageText);

            //
            // ... кнопка очистки
            //
            if (!d->isReadOnly) {
                painter.setFont(Ui::DesignSystem::font().iconsMid());
                painter.drawText(d->clearButtonRect(), Qt::AlignCenter, u8"\U000F0156");
            }
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
        const auto cardRect
            = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins());
        const auto borderRadius = Ui::DesignSystem::card().borderRadius();
        painter.drawRoundedRect(cardRect, borderRadius, borderRadius);
        //
        painter.setPen(Ui::DesignSystem::color().onAccent());
        painter.setBrush(Qt::NoBrush);
        auto iconFont = Ui::DesignSystem::font().iconsForEditors();
        iconFont.setPixelSize(Ui::DesignSystem::layout().px(82));
        painter.setFont(iconFont);
        painter.drawText(cardRect, Qt::AlignCenter, u8"\U000F01DA");
        //
        painter.setOpacity(1.0);
    }
}

void ImageCard::resizeEvent(QResizeEvent* _event)
{
    Card::resizeEvent(_event);

    d->prepareImageForDisplaing(_event->size());
}

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
void ImageCard::enterEvent(QEnterEvent* _event)
#else
void ImageCard::enterEvent(QEvent* _event)
#endif
{
    Card::enterEvent(_event);

    d->decorationColorAnimation.setDirection(QVariantAnimation::Forward);
    d->decorationColorAnimation.start();
    d->overlayOpacityAnimation.setDirection(QVariantAnimation::Forward);
    d->overlayOpacityAnimation.start();
}

void ImageCard::leaveEvent(QEvent* _event)
{
    Card::leaveEvent(_event);

    d->decorationColorAnimation.setDirection(QVariantAnimation::Backward);
    d->decorationColorAnimation.start();
    d->overlayOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->overlayOpacityAnimation.start();
}

void ImageCard::mousePressEvent(QMouseEvent* _event)
{
    Card::mousePressEvent(_event);

    if (!d->isReadOnly && _event->button() == Qt::LeftButton) {
        if (d->isInsideClearButton(_event->pos())) {
            d->clearImageAction->trigger();
        } else {
            d->chooseImageFromFile();
        }
    }
}

void ImageCard::dragEnterEvent(QDragEnterEvent* _event)
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

void ImageCard::dragMoveEvent(QDragMoveEvent* _event)
{
    if (d->isReadOnly) {
        _event->ignore();
        return;
    }

    _event->acceptProposedAction();
}

void ImageCard::dragLeaveEvent(QDragLeaveEvent* _event)
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

void ImageCard::dropEvent(QDropEvent* _event)
{
    if (d->isReadOnly) {
        _event->ignore();
        return;
    }

    QPixmap droppedImage;
    const QMimeData* mimeData = _event->mimeData();
    //
    // Первым делом проверяем список ссылок, возможно выбраны сразу несколько фотогафий
    //
    if (mimeData->hasUrls()) {
        for (const auto& url : mimeData->urls()) {
            const QString urlString = url.toString().toLower();
            //
            // ... локальные изображения
            //
            if ((urlString.contains(".png") || urlString.contains(".jpg")
                 || urlString.contains(".jpeg") || urlString.contains(".gif")
                 || urlString.contains(".tiff") || urlString.contains(".bmp")
                 || urlString.contains(".webp"))
                && url.isLocalFile()) {
                droppedImage = QPixmap(url.toLocalFile());
            }
            //
            // ... подгружаем картинки с инета
            //
            else {
                //
                // TODO: сделать асинхронно
                //
                const QByteArray pixmapData = NetworkRequestLoader::loadSync(url);
                droppedImage.loadFromData(pixmapData);
            }
        }
    } else if (mimeData->hasImage()) {
        droppedImage = qvariant_cast<QPixmap>(mimeData->imageData());
    }

    if (!droppedImage.isNull()) {
        d->cropImage(droppedImage);
    }

    _event->acceptProposedAction();

    d->isDragActive = false;
    d->dragIndicationOpacityAnimation.setDirection(QVariantAnimation::Backward);
    d->dragIndicationOpacityAnimation.start();
}

void ImageCard::processTextColorChange()
{
    d->decorationColorAnimation.setStartValue(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::disabledTextOpacity()));
}

void ImageCard::updateTranslations()
{
    d->changeImageAction->setText(tr("Select file"));
    d->copyImageAction->setText(tr("Copy"));
    d->pasteImageAction->setText(tr("Paste"));
    d->clearImageAction->setText(tr("Delete"));
}

void ImageCard::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    d->decorationColorAnimation.setStartValue(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::disabledTextOpacity()));
    d->decorationColorAnimation.setEndValue(Ui::DesignSystem::color().accent());

    d->contextMenu->setBackgroundColor(Ui::DesignSystem::color().background());
    d->contextMenu->setTextColor(Ui::DesignSystem::color().onBackground());
}

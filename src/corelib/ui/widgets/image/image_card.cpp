#include "image_card.h"

#include "image_cropping_dialog.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>

#include <QApplication>
#include <QFileDialog>
#include <QPainter>
#include <QResizeEvent>
#include <QVariantAnimation>


class ImageCard::Implementation
{
public:
    Implementation();

    void prepareImageForDisplaing(const QSize& _size);

    void changeImage(ImageCard* _imageCard);


    QString decorationIcon = u8"\U000F0513";
    QString decorationText;
    QString imageCroppingText;
    QVariantAnimation decorationColorAnimation;

    struct {
        QPixmap source;
        QPixmap display;
    } image;
};

ImageCard::Implementation::Implementation()
{
    decorationColorAnimation.setDuration(240);
    decorationColorAnimation.setEasingCurve(QEasingCurve::OutQuad);
}

void ImageCard::Implementation::prepareImageForDisplaing(const QSize& _size)
{
    if (image.source.isNull()) {
        return;
    }

    //
    // Добавляем небольшую дельту, чтобы постер занимал всё доступное пространство
    //
    const auto sizeCorrected = QRect({}, _size)
                               .marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins()).size();
    if (image.display.size() == sizeCorrected) {
        return;
    }

    image.display = image.source.scaled(sizeCorrected, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
}

void ImageCard::Implementation::changeImage(ImageCard* _imageCard)
{
    const QString coverPath = QFileDialog::getOpenFileName(_imageCard->window(), tr("Choose cover"), {},
                                    QString("%1 (*.png *.jpeg *.jpg *.bmp *.tiff *.tif *.gif)").arg(tr("Images")));
    if (coverPath.isEmpty()) {
        return;
    }

    QPixmap cover(coverPath);
    if (cover.isNull()) {
        return;
    }

    auto dlg = new ImageCroppingDialog(_imageCard->window());
    dlg->setImage(cover);
    dlg->setImageProportion(_imageCard->size());
    dlg->setImageProportionFixed(true);
    dlg->setImageCroppingText(imageCroppingText);
    connect(dlg, &ImageCroppingDialog::disappeared, dlg, &ImageCroppingDialog::deleteLater);
    connect(dlg, &ImageCroppingDialog::imageSelected, _imageCard, &ImageCard::setImage);

    dlg->showDialog();
}


// ****


ImageCard::ImageCard(QWidget* _parent)
    : Card(_parent),
      d(new Implementation)
{
    setAttribute(Qt::WA_Hover);

    connect(&d->decorationColorAnimation, &QVariantAnimation::valueChanged, this, qOverload<>(&ImageCard::update));

    updateTranslations();
    designSystemChangeEvent(nullptr);
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

void ImageCard::setDecorationText(const QString& _text)
{
    if (d->decorationText == _text) {
        return;
    }

    d->decorationText = _text;
    if (!d->image.source.isNull()) {
        return;
    }

    update();
}

void ImageCard::setImageCroppingText(const QString& _text)
{
    if (d->imageCroppingText == _text) {
        return;
    }

    d->imageCroppingText = _text;
}

void ImageCard::setImage(const QPixmap& _image)
{
    if ((d->image.source.isNull() && _image.isNull())
        || (!d->image.source.isNull() && !_image.isNull() && d->image.source == _image)) {
        return;
    }

    d->image.source = _image;
    d->image.display = {};
    d->prepareImageForDisplaing(size());
    update();

    emit imageChanged(d->image.source);
}

void ImageCard::paintEvent(QPaintEvent* _event)
{
    Card::paintEvent(_event);

    QPainter painter(this);

    //
    // Если задано изображение, то рисуем его
    //
    if (!d->image.display.isNull()) {
        const auto imageRect = rect().marginsRemoved(Ui::DesignSystem::card().shadowMargins().toMargins());
        const auto borderRadius = Ui::DesignSystem::card().borderRadius();
        ImageHelper::drawRoundedImage(painter, imageRect, d->image.display, borderRadius);
        return;
    }

    //
    // Если изображение не задано, то рисуем декорацию
    //
    const auto decorationColor = d->decorationColorAnimation.currentValue().isValid()
                                 ? d->decorationColorAnimation.currentValue().value<QColor>()
                                 : d->decorationColorAnimation.startValue().value<QColor>();
    painter.setPen(decorationColor);
    painter.setBrush(Qt::NoBrush);
    auto iconFont = Ui::DesignSystem::font().iconsBig();
    iconFont.setPixelSize(Ui::DesignSystem::scaleFactor() * Ui::DesignSystem::layout().px48() * 2);
    const auto iconHeight = QFontMetricsF(iconFont).boundingRect(d->decorationIcon).height();
    const auto textHeight = QFontMetricsF(Ui::DesignSystem::font().button()).boundingRect(d->decorationText).height();
    const auto decorationHeight = iconHeight + textHeight;
    const auto iconRect = QRectF(0.0, (height() - decorationHeight) / 2.0,
                                 width(), iconHeight);
    painter.setFont(iconFont);
    painter.drawText(iconRect, Qt::AlignCenter, d->decorationIcon);
    //
    const auto textRect = QRectF(0.0, iconRect.bottom(),
                                 width(), textHeight);
    painter.setFont(Ui::DesignSystem::font().button());
    painter.drawText(textRect, Qt::AlignHCenter, d->decorationText);
}

void ImageCard::resizeEvent(QResizeEvent* _event)
{
    Card::resizeEvent(_event);

    d->prepareImageForDisplaing(_event->size());
}

void ImageCard::enterEvent(QEvent* _event)
{
    Card::enterEvent(_event);

    d->decorationColorAnimation.setDirection(QVariantAnimation::Forward);
    d->decorationColorAnimation.start();
}

void ImageCard::leaveEvent(QEvent* _event)
{
    Card::leaveEvent(_event);

    d->decorationColorAnimation.setDirection(QVariantAnimation::Backward);
    d->decorationColorAnimation.start();
}

void ImageCard::mousePressEvent(QMouseEvent* _event)
{
    Card::mousePressEvent(_event);

    d->changeImage(this);
}

void ImageCard::updateTranslations()
{

}

void ImageCard::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    d->decorationColorAnimation.setStartValue(
                ColorHelper::transparent(
                    Ui::DesignSystem::color().onBackground(),
                    Ui::DesignSystem::disabledTextOpacity()));
    d->decorationColorAnimation.setEndValue(Ui::DesignSystem::color().secondary());
}

#include "label.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/icon_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QFontMetrics>
#include <QPaintEvent>
#include <QPainter>


class AbstractLabel::Implementation
{
public:
    QString text;
    QString skeleton;
    Qt::Alignment alignment = Qt::AlignTop | Qt::AlignLeft;
    bool clickable = false;
};


// ****


AbstractLabel::AbstractLabel(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
}

QString AbstractLabel::text() const
{
    return d->text;
}

AbstractLabel::~AbstractLabel() = default;

void AbstractLabel::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    updateGeometry();
    update();
}

void AbstractLabel::setSkeleton(const QString& _filler)
{
    if (d->skeleton == _filler) {
        return;
    }

    d->skeleton = _filler;
    updateGeometry();
    update();
}

void AbstractLabel::setAlignment(Qt::Alignment _alignment)
{
    if (d->alignment == _alignment) {
        return;
    }

    d->alignment = _alignment;
    update();
}

void AbstractLabel::setClickable(bool _clickable)
{
    if (d->clickable == _clickable) {
        return;
    }

    setAttribute(Qt::WA_Hover, _clickable);
    d->clickable = _clickable;
    update();
}

QSize AbstractLabel::sizeHint() const
{
    int width = 0;
    const auto lines = (d->text.isEmpty() ? d->skeleton : d->text).split('\n');
    for (const auto& line : lines) {
        width = std::max(width, TextHelper::fineTextWidth(line, textFont()));
    }
    const int height = TextHelper::fineLineSpacing(textFont()) * lines.size();
    return QRect(QPoint(0, 0), QSize(width, height)).marginsAdded(contentsMargins()).size();
}

int AbstractLabel::heightForWidth(int _width) const
{
    const int textWidth = _width - contentsMargins().left() - contentsMargins().right();
    int textHeight = 0;
    const auto lines = (d->text.isEmpty() ? d->skeleton : d->text).split('\n');
    for (const auto& line : lines) {
        textHeight += static_cast<int>(TextHelper::heightForWidth(line, textFont(), textWidth));
    }
    return contentsMargins().top() + textHeight + contentsMargins().bottom();
}

void AbstractLabel::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);

    //
    // Рисуем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Если не заданы ни текст, ни скелетон, больше тут делать нечего
    //
    if (d->text.isEmpty() && d->skeleton.isEmpty()) {
        return;
    }

    //
    // Если текста нет, рисуем скелетон
    //
    if (d->text.isEmpty()) {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.setPen(Qt::NoPen);
        const int textWidth = contentsRect().width();
        const auto lines = (d->text.isEmpty() ? d->skeleton : d->text).split('\n');
        int lastY = contentsMargins().top();
        for (const auto& line : lines) {
            const QRectF textRect(
                contentsMargins().left(), lastY,
                std::min(TextHelper::fineTextWidth(line, textFont()), textWidth),
                static_cast<int>(TextHelper::heightForWidth(line, textFont(), textWidth)));
            if (!line.isEmpty()) {
                QLinearGradient backgroundGradient(0, 0, textRect.width(), 0);
                backgroundGradient.setColorAt(
                    0,
                    ColorHelper::transparent(textColor(), Ui::DesignSystem::elevationEndOpacity()));
                backgroundGradient.setColorAt(
                    0.3,
                    ColorHelper::transparent(textColor(),
                                             Ui::DesignSystem::elevationStartOpacity()));
                backgroundGradient.setColorAt(
                    0.6,
                    ColorHelper::transparent(textColor(),
                                             Ui::DesignSystem::elevationStartOpacity()));
                backgroundGradient.setColorAt(
                    1,
                    ColorHelper::transparent(textColor(),
                                             Ui::DesignSystem::hoverBackgroundOpacity()));
                painter.setBrush(backgroundGradient);

                painter.drawRoundedRect(textRect, Ui::DesignSystem::layout().px(6),
                                        Ui::DesignSystem::layout().px(6));
            }

            lastY = textRect.bottom();
        }
        return;
    }

    //
    // Рисуем текст
    //
    painter.setRenderHint(QPainter::TextAntialiasing);
    auto font = textFont();
    if (d->clickable && underMouse()) {
        font.setUnderline(true);
    }
    painter.setFont(font);
    painter.setPen(textColor());
    painter.setOpacity(isEnabled() ? 1.0 : Ui::DesignSystem::disabledTextOpacity());

    painter.drawText(
        contentsRect(),
        d->alignment
            | (d->text.isRightToLeft() ? Qt::TextForceRightToLeft : Qt::TextForceLeftToRight)
            | Qt::TextWordWrap,
        d->text);
}

void AbstractLabel::mouseReleaseEvent(QMouseEvent* _event)
{
    Widget::mouseReleaseEvent(_event);

    if (!isEnabled() || !rect().contains(_event->pos())) {
        return;
    }

    emit clicked();
}


// ****


H4Label::H4Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& H4Label::textFont() const
{
    return Ui::DesignSystem::font().h4();
}


// ****


H5Label::H5Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& H5Label::textFont() const
{
    return Ui::DesignSystem::font().h5();
}


// ****


H6Label::H6Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& H6Label::textFont() const
{
    return Ui::DesignSystem::font().h6();
}


// ****


Subtitle1Label::Subtitle1Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Subtitle1Label::textFont() const
{
    return Ui::DesignSystem::font().subtitle1();
}


// ****


Subtitle2Label::Subtitle2Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Subtitle2Label::textFont() const
{
    return Ui::DesignSystem::font().subtitle2();
}


// ****


Body1Label::Body1Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Body1Label::textFont() const
{
    return Ui::DesignSystem::font().body1();
}


// ****


Body2Label::Body2Label(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& Body2Label::textFont() const
{
    return Ui::DesignSystem::font().body2();
}


// ****


ButtonLabel::ButtonLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& ButtonLabel::textFont() const
{
    return Ui::DesignSystem::font().button();
}


// ****


CaptionLabel::CaptionLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& CaptionLabel::textFont() const
{
    return Ui::DesignSystem::font().caption();
}


// ****


OverlineLabel::OverlineLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

const QFont& OverlineLabel::textFont() const
{
    return Ui::DesignSystem::font().overline();
}


// ****


AbstractIconsLabel::AbstractIconsLabel(QWidget* _parent)
    : AbstractLabel(_parent)
{
}

void AbstractIconsLabel::setIcon(const QString& _icon)
{
    if (m_icon == _icon) {
        return;
    }

    m_icon = _icon;
    setText(IconHelper::directedIcon(m_icon));
}

QSize AbstractIconsLabel::sizeHint() const
{
    const auto size = std::max(TextHelper::fineTextWidth(m_icon, textFont()),
                               static_cast<int>(TextHelper::fineLineSpacing(textFont())));
    return QRect(QPoint(0, 0), QSize(size, size)).marginsAdded(contentsMargins()).size();
}

bool AbstractIconsLabel::event(QEvent* _event)
{
    if (_event->type() == QEvent::LayoutDirectionChange) {
        setText(IconHelper::directedIcon(m_icon));
    }

    return AbstractLabel::event(_event);
}

void AbstractIconsLabel::setText(const QString& _text)
{
    AbstractLabel::setText(_text);
}


// ****


IconsSmallLabel::IconsSmallLabel(QWidget* _parent)
    : AbstractIconsLabel(_parent)
{
}

const QFont& IconsSmallLabel::textFont() const
{
    return Ui::DesignSystem::font().iconsSmall();
}


// ****


IconsMidLabel::IconsMidLabel(QWidget* _parent)
    : AbstractIconsLabel(_parent)
{
}

void IconsMidLabel::setDecorationVisible(bool _visible)
{
    if (m_isDecorationVisible == _visible) {
        return;
    }

    m_isDecorationVisible = _visible;
    updateGeometry();
    update();
}

QSize IconsMidLabel::sizeHint() const
{
    if (m_isDecorationVisible) {
        return QSize(Ui::DesignSystem::layout().px48(), Ui::DesignSystem::layout().px48());
    }

    return AbstractIconsLabel::sizeHint();
}

int IconsMidLabel::heightForWidth(int _width) const
{
    if (m_isDecorationVisible) {
        return sizeHint().height();
    }

    return AbstractIconsLabel::heightForWidth(_width);
}

const QFont& IconsMidLabel::textFont() const
{
    return Ui::DesignSystem::font().iconsMid();
}

void IconsMidLabel::paintEvent(QPaintEvent* _event)
{
    if (!m_isDecorationVisible) {
        AbstractIconsLabel::paintEvent(_event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Рисуем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем декорацию
    //
    painter.setPen(Qt::NoPen);
    painter.setBrush(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::focusBackgroundOpacity()));
    painter.drawEllipse(contentsRect());

    //
    // Рисуем текст
    //
    painter.setFont(textFont());
    painter.setPen(textColor());
    painter.setOpacity(isEnabled() ? 1.0 : Ui::DesignSystem::disabledTextOpacity());
    painter.drawText(contentsRect(), Qt::AlignCenter, text());
}


// ****


IconsBigLabel::IconsBigLabel(QWidget* _parent)
    : AbstractIconsLabel(_parent)
{
}

void IconsBigLabel::setDecorationVisible(bool _visible)
{
    if (m_isDecorationVisible == _visible) {
        return;
    }

    m_isDecorationVisible = _visible;
    updateGeometry();
    update();
}

QSize IconsBigLabel::sizeHint() const
{
    if (m_isDecorationVisible) {
        return QSize(Ui::DesignSystem::layout().px62(), Ui::DesignSystem::layout().px62());
    }

    return AbstractIconsLabel::sizeHint();
}

int IconsBigLabel::heightForWidth(int _width) const
{
    if (m_isDecorationVisible) {
        return sizeHint().height();
    }

    return AbstractIconsLabel::heightForWidth(_width);
}

const QFont& IconsBigLabel::textFont() const
{
    return Ui::DesignSystem::font().iconsBig();
}

void IconsBigLabel::paintEvent(QPaintEvent* _event)
{
    if (!m_isDecorationVisible) {
        AbstractIconsLabel::paintEvent(_event);
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Рисуем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем декорацию
    //
    painter.setPen(Qt::NoPen);
    painter.setBrush(
        ColorHelper::transparent(textColor(), Ui::DesignSystem::focusBackgroundOpacity()));
    painter.drawEllipse(contentsRect());

    //
    // Рисуем текст
    //
    painter.setFont(textFont());
    painter.setPen(textColor());
    painter.setOpacity(isEnabled() ? 1.0 : Ui::DesignSystem::disabledTextOpacity());
    painter.drawText(contentsRect(), Qt::AlignCenter, text());
}


// ****


class ImageLabel::Implementation
{
public:
    explicit Implementation(ImageLabel* _q);

    void updateDisplayImage();


    ImageLabel* q = nullptr;

    qreal borderRadius = 0.0;
    bool isSkeletonEnabled = false;
    QPixmap sourceImage;
    QPixmap displayImage;
};

ImageLabel::Implementation::Implementation(ImageLabel* _q)
    : q(_q)
{
}

void ImageLabel::Implementation::updateDisplayImage()
{
    if (!sourceImage.isNull()) {
        if (qFuzzyCompare(borderRadius, 0.0)) {
            displayImage = sourceImage.scaled(q->contentsRect().size(), Qt::IgnoreAspectRatio,
                                              Qt::SmoothTransformation);
        } else {
            const auto roundedImage = sourceImage.scaled(
                q->contentsRect().size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

            displayImage = QPixmap(q->contentsRect().size());
            displayImage.fill(Qt::transparent);
            QPainter painter(&displayImage);
            painter.setRenderHint(QPainter::Antialiasing);
            ImageHelper::drawRoundedImage(painter, roundedImage.rect(), roundedImage, borderRadius);
        }
    } else {
        displayImage = {};
    }
}

// **

ImageLabel::ImageLabel(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
}

ImageLabel::~ImageLabel() = default;

void ImageLabel::setBorderRadius(qreal _radius)
{
    if (qFuzzyCompare(d->borderRadius, _radius)) {
        return;
    }

    d->borderRadius = _radius;
    d->updateDisplayImage();
    update();
}

void ImageLabel::setSkeleton(bool _enabled)
{
    if (d->isSkeletonEnabled == _enabled) {
        return;
    }

    d->isSkeletonEnabled = _enabled;
    update();
}

void ImageLabel::setImage(const QPixmap& _image)
{
    d->sourceImage = _image;
    d->updateDisplayImage();
    update();
}

void ImageLabel::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    QBrush backgroundBrush = backgroundColor();
    if (d->isSkeletonEnabled) {
        QLinearGradient backgroundGradient(0, 0, width(), 0);
        backgroundGradient.setColorAt(
            0,
            ColorHelper::transparent(backgroundColor(), Ui::DesignSystem::elevationEndOpacity()));
        backgroundGradient.setColorAt(
            0.2,
            ColorHelper::transparent(backgroundColor(), Ui::DesignSystem::elevationStartOpacity()));
        backgroundGradient.setColorAt(
            0.6,
            ColorHelper::transparent(backgroundColor(), Ui::DesignSystem::elevationStartOpacity()));
        backgroundGradient.setColorAt(
            1,
            ColorHelper::transparent(backgroundColor(), Ui::DesignSystem::elevationEndOpacity()));
        backgroundBrush = backgroundGradient;
    }

    if (qFuzzyCompare(d->borderRadius, 0.0)) {
        painter.fillRect(_event->rect(), backgroundBrush);
    } else {
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(_event->rect(), Qt::transparent);
        painter.setPen(Qt::NoPen);
        painter.setBrush(backgroundBrush);
        painter.drawRoundedRect(contentsRect(), d->borderRadius, d->borderRadius);
    }
    if (!d->displayImage.isNull()) {
        painter.drawPixmap(contentsRect(), d->displayImage);
    }
}

void ImageLabel::resizeEvent(QResizeEvent* _event)
{
    d->updateDisplayImage();

    Widget::resizeEvent(_event);
}

#include "button.h"

#include <ui/design_system/design_system.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QKeyEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QVariantAnimation>


class Button::Implementation
{
public:
    Implementation();

    /**
     * @brief Анимировать клик
     */
    void animateClick();

    /**
     * @brief Анимировать тень
     */
    void animateHoverIn();
    void animateHoverOut();


    QString icon;
    QString text;
    bool isContained = false;
    bool isOutlined = false;
    bool isFlat = false;

    /**
     * @brief  Декорации кнопки при клике
     */
    QPointF decorationCenterPosition;
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;

    /**
     * @brief  Декорации тени при наведении
     */
    QVariantAnimation shadowBlurRadiusAnimation;
};

Button::Implementation::Implementation()
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationRadiusAnimation.setStartValue(1.0);
    decorationRadiusAnimation.setDuration(240);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(420);

    shadowBlurRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    shadowBlurRadiusAnimation.setDuration(160);
}

void Button::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}

void Button::Implementation::animateHoverIn()
{
    shadowBlurRadiusAnimation.setDirection(QVariantAnimation::Forward);
    shadowBlurRadiusAnimation.start();
}

void Button::Implementation::animateHoverOut()
{
    shadowBlurRadiusAnimation.setDirection(QVariantAnimation::Backward);
    shadowBlurRadiusAnimation.start();
}


// ****


Button::Button(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&Button::update));
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&Button::update));
    connect(&d->shadowBlurRadiusAnimation, &QVariantAnimation::valueChanged, this,
            qOverload<>(&Button::update));

    designSystemChangeEvent(nullptr);
}

Button::~Button() = default;

void Button::setIcon(const QString& _icon)
{
    if (d->icon == _icon) {
        return;
    }

    d->icon = _icon;
    updateGeometry();
    update();
}

void Button::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    updateGeometry();
    update();
}

void Button::setContained(bool _contained)
{
    if (d->isContained == _contained) {
        return;
    }

    d->isContained = _contained;
    designSystemChangeEvent(nullptr);
}

void Button::setOutlined(bool _outlined)
{
    if (d->isOutlined == _outlined) {
        return;
    }

    d->isOutlined = _outlined;
    designSystemChangeEvent(nullptr);
}

void Button::setFlat(bool _flat)
{
    if (d->isFlat == _flat) {
        return;
    }

    d->isFlat = _flat;
    designSystemChangeEvent(nullptr);
}

void Button::click()
{
    emit clicked();
}

QSize Button::sizeHint() const
{
    const auto shadowMarginsWidth = d->isContained
        ? Ui::DesignSystem::button().shadowMargins().left()
            + Ui::DesignSystem::button().shadowMargins().right()
        : 0.0;
    const qreal width
        = std::max(
              Ui::DesignSystem::button().minimumWidth(),
              Ui::DesignSystem::button().margins().left()
                  + (!d->icon.isEmpty() ? Ui::DesignSystem::button().iconSize().width() : 0)
                  + (!d->icon.isEmpty() && !d->text.isEmpty() ? Ui::DesignSystem::button().spacing()
                                                              : 0)
                  + (!d->text.isEmpty()
                         ? TextHelper::fineTextWidth(d->text, Ui::DesignSystem::font().button())
                         : 0)
                  + Ui::DesignSystem::button().margins().right())
        + shadowMarginsWidth;
    const auto shadowMarginsHeight = d->isContained
        ? Ui::DesignSystem::button().shadowMargins().top()
            + Ui::DesignSystem::button().shadowMargins().bottom()
        : 0.0;
    const qreal height = Ui::DesignSystem::button().height() + shadowMarginsHeight;
    return QSize(static_cast<int>(width) + contentsMargins().left() + contentsMargins().right(),
                 static_cast<int>(height) + contentsMargins().top() + contentsMargins().bottom());
}

void Button::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    const QRect backgroundRect = contentsRect().marginsRemoved(
        d->isContained ? Ui::DesignSystem::button().shadowMargins().toMargins() : QMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setOpacity(opacity());

    //
    // Заливаем фон
    //
    QColor backgroundColor;
    if (isEnabled() && underMouse()) {
        if (d->isContained || d->isFlat) {
            backgroundColor = ColorHelper::transparent(
                this->backgroundColor(), 1.0 - Ui::DesignSystem::hoverBackgroundOpacity());
        } else {
            backgroundColor = ColorHelper::transparent(this->backgroundColor(),
                                                       Ui::DesignSystem::hoverBackgroundOpacity());
        }
    } else if (hasFocus()) {
        if (d->isContained || d->isFlat) {
            backgroundColor = ColorHelper::transparent(
                this->backgroundColor(), 1.0 - Ui::DesignSystem::focusBackgroundOpacity());
        } else {
            backgroundColor = ColorHelper::transparent(this->backgroundColor(),
                                                       Ui::DesignSystem::focusBackgroundOpacity());
        }
    } else {
        if (d->isContained || d->isFlat) {
            backgroundColor = this->backgroundColor();
        }
    }
    if (backgroundColor.isValid()) {
        //
        // Тень рисуем только в случае, если кнопка имеет установленный фон
        //
        if (d->isContained) {
            QPixmap backgroundImage(backgroundRect.size());
            backgroundImage.fill(Qt::transparent);
            QPainter backgroundImagePainter(&backgroundImage);
            backgroundImagePainter.setPen(Qt::NoPen);
            backgroundImagePainter.setBrush(backgroundColor);
            backgroundImagePainter.drawRoundedRect(QRect({ 0, 0 }, backgroundImage.size()),
                                                   Ui::DesignSystem::button().borderRadius(),
                                                   Ui::DesignSystem::button().borderRadius());

            const qreal shadowBlurRadius
                = std::max(Ui::DesignSystem::button().minimumShadowBlurRadius(),
                           d->shadowBlurRadiusAnimation.currentValue().toReal());
            const QPixmap shadow = ImageHelper::dropShadow(
                backgroundImage, Ui::DesignSystem::button().shadowMargins(), shadowBlurRadius,
                Ui::DesignSystem::color().shadow());
            painter.drawPixmap(contentsRect().topLeft(), shadow);
        }
        //
        // ... собственно отрисовка фона
        //
        painter.setPen(Qt::NoPen);
        painter.setBrush(backgroundColor);
        painter.drawRoundedRect(backgroundRect, Ui::DesignSystem::button().borderRadius(),
                                Ui::DesignSystem::button().borderRadius());
    }

    //
    // Рисуем рамку
    //
    if (d->isOutlined) {
        painter.setPen(QPen(textColor(), 1.0 * Ui::DesignSystem::scaleFactor()));
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(backgroundRect, Ui::DesignSystem::button().borderRadius(),
                                Ui::DesignSystem::button().borderRadius());
    }

    //
    // Декорация
    //
    if (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
        || d->decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        painter.setClipRect(backgroundRect);
        painter.setPen(Qt::NoPen);
        painter.setBrush(textColor());
        painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
        painter.drawEllipse(d->decorationCenterPosition,
                            d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(opacity());
        painter.setClipRect(QRect(), Qt::NoClip);
    }

    //
    // Рисуем текст
    //
    painter.setPen(isEnabled() ? textColor()
                               : ColorHelper::transparent(textColor(),
                                                          Ui::DesignSystem::disabledTextOpacity()));
    //
    QRectF buttonInnerRect
        = contentsRect().marginsRemoved(Ui::DesignSystem::button().margins().toMargins());
    const qreal textWidth = TextHelper::fineTextWidthF(d->text, Ui::DesignSystem::font().button());
    //
    // ... если иконка задана, рисуем иконку и корректируем область отрисовки текста
    //
    if (!d->icon.isEmpty()) {
        const QSizeF iconSize = Ui::DesignSystem::button().iconSize();
        const qreal textWithIconWidth = iconSize.width()
            + (textWidth > 0 ? Ui::DesignSystem::button().spacing() + textWidth : 0);

        qreal iconX = buttonInnerRect.x();
        if (isLeftToRight()) {
            iconX += d->isContained ? ((buttonInnerRect.width() - textWithIconWidth) / 2.0) : 0.0;
        } else {
            iconX += d->isContained ? ((buttonInnerRect.width() - textWithIconWidth) / 2.0
                                       + textWidth + Ui::DesignSystem::button().spacing())
                                    : buttonInnerRect.width() - iconSize.width();
        }
        const QRectF iconRect(QPointF(iconX, buttonInnerRect.top()),
                              QSizeF(iconSize.width(), buttonInnerRect.height()));
        painter.setFont(Ui::DesignSystem::font().iconsMid());
        painter.drawText(iconRect, Qt::AlignCenter, d->icon);

        if (isLeftToRight()) {
            buttonInnerRect.setX(iconRect.right() + Ui::DesignSystem::button().spacing());
        } else {
            buttonInnerRect.setX(iconRect.left() - Ui::DesignSystem::button().spacing()
                                 - textWidth);
        }
    }
    //
    // ... а если иконки нет, то просто корректируем область в которой будет рисоваться текст
    //
    else {
        qreal textX = buttonInnerRect.x();

        if (d->isContained) {
            textX += (buttonInnerRect.width() - textWidth) / 2.0;
        } else {
            textX += isLeftToRight() ? 0.0 : (buttonInnerRect.width() - textWidth);
        }

        buttonInnerRect.setX(textX);
    }
    //
    // ... если ширина текста меньше ширины кнопки, то корректируем обасть для отображения текста,
    //     т.к. текст рисуется в её центре
    //
    auto textAlignment = Qt::AlignLeft | Qt::AlignVCenter;
    if (buttonInnerRect.width() >= textWidth) {
        buttonInnerRect.setWidth(textWidth);
        textAlignment = Qt::AlignCenter;
    }
    //
    if (!d->text.isEmpty()) {
        painter.setFont(Ui::DesignSystem::font().button());
        painter.drawText(
            buttonInnerRect, textAlignment,
            painter.fontMetrics().elidedText(d->text, Qt::ElideRight, buttonInnerRect.width()));
    }
}

#if (QT_VERSION > QT_VERSION_CHECK(6, 0, 0))
void Button::enterEvent(QEnterEvent* _event)
#else
void Button::enterEvent(QEvent* _event)
#endif
{
    Q_UNUSED(_event)
    d->animateHoverIn();
}

void Button::leaveEvent(QEvent* _event)
{
    Q_UNUSED(_event)
    d->animateHoverOut();
}

void Button::mousePressEvent(QMouseEvent* _event)
{
    d->decorationCenterPosition = _event->pos();
    d->decorationRadiusAnimation.setEndValue(static_cast<qreal>(width()));
    d->animateClick();
}

void Button::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event)

    if (!rect().contains(_event->pos())) {
        return;
    }

    emit clicked();
}

void Button::keyPressEvent(QKeyEvent* _event)
{
    if (_event->key() == Qt::Key_Space || _event->key() == Qt::Key_Enter
        || _event->key() == Qt::Key_Return) {
        _event->accept();
        emit clicked();
        return;
    }

    _event->ignore();
}

void Button::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->shadowBlurRadiusAnimation.setStartValue(
        Ui::DesignSystem::button().minimumShadowBlurRadius());
    d->shadowBlurRadiusAnimation.setEndValue(Ui::DesignSystem::button().maximumShadowBlurRadius());

    updateGeometry();
    update();
}

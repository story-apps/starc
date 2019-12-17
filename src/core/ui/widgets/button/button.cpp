#include "button.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPainter>
#include <QPaintEvent>
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
    : Widget(_parent),
      d(new Implementation)
{
    setAttribute(Qt::WA_Hover);
    setFocusPolicy(Qt::StrongFocus);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->shadowBlurRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });

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
    update();
}

void Button::click()
{
    emit clicked();
}

QSize Button::sizeHint() const
{
    const qreal width = Ui::DesignSystem::button().shadowMargins().left()
                        + std::max(Ui::DesignSystem::button().minimumWidth(),
                                   Ui::DesignSystem::button().margins().left()
                                   + TextHelper::fineTextWidth(d->text, Ui::DesignSystem::font().button())
                                   + Ui::DesignSystem::button().margins().right())
                        + Ui::DesignSystem::button().shadowMargins().right();
    const qreal height = Ui::DesignSystem::button().shadowMargins().top()
                         + Ui::DesignSystem::button().height()
                         + Ui::DesignSystem::button().shadowMargins().bottom();
    return QSize(static_cast<int>(width), static_cast<int>(height));
}

void Button::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event)

    const QRect backgroundRect = rect().marginsRemoved(Ui::DesignSystem::button().shadowMargins().toMargins());
    if (!backgroundRect.isValid()) {
        return;
    }

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    QColor backgroundColor;
    if (d->isContained) {
        backgroundColor = this->backgroundColor();
    } else if (underMouse() || hasFocus()) {
        backgroundColor = this->backgroundColor();
        backgroundColor.setAlphaF(0.2);
    }
    if (backgroundColor.isValid()) {
        QPixmap backgroundImage(backgroundRect.size());
        backgroundImage.fill(Qt::transparent);
        QPainter backgroundImagePainter(&backgroundImage);
        backgroundImagePainter.setPen(Qt::NoPen);
        backgroundImagePainter.setBrush(backgroundColor);
        backgroundImagePainter.drawRoundedRect(QRect({0,0}, backgroundImage.size()),
                                     Ui::DesignSystem::button().borderRadius(),
                                     Ui::DesignSystem::button().borderRadius());
        //
        // Тень рисуем только в случае, если кнопка имеет установленный фон
        //
        if (d->isContained) {
            const qreal shadowBlurRadius = std::max(Ui::DesignSystem::button().minimumShadowBlurRadius(),
                                                    d->shadowBlurRadiusAnimation.currentValue().toReal());
            const QPixmap shadow
                    = ImageHelper::dropShadow(backgroundImage,
                                              Ui::DesignSystem::button().shadowMargins(),
                                              shadowBlurRadius,
                                              Ui::DesignSystem::color().shadow());
            painter.drawPixmap(0, 0, shadow);
        }
        //
        // ... собственно отрисовка фона
        //
        painter.setPen(Qt::NoPen);
        painter.setBrush(backgroundColor);
        painter.drawRoundedRect(backgroundRect,
                                Ui::DesignSystem::button().borderRadius(),
                                Ui::DesignSystem::button().borderRadius());
    }

    //
    // Декорация
    //
    if (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
        || d->decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        painter.setClipRect(contentsRect());
        painter.setPen(Qt::NoPen);
        painter.setBrush(textColor());
        painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
        painter.drawEllipse(d->decorationCenterPosition, d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(1.0);
        painter.setClipRect(QRect(), Qt::NoClip);
    }

    //
    // Рисуем текст
    //
    painter.setPen(isEnabled()
                   ? textColor()
                   : ColorHelper::transparent(textColor(), Ui::DesignSystem::disabledTextOpacity()));
    //
    QRectF buttonInnerRect = contentsRect().marginsRemoved(Ui::DesignSystem::button().margins().toMargins());
    const qreal textWidth = TextHelper::fineTextWidth(d->text, Ui::DesignSystem::font().button());
    //
    // ... если иконка задана, рисуем иконку и корректируем область отрисовки текста
    //
    if (!d->icon.isEmpty()) {
        const QSizeF iconSize = Ui::DesignSystem::button().iconSize();
        const qreal textWithIconWidth = iconSize.width() + Ui::DesignSystem::button().spacing() + textWidth;
        const qreal iconX = buttonInnerRect.x()
                            + (d->isContained
                               ? ((buttonInnerRect.width() - textWithIconWidth) / 2.0)
                               : 0.0);
        const QRectF iconRect(QPointF(iconX, buttonInnerRect.top()), QSizeF(iconSize.width(), buttonInnerRect.height()));
        painter.setFont(Ui::DesignSystem::font().iconsMid());
        painter.drawText(iconRect, Qt::AlignCenter, d->icon);

        buttonInnerRect.setX(iconRect.right() + Ui::DesignSystem::button().spacing());
        buttonInnerRect.setWidth(textWidth);
    }
    //
    // ... а если иконки нет, то просто корректируем область в которой будет рисоваться текст
    //
    else {
        const qreal textX = buttonInnerRect.x()
                            + (d->isContained
                               ? ((buttonInnerRect.width() - textWidth) / 2.0)
                               : 0.0);
        buttonInnerRect.setX(textX);
        buttonInnerRect.setWidth(textWidth);
    }
    //
    painter.setFont(Ui::DesignSystem::font().button());
    painter.drawText(buttonInnerRect, Qt::AlignCenter, d->text);
}

void Button::enterEvent(QEvent* _event)
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

void Button::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setContentsMargins(Ui::DesignSystem::button().shadowMargins().toMargins());
    d->shadowBlurRadiusAnimation.setStartValue(Ui::DesignSystem::button().minimumShadowBlurRadius());
    d->shadowBlurRadiusAnimation.setEndValue(Ui::DesignSystem::button().maximumShadowBlurRadius());

    updateGeometry();
    update();
}

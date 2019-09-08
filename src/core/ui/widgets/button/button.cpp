#include "button.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/image_helper.h>

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

    QString text;
    bool isContained = false;

    /**
     * @brief  Декорации кнопки при клике
     */
    QPointF decorationCenterPosition;
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
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
}

void Button::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****


Button::Button(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    setAttribute(Qt::WA_Hover);

    designSystemChangeEvent(nullptr);

    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
}

Button::~Button() = default;

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

QSize Button::sizeHint() const
{
    const qreal width = Ui::DesignSystem::button().shadowMargins().top()
                        + std::max(Ui::DesignSystem::button().minimumWidth(),
                                   Ui::DesignSystem::button().margins().left()
                                   + QFontMetrics(Ui::DesignSystem::font().button()).width(d->text)
                                   + Ui::DesignSystem::button().margins().right())
                        + Ui::DesignSystem::button().shadowMargins().bottom();
    const qreal height = Ui::DesignSystem::button().shadowMargins().top()
                         + Ui::DesignSystem::button().height()
                         + Ui::DesignSystem::button().shadowMargins().bottom();
    return QSize(static_cast<int>(width), static_cast<int>(height));
}

void Button::paintEvent(QPaintEvent* _event)
{
    Q_UNUSED(_event);

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
    } else if (underMouse()) {
        backgroundColor = this->backgroundColor();
        backgroundColor.setAlphaF(0.2);
    }
    if (backgroundColor.isValid()) {
        QPixmap image(backgroundRect.size());
        image.fill(Qt::transparent);
        QPainter imagePainter(&image);
        imagePainter.setPen(Qt::NoPen);
        imagePainter.setBrush(backgroundColor);
        imagePainter.drawRoundedRect(QRect({0,0}, image.size()),
                                     Ui::DesignSystem::button().borderRadius(),
                                     Ui::DesignSystem::button().borderRadius());
        //
        // Тень рисуем только в случае, если кнопка имеет установленный фон
        //
        if (d->isContained) {
            const QPixmap shadow
                    = ImageHelper::dropShadow(image,
                                              QMarginsF(Ui::DesignSystem::button().shadowMargins().left(),
                                                        Ui::DesignSystem::button().shadowMargins().top(),
                                                        Ui::DesignSystem::button().shadowMargins().right(),
                                                        Ui::DesignSystem::button().minimumShadowHeight()),
                                              Ui::DesignSystem::button().shadowBlurRadius(),
                                              Ui::DesignSystem::color().shadow());
            painter.drawPixmap(0, 0, shadow);
        }
        painter.drawPixmap(backgroundRect.topLeft(), image);
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
    painter.setFont(Ui::DesignSystem::font().button());
    painter.setPen(textColor());
    painter.drawText(contentsRect(), Qt::AlignCenter, d->text);
}

void Button::mousePressEvent(QMouseEvent* _event)
{
    d->decorationCenterPosition = _event->pos();
    d->decorationRadiusAnimation.setEndValue(static_cast<qreal>(width()));
    d->animateClick();
}

void Button::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    if (!rect().contains(_event->pos())) {
        return;
    }

    emit clicked();
}

void Button::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    setContentsMargins(Ui::DesignSystem::button().shadowMargins().toMargins());
    updateGeometry();
    update();
}

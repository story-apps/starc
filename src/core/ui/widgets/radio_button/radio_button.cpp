#include "radio_button.h"

#include <ui/design_system/design_system.h>

#include <utils/helpers/text_helper.h>

#include <QPainter>
#include <QPaintEvent>
#include <QTimer>
#include <QVariantAnimation>


class RadioButton::Implementation
{
public:
    Implementation();

    /**
     * @brief Анимировать клик
     */
    void animateClick();


    bool isChecked = false;
    QString text;

    /**
     * @brief  Декорации переключателя при клике
     */
    QVariantAnimation decorationRadiusAnimation;
    QVariantAnimation decorationOpacityAnimation;
};

RadioButton::Implementation::Implementation()
{
    decorationRadiusAnimation.setEasingCurve(QEasingCurve::OutQuad);
    decorationRadiusAnimation.setDuration(160);

    decorationOpacityAnimation.setEasingCurve(QEasingCurve::InQuad);
    decorationOpacityAnimation.setStartValue(0.5);
    decorationOpacityAnimation.setEndValue(0.0);
    decorationOpacityAnimation.setDuration(160);
}

void RadioButton::Implementation::animateClick()
{
    decorationOpacityAnimation.setCurrentTime(0);
    decorationRadiusAnimation.start();
    decorationOpacityAnimation.start();
}


// ****


RadioButton::RadioButton(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation)
{
    connect(&d->decorationRadiusAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });
    connect(&d->decorationOpacityAnimation, &QVariantAnimation::valueChanged, this, [this] { update(); });

    designSystemChangeEvent(nullptr);
}

RadioButton::~RadioButton() = default;

bool RadioButton::isChecked() const
{
    return d->isChecked;
}

void RadioButton::setChecked(bool _checked)
{
    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    emit checkedChanged(d->isChecked);
    update();
}

void RadioButton::setText(const QString& _text)
{
    if (d->text == _text) {
        return;
    }

    d->text = _text;
    updateGeometry();
    update();
}

QSize RadioButton::sizeHint() const
{
    return QSize(static_cast<int>(Ui::DesignSystem::radioButton().margins().left()
                                  + Ui::DesignSystem::radioButton().iconSize().width()
                                  + Ui::DesignSystem::radioButton().spacing()
                                  + TextHelper::fineTextWidth(d->text, Ui::DesignSystem::font().subtitle1())
                                  + Ui::DesignSystem::radioButton().margins().right()),
                 static_cast<int>(Ui::DesignSystem::radioButton().height()));
}

void RadioButton::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());

    //
    // Рисуем декорацию переключателя
    //
    const QRectF iconRect(QPointF(Ui::DesignSystem::radioButton().margins().left(),
                                  Ui::DesignSystem::radioButton().margins().top()),
                          Ui::DesignSystem::radioButton().iconSize());
    if (d->decorationRadiusAnimation.state() == QVariantAnimation::Running
        || d->decorationOpacityAnimation.state() == QVariantAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().secondary());
        painter.setOpacity(d->decorationOpacityAnimation.currentValue().toReal());
        painter.drawEllipse(iconRect.center(), d->decorationRadiusAnimation.currentValue().toReal(),
                            d->decorationRadiusAnimation.currentValue().toReal());
        painter.setOpacity(1.0);
    }

    //
    // Рисуем сам переключатель
    //
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.setPen(d->isChecked ? Ui::DesignSystem::color().secondary() : textColor());
    painter.drawText(iconRect, Qt::AlignCenter, d->isChecked ? "\uf43e" : "\uf43d");

    //
    // Рисуем текст
    //
    painter.setFont(Ui::DesignSystem::font().subtitle1());
    painter.setPen(textColor());
    const qreal textRectX = iconRect.right() + Ui::DesignSystem::radioButton().spacing();
    const QRectF textRect(textRectX, 0, width() - textRectX, height());
    painter.drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, d->text);
}

void RadioButton::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    if (!rect().contains(_event->pos())) {
        return;
    }

    setChecked(true);
    d->animateClick();
}

void RadioButton::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->decorationRadiusAnimation.setStartValue(Ui::DesignSystem::radioButton().iconSize().height() / 2.0);
    d->decorationRadiusAnimation.setEndValue(Ui::DesignSystem::radioButton().height() / 2.5);

    updateGeometry();
    update();
}

#include "icon_button.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/icon_helper.h>
#include <utils/helpers/text_helper.h>

#include <QPaintEvent>
#include <QPainter>

#include <optional>


class IconButton::Implementation
{
public:
    explicit Implementation(IconButton* _q);

    /**
     * @brief Обновить радиус анимации в зависимости от шрифта
     */
    void updateAnimationRadius();


    IconButton* q = nullptr;

    bool isCheckable = false;
    bool isChecked = false;
    QString icon;

    /**
     * @brief Кастомный шрифт
     */
    std::optional<QFont> customFont;

    /**
     * @brief  Декорации переключателя при клике
     */
    ClickAnimation decorationAnimation;
};

IconButton::Implementation::Implementation(IconButton* _q)
    : q(_q)
{
}

void IconButton::Implementation::updateAnimationRadius()
{
    const auto height = std::max(q->minimumHeight(), q->sizeHint().height());
    decorationAnimation.setRadiusInterval(height / 4.0, height / 2.5);
}


// ****


IconButton::IconButton(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&IconButton::update));
}

IconButton::~IconButton() = default;

void IconButton::setCheckable(bool _checkable)
{
    if (d->isCheckable == _checkable) {
        return;
    }

    d->isCheckable = _checkable;
    update();
}

bool IconButton::isChecked() const
{
    return d->isChecked;
}

void IconButton::setChecked(bool _checked)
{
    if (!d->isCheckable) {
        return;
    }

    if (d->isChecked == _checked) {
        return;
    }

    d->isChecked = _checked;
    emit checkedChanged(d->isChecked);
    update();
}

void IconButton::setIcon(const QString& _icon)
{
    if (d->icon == _icon) {
        return;
    }

    d->icon = _icon;
    update();
}

void IconButton::setCustomFont(const QFont& _font)
{
    if (d->customFont == _font) {
        return;
    }

    d->customFont = _font;
    d->updateAnimationRadius();
    updateGeometry();
    update();
}

QSize IconButton::sizeHint() const
{
    return QRectF({}, Ui::DesignSystem::iconButton().size())
        .marginsAdded(contentsMargins())
        .size()
        .toSize();
}

void IconButton::paintEvent(QPaintEvent* _event)
{
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    //
    // Заливаем фон
    //
    painter.fillRect(_event->rect(), backgroundColor());
    painter.translate(contentsRect().topLeft());

    //
    // Рисуем декорацию кнопки
    //
    const auto iconRect = d->customFont.has_value()
        ? contentsRect()
        : QRectF(QPointF(Ui::DesignSystem::iconButton().margins().left(),
                         Ui::DesignSystem::iconButton().margins().top()),
                 Ui::DesignSystem::iconButton().iconSize());
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(Ui::DesignSystem::color().accent());
        painter.setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        painter.drawEllipse(iconRect.center(), radius, radius);
        painter.setOpacity(1.0);
    }

    //
    // Рисуем иконку
    //
    painter.setFont(d->customFont.value_or(Ui::DesignSystem::font().iconsMid()));
    const auto iconColor = d->isChecked ? Ui::DesignSystem::color().accent() : textColor();
    painter.setPen(
        isEnabled() ? iconColor
                    : ColorHelper::transparent(iconColor, Ui::DesignSystem::disabledTextOpacity()));
    painter.drawText(iconRect, Qt::AlignCenter, IconHelper::directedIcon(d->icon));
}

void IconButton::mousePressEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event)

    if (!rect().contains(_event->pos())) {
        return;
    }

    d->decorationAnimation.start();
}

void IconButton::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    if (!rect().contains(_event->pos())) {
        return;
    }

    setChecked(!d->isChecked);
    emit clicked();
}

void IconButton::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->updateAnimationRadius();

    updateGeometry();
    update();
}

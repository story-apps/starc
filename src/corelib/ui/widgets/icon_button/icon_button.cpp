#include "icon_button.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/animations/click_animation.h>
#include <utils/helpers/icon_helper.h>

#include <QPaintEvent>
#include <QPainter>


class IconButton::Implementation
{
public:
    bool isCheckable = false;
    bool isChecked = false;
    QString icon;

    /**
     * @brief  Декорации переключателя при клике
     */
    ClickAnimation decorationAnimation;
};


// ****


IconButton::IconButton(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation)
{
    setFocusPolicy(Qt::StrongFocus);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

    connect(&d->decorationAnimation, &ClickAnimation::valueChanged, this,
            qOverload<>(&IconButton::update));

    designSystemChangeEvent(nullptr);
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

QSize IconButton::sizeHint() const
{
    return QRectF({}, Ui::DesignSystem::toggleButton().size())
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
    const QRectF iconRect(QPointF(Ui::DesignSystem::toggleButton().margins().left(),
                                  Ui::DesignSystem::toggleButton().margins().top()),
                          Ui::DesignSystem::toggleButton().iconSize());
    if (d->decorationAnimation.state() == ClickAnimation::Running) {
        painter.setPen(Qt::NoPen);
        painter.setBrush(isChecked() ? Ui::DesignSystem::color().secondary() : textColor());
        painter.setOpacity(d->decorationAnimation.opacity());
        const auto radius = d->decorationAnimation.radius();
        painter.drawEllipse(iconRect.center(), radius, radius);
        painter.setOpacity(1.0);
    }

    //
    // Рисуем иконку
    //
    painter.setFont(Ui::DesignSystem::font().iconsMid());
    painter.setPen(d->isChecked ? Ui::DesignSystem::color().secondary() : textColor());
    painter.drawText(iconRect, Qt::AlignCenter, IconHelper::directedIcon(d->icon));
}

void IconButton::mouseReleaseEvent(QMouseEvent* _event)
{
    Q_UNUSED(_event);

    if (!rect().contains(_event->pos())) {
        return;
    }

    setChecked(!d->isChecked);
    d->decorationAnimation.start();

    emit clicked();
}

void IconButton::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event);

    d->decorationAnimation.setRadiusInterval(
        Ui::DesignSystem::toggleButton().iconSize().height() / 2.0,
        Ui::DesignSystem::toggleButton().size().height() / 2.5);

    updateGeometry();
    update();
}

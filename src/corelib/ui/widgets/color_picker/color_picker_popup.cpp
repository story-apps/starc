#include "color_picker_popup.h"

#include "color_picker.h"

#include <ui/design_system/design_system.h>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QScreen>
#include <QVariantAnimation>

class ColorPickerPopup::Implementation
{
public:
    explicit Implementation(ColorPickerPopup* _q);

    bool autoHide = true;
    ColorPicker* colorPicker = nullptr;
    QVariantAnimation heightAnimation;

    /**
     * @brief Виджет, за событиями которого мы следим, чтобы при потере фокуса скрыть попап
     */
    QWidget* watchedWidget = nullptr;
};

ColorPickerPopup::Implementation::Implementation(ColorPickerPopup* _q)
    : colorPicker(new ColorPicker(_q))
{
    heightAnimation.setEasingCurve(QEasingCurve::OutQuint);
    heightAnimation.setDuration(240);
    heightAnimation.setStartValue(0);
    heightAnimation.setEndValue(0);
}


// ****


ColorPickerPopup::ColorPickerPopup(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    setFocusPolicy(Qt::StrongFocus);
    hide();

    auto popupLayout = new QHBoxLayout;
    popupLayout->setContentsMargins({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(d->colorPicker);
    setContentLayout(popupLayout);

    connect(d->colorPicker, &ColorPicker::selectedColorChanged, this, [this](const QColor& _color) {
        emit selectedColorChanged(_color);
        if (_color.isValid() && d->autoHide) {
            hidePopup();
        }
    });
    connect(&d->heightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                const auto height = _value.toInt();
                resize(width(), height);
            });
    connect(&d->heightAnimation, &QVariantAnimation::finished, this, [this] {
        if (d->heightAnimation.direction() == QAbstractAnimation::Backward) {
            hide();
        }
    });
}

ColorPickerPopup::~ColorPickerPopup() = default;

void ColorPickerPopup::setCustomPalette(const QVector<QColor>& _palette)
{
    d->colorPicker->setCustomPalette(_palette);
}

void ColorPickerPopup::setColorCanBeDeselected(bool _can)
{
    d->colorPicker->setColorCanBeDeselected(_can);
}

void ColorPickerPopup::setAutoHideOnSelection(bool _autoHide)
{
    d->autoHide = _autoHide;
}

QColor ColorPickerPopup::selectedColor() const
{
    return d->colorPicker->selectedColor();
}

void ColorPickerPopup::setSelectedColor(const QColor& _color)
{
    d->colorPicker->setSelectedColor(_color);
}

bool ColorPickerPopup::isPopupShown() const
{
    //
    // Попам видимый и не находится в процессе скрытия в данный момент
    //
    return isVisible() && d->heightAnimation.direction() == QAbstractAnimation::Forward;
}

void ColorPickerPopup::showPopup(QWidget* _parent, Qt::Alignment _alignment)
{
    if (isVisible()) {
        return;
    }

    Q_ASSERT(_parent);
    if (_parent == nullptr) {
        return;
    }

    d->watchedWidget = _parent;
    _parent->installEventFilter(this);

    //
    // Размер панели с цветами мог измениться, пока виджет был скрыт, но для того, чтобы всё
    // обновилось, виджет нужно показать, поэтому тут форсим перерасчёт размеров лейаута
    //
    contentLayout()->invalidate();

    const auto sizeHint = this->sizeHint();
    resize(sizeHint.width(), 1);

    QPoint leftTop;
    if (_alignment.testFlag(Qt::AlignHCenter)) {
        leftTop
            = QPoint(_parent->rect().center().x() - sizeHint.width() / 2,
                     _parent->rect().bottom() - Ui::DesignSystem::textField().margins().bottom());
    } else if (_alignment.testFlag(Qt::AlignRight)) {
        leftTop
            = QPoint(_parent->rect().right() - sizeHint.width(),
                     _parent->rect().bottom() - Ui::DesignSystem::textField().margins().bottom());
    }
    auto pos = _parent->mapToGlobal(leftTop);

    //
    // Если не влезает внизу экрана, то смещаем наверх от нижнего края
    //
    const auto screenGeometry = screen()->geometry();
    if (pos.y() + sizeHint.height() > screenGeometry.bottom()) {
        pos.setY(screenGeometry.bottom() - sizeHint.height());
    }

    move(pos);
    show();
    setFocus();

    d->heightAnimation.setDirection(QVariantAnimation::Forward);
    d->heightAnimation.setEndValue(sizeHint.height());
    d->heightAnimation.start();
}

void ColorPickerPopup::hidePopup()
{
    d->heightAnimation.setDirection(QVariantAnimation::Backward);
    d->heightAnimation.start();

    if (d->watchedWidget) {
        d->watchedWidget->removeEventFilter(this);
    }
}

void ColorPickerPopup::processBackgroundColorChange()
{
    d->colorPicker->setBackgroundColor(backgroundColor());
}

void ColorPickerPopup::processTextColorChange()
{
    d->colorPicker->setTextColor(textColor());
}

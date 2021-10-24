#include "color_picker_popup.h"

#include "color_picker.h"

#include <ui/design_system/design_system.h>

#include <QHBoxLayout>
#include <QVariantAnimation>

class ColorPickerPopup::Implementation
{
public:
    explicit Implementation(ColorPickerPopup* _q);

    bool isPopupShown = false;
    ColorPicker* colorPicker = nullptr;
    QVariantAnimation heightAnimation;
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
    setFocusPolicy(Qt::StrongFocus);
    setWindowFlags(Qt::SplashScreen | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
    hide();

    auto popupLayout = new QHBoxLayout;
    popupLayout->setMargin({});
    popupLayout->setSpacing(0);
    popupLayout->addWidget(d->colorPicker);
    setLayoutReimpl(popupLayout);

    connect(d->colorPicker, &ColorPicker::colorSelected, this, [this](const QColor& _color) {
        hidePopup();
        emit colorSelected(_color);
    });
    connect(&d->heightAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                const auto height = _value.toInt();
                resize(width(), height);
            });
    connect(&d->heightAnimation, &QVariantAnimation::finished, this, [this] {
        if (!d->isPopupShown) {
            hide();
        }
    });
}

ColorPickerPopup::~ColorPickerPopup() = default;

void ColorPickerPopup::setSelectedColor(const QColor& _color)
{
    d->colorPicker->setSelectedColor(_color);
}

bool ColorPickerPopup::isPopupShown() const
{
    return d->isPopupShown;
}

void ColorPickerPopup::showPopup(QWidget* _parent)
{
    d->isPopupShown = true;

    resize(sizeHint().width(), 0);

    const auto left
        = QPoint(_parent->rect().center().x() - width() / 2,
                 _parent->rect().bottom() - Ui::DesignSystem::textField().margins().bottom());
    const auto pos = _parent->mapToGlobal(left);
    move(pos);
    show();
    setFocus();

    d->heightAnimation.setDirection(QVariantAnimation::Forward);
    d->heightAnimation.setEndValue(sizeHint().height());
    d->heightAnimation.start();
}

void ColorPickerPopup::hidePopup()
{
    d->isPopupShown = false;

    d->heightAnimation.setDirection(QVariantAnimation::Backward);
    d->heightAnimation.start();
}

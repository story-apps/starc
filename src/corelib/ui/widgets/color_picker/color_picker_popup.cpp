#include "color_picker_popup.h"

#include "color_picker.h"

#include <ui/design_system/design_system.h>

#include <QHBoxLayout>
#include <QKeyEvent>
#include <QVariantAnimation>

class ColorPickerPopup::Implementation
{
public:
    explicit Implementation(ColorPickerPopup* _q);

    bool isPopupShown = false;
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
    setFocusPolicy(Qt::StrongFocus);
    setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint);
    setAttribute(Qt::WA_Hover, false);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_ShowWithoutActivating);
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
        if (!d->isPopupShown) {
            hide();
        }
    });
}

ColorPickerPopup::~ColorPickerPopup() = default;

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
    return d->isPopupShown;
}

void ColorPickerPopup::showPopup(QWidget* _parent, Qt::Alignment _alignment)
{
    if (_parent) {
        d->watchedWidget = _parent;
        _parent->installEventFilter(this);
    }

    d->isPopupShown = true;

    resize(sizeHint().width(), 0);

    QPoint leftTop;
    if (_alignment.testFlag(Qt::AlignHCenter)) {
        leftTop
            = QPoint(_parent->rect().center().x() - width() / 2,
                     _parent->rect().bottom() - Ui::DesignSystem::textField().margins().bottom());
    } else if (_alignment.testFlag(Qt::AlignRight)) {
        leftTop
            = QPoint(_parent->rect().right() - width(),
                     _parent->rect().bottom() - Ui::DesignSystem::textField().margins().bottom());
    }
    const auto pos = _parent->mapToGlobal(leftTop);
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

bool ColorPickerPopup::eventFilter(QObject* _watched, QEvent* _event)
{
    if (_watched == d->watchedWidget) {
        if (_event->type() == QEvent::KeyPress) {
            auto keyEvent = static_cast<QKeyEvent*>(_event);
            if (keyEvent->key() == Qt::Key_Escape) {
                hidePopup();
            }
        } else if (_event->type() == QEvent::FocusOut) {
            if (!hasFocus()) {
                hidePopup();
            }
        }
    }

    return Widget::eventFilter(_watched, _event);
}

#include "color_picker.h"

#include "color_2d_slider.h"
#include "color_hue_slider.h"
#include "color_palette.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>

#include <QVBoxLayout>


class ColorPicker::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    ColorPallete* colorPallete = nullptr;
    Widget* customColorPanel = nullptr;
    color_widgets::Color2DSlider* colorSlider = nullptr;
    ColorHueSlider* colorHueSlider = nullptr;
    Button* cancelButton = nullptr;
    Button* addButton = nullptr;
    QHBoxLayout* buttonsLayout = nullptr;
};

ColorPicker::Implementation::Implementation(QWidget* _parent)
    : colorPallete(new ColorPallete(_parent))
    , customColorPanel(new Widget(_parent))
    , colorSlider(new color_widgets::Color2DSlider(_parent))
    , colorHueSlider(new ColorHueSlider(_parent))
    , cancelButton(new Button(_parent))
    , addButton(new Button(_parent))
    , buttonsLayout(new QHBoxLayout)
{
    customColorPanel->hide();
    cancelButton->setFocusPolicy(Qt::NoFocus);
    addButton->setFocusPolicy(Qt::NoFocus);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(addButton);
}


// ****


ColorPicker::ColorPicker(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{

    QVBoxLayout* customColorPanelLayout = new QVBoxLayout(d->customColorPanel);
    customColorPanelLayout->setContentsMargins({});
    customColorPanelLayout->setSpacing(0);
    customColorPanelLayout->addWidget(d->colorSlider, 1);
    customColorPanelLayout->addWidget(d->colorHueSlider);
    customColorPanelLayout->addLayout(d->buttonsLayout);

    setCurrentWidget(d->colorPallete);
    addWidget(d->customColorPanel);

    d->colorHueSlider->setHue(d->colorSlider->hue());

    connect(d->colorPallete, &ColorPallete::selectedColorChanged, this,
            &ColorPicker::selectedColorChanged);
    connect(d->colorPallete, &ColorPallete::addCustomColorPressed, this,
            [this] { setCurrentWidget(d->customColorPanel); });
    connect(d->colorHueSlider, &ColorHueSlider::hueChanged, this,
            [this](qreal _hue) { d->colorSlider->setHue(_hue); });
    connect(d->cancelButton, &Button::clicked, this, [this] { setCurrentWidget(d->colorPallete); });
    connect(d->addButton, &Button::clicked, this, [this] {
        d->colorPallete->addCustomColor(d->colorSlider->color());
        setCurrentWidget(d->colorPallete);
    });

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ColorPicker::~ColorPicker() = default;

QColor ColorPicker::selectedColor() const
{
    return d->colorPallete->selectedColor();
}

void ColorPicker::setSelectedColor(const QColor& _color)
{
    d->colorPallete->setSelectedColor(_color);
}

void ColorPicker::updateTranslations()
{
    d->cancelButton->setText(tr("Cancel"));
    d->addButton->setText(tr("Add"));
}

void ColorPicker::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().background());

    d->colorPallete->setBackgroundColor(Ui::DesignSystem::color().background());
    d->colorPallete->setTextColor(Ui::DesignSystem::color().onBackground());

    d->customColorPanel->setBackgroundColor(Ui::DesignSystem::color().background());
    //
    d->colorSlider->setBackgroundColor(Ui::DesignSystem::color().background());
    d->colorSlider->setTextColor(Ui::DesignSystem::color().onBackground());
    d->colorHueSlider->setBackgroundColor(Ui::DesignSystem::color().background());
    d->colorHueSlider->setTextColor(Ui::DesignSystem::color().onBackground());
    d->colorHueSlider->setFixedHeight(Ui::DesignSystem::layout().px24());
    //
    d->addButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->addButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().secondary());
    //
    d->buttonsLayout->setContentsMargins(
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px2(),
        Ui::DesignSystem::layout().px8(), Ui::DesignSystem::layout().px8());
}

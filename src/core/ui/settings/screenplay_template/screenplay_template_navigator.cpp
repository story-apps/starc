#include "screenplay_template_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>
#include <utils/helpers/color_helper.h>

#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

class ScreenplayTemplateNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    Body2Label* title = nullptr;
    CaptionLabel* metricsTitle = nullptr;
    RadioButton* mm = nullptr;
    RadioButton* inch = nullptr;
};

ScreenplayTemplateNavigator::Implementation::Implementation(QWidget* _parent)
    : title(new Body2Label(_parent))
    , metricsTitle(new CaptionLabel(_parent))
    , mm(new RadioButton(_parent))
    , inch(new RadioButton(_parent))
{
    auto metricsGroup = new RadioButtonGroup(_parent);
    metricsGroup->add(mm);
    metricsGroup->add(inch);
    mm->setChecked(true);
}


// ****


ScreenplayTemplateNavigator::ScreenplayTemplateNavigator(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->title);
    layout->addWidget(d->metricsTitle);
    layout->addWidget(d->mm);
    layout->addWidget(d->inch);
    layout->addStretch();

    connect(d->mm, &RadioButton::checkedChanged, this,
            &ScreenplayTemplateNavigator::mmCheckedChanged);

    designSystemChangeEvent(nullptr);
}

void ScreenplayTemplateNavigator::updateTranslations()
{
    d->title->setText(tr("Edit template parameters"));
    d->metricsTitle->setText(tr("Show template parameters in"));
    d->mm->setText(tr("Millimeters"));
    d->inch->setText(tr("Inches"));
}

void ScreenplayTemplateNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());

    for (auto widget : std::vector<Widget*>{ d->title, d->metricsTitle, d->mm, d->inch }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(DesignSystem::color().onPrimary());
    }

    d->title->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                    Ui::DesignSystem::inactiveTextOpacity()));
    d->title->setContentsMargins(
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px12(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24());
    d->metricsTitle->setContentsMargins(Ui::DesignSystem::layout().px24(), 0.0,
                                        Ui::DesignSystem::layout().px24(),
                                        Ui::DesignSystem::layout().px4());
}

ScreenplayTemplateNavigator::~ScreenplayTemplateNavigator() = default;

} // namespace Ui

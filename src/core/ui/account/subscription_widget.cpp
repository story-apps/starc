#include "subscription_widget.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QVBoxLayout>


namespace Ui {

class SubscriptionWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    H6Label* deviceName = nullptr;
    Body1Label* location = nullptr;
    IconsSmallLabel* lastUsedIcon = nullptr;
    Body1Label* lastUsed = nullptr;
    Button* terinateSession = nullptr;
    QVBoxLayout* layout = nullptr;
};

SubscriptionWidget::Implementation::Implementation(QWidget* _parent)
    : deviceName(new H6Label(_parent))
    , location(new Body1Label(_parent))
    , lastUsedIcon(new IconsSmallLabel(_parent))
    , lastUsed(new Body1Label(_parent))
    , terinateSession(new Button(_parent))
    , layout(new QVBoxLayout)
{
    lastUsedIcon->setIcon(u8"\U000F05E0");
}


// ****


SubscriptionWidget::SubscriptionWidget(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    d->layout->setContentsMargins({});
    d->layout->setSpacing(0);
    d->layout->addWidget(d->deviceName);
    d->layout->addWidget(d->location);
    const auto lastUsedLayout = new QHBoxLayout;
    lastUsedLayout->setContentsMargins({});
    lastUsedLayout->setSpacing(0);
    lastUsedLayout->addWidget(d->lastUsedIcon, 0, Qt::AlignCenter);
    lastUsedLayout->addWidget(d->lastUsed, 1);
    d->layout->addLayout(lastUsedLayout);
    d->layout->addWidget(d->terinateSession, 0, Qt::AlignRight);
    setContentLayout(d->layout);
}

SubscriptionWidget::~SubscriptionWidget()
{
}

void SubscriptionWidget::updateTranslations()
{
}

void SubscriptionWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    setBackgroundColor(Ui::DesignSystem::color().background());

    const auto labelColor = ColorHelper::transparent(DesignSystem::color().onBackground(),
                                                     DesignSystem::inactiveTextOpacity());
    for (auto label : std::vector<Widget*>{
             d->deviceName,
             d->location,
             d->lastUsed,
         }) {
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(labelColor);
    }
    d->deviceName->setTextColor(Ui::DesignSystem::color().onBackground());

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setRight(0);
    labelMargins.setBottom(0);
    d->deviceName->setContentsMargins(labelMargins);
    labelMargins.setTop(Ui::DesignSystem::layout().px8());
    d->location->setContentsMargins(labelMargins);
    labelMargins.setBottom(Ui::DesignSystem::layout().px24());
    //    if (d->sessionInfo.isCurrentDevice) {
    //        d->lastUsedIcon->setBackgroundColor(Ui::DesignSystem::color().background());
    //        d->lastUsedIcon->setTextColor(ColorHelper::transparent(
    //            Ui::DesignSystem::color().accent(), Ui::DesignSystem::inactiveTextOpacity()));
    //        d->lastUsedIcon->setContentsMargins(labelMargins);
    //        labelMargins.setLeft(Ui::DesignSystem::layout().px8());
    //    }
    d->lastUsed->setContentsMargins(labelMargins);

    d->terinateSession->setBackgroundColor(Ui::DesignSystem::color().accent());
    d->terinateSession->setTextColor(Ui::DesignSystem::color().accent());

    d->layout->setContentsMargins(0, 0, Ui::DesignSystem::layout().px16(),
                                  Ui::DesignSystem::layout().px16());
}

} // namespace Ui

#include "account_navigator.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/circular_progress_bar/circular_progress_bar.h>
#include <ui/widgets/label/label.h>

#include <utils/helpers/color_helper.h>

#include <QDate>
#include <QVBoxLayout>


namespace Ui
{

class AccountNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QString subscriptionEnd;

    QVBoxLayout* layout = nullptr;
    H6Label* titleLabel = nullptr;
    Body2Label* subtitleLabel = nullptr;
    Button* upgradeToProButton = nullptr;
    CircularProgressBar* cloudSpaceBar = nullptr;
    Body1Label* subscriptionAvailabilityLabel = nullptr;
    Button* renewSubscriptionButton = nullptr;
};

AccountNavigator::Implementation::Implementation(QWidget* _parent)
    : layout(new QVBoxLayout(_parent)),
      titleLabel(new H6Label(_parent)),
      subtitleLabel(new Body2Label(_parent)),
      upgradeToProButton(new Button(_parent)),
      cloudSpaceBar(new CircularProgressBar(_parent)),
      subscriptionAvailabilityLabel(new Body1Label(_parent)),
      renewSubscriptionButton(new Button(_parent))
{
    upgradeToProButton->setIcon(u8"\U000f042d");
    renewSubscriptionButton->setIcon(u8"\U000f006a");

    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(upgradeToProButton);
    layout->addWidget(cloudSpaceBar);
    layout->addWidget(subscriptionAvailabilityLabel);
    layout->addWidget(renewSubscriptionButton);
    layout->addStretch();
}


// ****

AccountNavigator::AccountNavigator(QWidget* _parent)
    : Widget(_parent),
      d(new Implementation(this))
{
    connect(d->upgradeToProButton, &Button::clicked, this, &AccountNavigator::upgradeToProPressed);
    connect(d->renewSubscriptionButton, &Button::clicked, this, &AccountNavigator::renewSubscriptionPressed);

    designSystemChangeEvent(nullptr);
}

void AccountNavigator::setSubscriptionEnd(const QString& _subscriptionEnd)
{
    d->subscriptionEnd = _subscriptionEnd;
    updateTranslations();
}

AccountNavigator::~AccountNavigator() = default;

void AccountNavigator::updateTranslations()
{
    d->titleLabel->setText("STARC");
    d->subtitleLabel->setText("Story Architect " + tr("free version"));
    d->upgradeToProButton->setText(tr("Upgrade to pro"));
    d->cloudSpaceBar->setText(tr("Used 0.34 Gb \nfrom 2 Gb"));
    d->subscriptionAvailabilityLabel->setText(
                d->subscriptionEnd.isEmpty()
                ? tr("Information about subscription not loaded")
                : tr("Cloud service subscription available to") + " "
                  + QDate::fromString(d->subscriptionEnd, "yyyy-MM-dd").toString(Qt::SystemLocaleShortDate));
    d->renewSubscriptionButton->setText(tr("Renew subscription"));
}

void AccountNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    setBackgroundColor(DesignSystem::color().primary());

    d->layout->setContentsMargins(
                QMarginsF(Ui::DesignSystem::layout().px2(),
                          Ui::DesignSystem::layout().px2(),
                          Ui::DesignSystem::layout().px2(),
                          Ui::DesignSystem::layout().px2()).toMargins());
    const QMarginsF titleMargins(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px4(),
                                 Ui::DesignSystem::layout().px24(), 0.0);
    d->titleLabel->setContentsMargins(titleMargins.toMargins());
    d->titleLabel->setBackgroundColor(DesignSystem::color().primary());
    d->titleLabel->setTextColor(DesignSystem::color().onPrimary());
    const QMarginsF subtitleMargins(Ui::DesignSystem::layout().px24(), 0.0,
                                       Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px4());
    d->subtitleLabel->setContentsMargins(subtitleMargins.toMargins());
    d->subtitleLabel->setBackgroundColor(DesignSystem::color().primary());
    d->subtitleLabel->setTextColor(
                ColorHelper::colorBetween(DesignSystem::color().primary(),
                                          DesignSystem::color().onPrimary()));
    const QMarginsF cloudSpaceBarRect(0, Ui::DesignSystem::layout().px24(), 0, Ui::DesignSystem::layout().px12());
    d->cloudSpaceBar->setContentsMargins(cloudSpaceBarRect.toMargins());
    d->cloudSpaceBar->setMinimumHeight(static_cast<int>(Ui::DesignSystem::layout().px16() * 12));
    d->cloudSpaceBar->setBackgroundColor(DesignSystem::color().primary());
    d->cloudSpaceBar->setTextColor(DesignSystem::color().onPrimary());
    d->cloudSpaceBar->setBarColor(DesignSystem::color().secondary());
    d->subscriptionAvailabilityLabel->setContentsMargins(titleMargins.toMargins());
    d->subscriptionAvailabilityLabel->setBackgroundColor(DesignSystem::color().primary());
    d->subscriptionAvailabilityLabel->setTextColor(DesignSystem::color().onPrimary());
    for (auto button : {d->upgradeToProButton, d->renewSubscriptionButton}) {
        button->setBackgroundColor(DesignSystem::color().secondary());
        button->setTextColor(DesignSystem::color().secondary());
    }
}

} // namespace Ui

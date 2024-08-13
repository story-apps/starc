#include "subscription_widget.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <utils/helpers/color_helper.h>

#include <QDateTime>
#include <QVBoxLayout>


namespace Ui {

class SubscriptionWidget::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    bool isActive = false;
    bool isLifetime = false;

    H6Label* subscriptionName = nullptr;
    Body1Label* minimumPrice = nullptr;
    CaptionLabel* trialInfo = nullptr;
    IconsSmallLabel* isActiveIcon = nullptr;
    Body1Label* isActiveInfo = nullptr;
    Body1Label* subscriptionDescription = nullptr;
    Button* tryButton = nullptr;
    Button* buyButton = nullptr;
    Button* buyLifetimeButton = nullptr;
    QVBoxLayout* layout = nullptr;
};

SubscriptionWidget::Implementation::Implementation(QWidget* _parent)
    : subscriptionName(new H6Label(_parent))
    , minimumPrice(new Body1Label(_parent))
    , trialInfo(new CaptionLabel(_parent))
    , isActiveIcon(new IconsSmallLabel(_parent))
    , isActiveInfo(new Body1Label(_parent))
    , subscriptionDescription(new Body1Label(_parent))
    , tryButton(new Button(_parent))
    , buyButton(new Button(_parent))
    , buyLifetimeButton(new Button(_parent))
    , layout(new QVBoxLayout)
{
    isActiveIcon->setIcon(u8"\U000F05E0");
    trialInfo->setAlignment(Qt::AlignCenter);

    layout->setContentsMargins({});
    layout->setSpacing(0);
    {
        auto titleLayout = new QHBoxLayout;
        titleLayout->setContentsMargins({});
        titleLayout->setSpacing(0);
        titleLayout->addWidget(subscriptionName);
        titleLayout->addStretch();
        titleLayout->addWidget(minimumPrice);
        titleLayout->addWidget(trialInfo);
        layout->addLayout(titleLayout);
    }
    {
        auto isActiveLayout = new QHBoxLayout;
        isActiveLayout->setContentsMargins({});
        isActiveLayout->setSpacing(0);
        isActiveLayout->addWidget(isActiveIcon, 0, Qt::AlignCenter);
        isActiveLayout->addWidget(isActiveInfo, 1);
        layout->addLayout(isActiveLayout);
    }
    layout->addWidget(subscriptionDescription, 1, Qt::AlignLeft | Qt::AlignTop);
    {
        auto buttonsLayout = new QHBoxLayout;
        buttonsLayout->setContentsMargins({});
        buttonsLayout->setSpacing(0);
        buttonsLayout->addStretch();
        buttonsLayout->addWidget(tryButton, 0, Qt::AlignBottom);
        buttonsLayout->addWidget(buyButton, 0, Qt::AlignBottom);
        buttonsLayout->addWidget(buyLifetimeButton, 0, Qt::AlignBottom);
        layout->addLayout(buttonsLayout);
    }
}


// ****


SubscriptionWidget::SubscriptionWidget(QWidget* _parent)
    : Card(_parent)
    , d(new Implementation(this))
{
    setContentLayout(d->layout);

    connect(d->tryButton, &Button::clicked, this, &SubscriptionWidget::tryPressed);
    connect(d->buyButton, &Button::clicked, this, &SubscriptionWidget::buyPressed);
    connect(d->buyLifetimeButton, &Button::clicked, this, &SubscriptionWidget::buyLifetimePressed);
}

SubscriptionWidget::~SubscriptionWidget() = default;

void SubscriptionWidget::setInfo(const QString& _name, const QString& _description)
{
    d->subscriptionName->setText(_name);
    d->subscriptionDescription->setText(_description);
}

void SubscriptionWidget::setStatus(bool _isActive, bool _isLifetime, const QDateTime& _activeUntil)
{
    d->isActive = _isActive;
    d->isLifetime = _isLifetime;

    d->isActiveIcon->setVisible(d->isActive);
    d->isActiveInfo->setVisible(d->isActive);
    d->isActiveInfo->setText(d->isLifetime
                                 ? tr("Lifetime access")
                                 : tr("Active until %1").arg(_activeUntil.toString("dd.MM.yyyy")));
}

void SubscriptionWidget::setPaymentOptions(const QVector<Domain::PaymentOption>& _paymentOptions)
{
    bool hasTrial = false;
    bool hasLifetime = false;
    qreal minimumPrice = 10000.0;
    for (const auto& option : _paymentOptions) {
        if (qFuzzyCompare(option.amount, 0.0)) {
            hasTrial = true;
            continue;
        }

        if (option.duration == Domain::PaymentDuration::Lifetime) {
            hasLifetime = true;
            continue;
        } else if (option.duration == Domain::PaymentDuration::Custom) {
            continue;
        }

        const auto monthPrice = option.totalAmount / 100.0 / static_cast<qreal>(option.duration);
        if (monthPrice < minimumPrice) {
            minimumPrice = monthPrice;
        }
    }

    d->minimumPrice->setVisible(!d->isLifetime);
    d->minimumPrice->setStrikeOut(hasTrial);
    d->minimumPrice->setTextColor(
        ColorHelper::transparent(DesignSystem::color().onBackground(),
                                 hasTrial ? DesignSystem::inactiveTextOpacity() : 1.0));
    d->minimumPrice->setText(tr("from $%1/mo").arg(minimumPrice, 0, 'f', 2));
    d->trialInfo->setVisible(hasTrial);
    d->tryButton->setVisible(hasTrial);
    d->buyButton->setVisible(!hasTrial && !d->isLifetime);
    d->buyButton->setText(d->isActive ? tr("Renew") : tr("Buy"));
    d->buyLifetimeButton->setVisible(!hasTrial && hasLifetime);
}

void SubscriptionWidget::updateTranslations()
{
    d->trialInfo->setText(tr("30 days for free"));
    d->tryButton->setText(tr("Try for free"));
    d->buyLifetimeButton->setText(tr("Buy lifetime"));
}

void SubscriptionWidget::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Card::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().background());

    for (auto label : std::vector<Widget*>{
             d->subscriptionName,
             d->minimumPrice,
             d->isActiveInfo,
             d->subscriptionDescription,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
    }
    d->trialInfo->setBackgroundColor(ColorHelper::transparent(DesignSystem::color().accent(),
                                                              DesignSystem::inactiveTextOpacity()));
    d->trialInfo->setTextColor(DesignSystem::color().onAccent());
    d->trialInfo->setBorderRadius(DesignSystem::layout().px4());
    d->trialInfo->setMinimumWidth(d->trialInfo->sizeHint().width() + DesignSystem::layout().px8());

    auto labelMargins = DesignSystem::label().margins().toMargins();
    labelMargins.setRight(0);
    labelMargins.setBottom(0);
    d->subscriptionName->setContentsMargins(labelMargins);
    d->minimumPrice->setContentsMargins(labelMargins);
    d->trialInfo->setContentsMargins(DesignSystem::layout().px8(), labelMargins.top(), 0, 0);
    labelMargins.setTop(DesignSystem::layout().px12());
    labelMargins.setBottom(DesignSystem::layout().px16());
    d->subscriptionDescription->setContentsMargins(labelMargins);
    labelMargins.setTop(DesignSystem::layout().px12());
    labelMargins.setBottom(DesignSystem::layout().px4());
    d->isActiveIcon->setBackgroundColor(DesignSystem::color().background());
    d->isActiveIcon->setTextColor(ColorHelper::transparent(DesignSystem::color().accent(),
                                                           DesignSystem::inactiveTextOpacity()));
    d->isActiveIcon->setContentsMargins(labelMargins);
    labelMargins.setLeft(DesignSystem::layout().px8());
    d->isActiveInfo->setContentsMargins(labelMargins);

    for (auto button : {
             d->tryButton,
             d->buyButton,
             d->buyLifetimeButton,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    d->layout->setContentsMargins(0, 0, DesignSystem::layout().px16(),
                                  DesignSystem::layout().px16());
}

} // namespace Ui

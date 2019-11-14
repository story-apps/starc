#include "upgrade_to_pro_dialog.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/button/button.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>

#include <QHBoxLayout>


namespace Ui {

class UpgradeToProDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    RadioButton* paypal = nullptr;
    RadioButton* bankCard = nullptr;
    RadioButton* yandexMoney = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* upgradeButton = nullptr;
    Button* cancelButton = nullptr;
};

UpgradeToProDialog::Implementation::Implementation(QWidget* _parent)
    : paypal(new RadioButton(_parent)),
      bankCard(new RadioButton(_parent)),
      yandexMoney(new RadioButton(_parent)),
      buttonsLayout(new QHBoxLayout),
      upgradeButton(new Button(_parent)),
      cancelButton(new Button(_parent))
{
    paypal->setChecked(true);

    RadioButtonGroup* paymentGroup = new RadioButtonGroup(_parent);
    paymentGroup->add(paypal);
    paymentGroup->add(bankCard);
    paymentGroup->add(yandexMoney);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(upgradeButton);
    buttonsLayout->addWidget(cancelButton);

#ifdef Q_OS_MAC
    paypal->hide();
    bankCard->hide();
    yandexMoney->hide();
#endif
}


// ****


UpgradeToProDialog::UpgradeToProDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    contentsLayout()->addWidget(d->paypal, 0, 0);
    contentsLayout()->addWidget(d->bankCard, 1, 0);
    contentsLayout()->addWidget(d->yandexMoney, 2, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 3, 0);

    connect(d->upgradeButton, &Button::clicked, this, &UpgradeToProDialog::upgradePressed);
    connect(d->cancelButton, &Button::clicked, this, &UpgradeToProDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

UpgradeToProDialog::~UpgradeToProDialog() = default;

QWidget* UpgradeToProDialog::focusedWidgetAfterShow() const
{
    return d->paypal;
}

void UpgradeToProDialog::updateTranslations()
{
    setTitle(tr("Upgrade to pro for $39.99 lifetime"));
    d->paypal->setText(tr("via PayPal"));
    d->bankCard->setText(tr("via bank card"));
    d->yandexMoney->setText(tr("via Yandex.Money"));
    d->upgradeButton->setText(tr("Upgrade"));
    d->cancelButton->setText(tr("Cancel"));
}

void UpgradeToProDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    for (auto radioButton : { d->paypal, d->bankCard, d->yandexMoney }) {
        radioButton->setBackgroundColor(Ui::DesignSystem::color().background());
        radioButton->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->upgradeButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

} // namespace Ui

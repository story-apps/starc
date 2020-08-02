#include "renew_subscription_dialog.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/button/button.h>
#include <ui/widgets/radio_button/radio_button.h>
#include <ui/widgets/radio_button/radio_button_group.h>

#include <QHBoxLayout>


namespace Ui {

class RenewSubscriptionDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    RadioButton* renew1Month = nullptr;
    RadioButton* renew2Month = nullptr;
    RadioButton* renew3Month = nullptr;
    RadioButton* renew6Month = nullptr;
    RadioButton* renew12Month = nullptr;

    RadioButton* paypal = nullptr;
    RadioButton* bankCard = nullptr;
    RadioButton* yandexMoney = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* renewButton = nullptr;
    Button* cancelButton = nullptr;
};

RenewSubscriptionDialog::Implementation::Implementation(QWidget* _parent)
    : renew1Month(new RadioButton(_parent)),
      renew2Month(new RadioButton(_parent)),
      renew3Month(new RadioButton(_parent)),
      renew6Month(new RadioButton(_parent)),
      renew12Month(new RadioButton(_parent)),
      paypal(new RadioButton(_parent)),
      bankCard(new RadioButton(_parent)),
      yandexMoney(new RadioButton(_parent)),
      buttonsLayout(new QHBoxLayout),
      renewButton(new Button(_parent)),
      cancelButton(new Button(_parent))
{
    renew1Month->setChecked(true);
    paypal->setChecked(true);

    RadioButtonGroup* monthGroup = new RadioButtonGroup(_parent);
    monthGroup->add(renew1Month);
    monthGroup->add(renew2Month);
    monthGroup->add(renew3Month);
    monthGroup->add(renew6Month);
    monthGroup->add(renew12Month);

    RadioButtonGroup* paymentGroup = new RadioButtonGroup(_parent);
    paymentGroup->add(paypal);
    paymentGroup->add(bankCard);
    paymentGroup->add(yandexMoney);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(renewButton);
    buttonsLayout->addWidget(cancelButton);

#ifdef Q_OS_MAC
    paypal->hide();
    bankCard->hide();
    yandexMoney->hide();
#endif
}


// ****


RenewSubscriptionDialog::RenewSubscriptionDialog(QWidget* _parent)
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    contentsLayout()->addWidget(d->renew1Month, 0, 0);
    contentsLayout()->addWidget(d->renew2Month, 1, 0);
    contentsLayout()->addWidget(d->renew3Month, 2, 0);
    contentsLayout()->addWidget(d->renew6Month, 3, 0);
    contentsLayout()->addWidget(d->renew12Month, 4, 0);
    contentsLayout()->setRowMinimumHeight(5, 0); // отступ между месяцами и способами оплаты
    contentsLayout()->addWidget(d->paypal, 6, 0);
    contentsLayout()->addWidget(d->bankCard, 7, 0);
    contentsLayout()->addWidget(d->yandexMoney, 8, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 9, 0);

    connect(d->renewButton, &Button::clicked, this, &RenewSubscriptionDialog::renewPressed);
    connect(d->cancelButton, &Button::clicked, this, &RenewSubscriptionDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

int RenewSubscriptionDialog::monthCount() const
{
    if (d->renew1Month->isChecked()) {
        return 1;
    } else if (d->renew2Month->isChecked()) {
        return 2;
    } else if (d->renew3Month->isChecked()) {
        return 3;
    } else if (d->renew6Month->isChecked()) {
        return 6;
    } else {
        return 12;
    }
}

int RenewSubscriptionDialog::paymentType() const
{
    if (d->paypal->isChecked()) {
        return 0;
    } else if (d->bankCard->isChecked()) {
        return 1;
    } else {
        return 2;
    }
}

RenewSubscriptionDialog::~RenewSubscriptionDialog() = default;

QWidget* RenewSubscriptionDialog::focusedWidgetAfterShow() const
{
    return d->renew1Month;
}

void RenewSubscriptionDialog::updateTranslations()
{
    setTitle(tr("Renew cloud service subscription for"));
//    d->renew1Month->setText(tr("1 month by $3.99"));
//    d->renew2Month->setText(tr("2 month by $7.99"));
//    d->renew3Month->setText(tr("3 month by $11.99"));
//    d->renew6Month->setText(tr("6 month by $21.99"));
//    d->renew12Month->setText(tr("12 month by $41.99"));
    d->paypal->setText(tr("via PayPal"));
    d->bankCard->setText(tr("via bank card"));
    d->yandexMoney->setText(tr("via Yandex.Money"));
    d->renewButton->setText(tr("Renew"));
    d->cancelButton->setText(tr("Cancel"));
}

void RenewSubscriptionDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto radioButton : { d->renew1Month, d->renew2Month, d->renew3Month, d->renew6Month, d->renew12Month,
                              d->paypal, d->bankCard, d->yandexMoney }) {
        radioButton->setBackgroundColor(Ui::DesignSystem::color().background());
        radioButton->setTextColor(Ui::DesignSystem::color().onBackground());
    }

#ifndef Q_OS_MAC
    contentsLayout()->setRowMinimumHeight(5, static_cast<int>(Ui::DesignSystem::layout().px16()));
#endif

    for (auto button : { d->renewButton, d->cancelButton }) {
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

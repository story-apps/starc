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
    RadioButton* yandexMoney = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* renewButton = nullptr;
    Button* restoreButton = nullptr;
    Button* cancelButton = nullptr;
};

RenewSubscriptionDialog::Implementation::Implementation(QWidget* _parent)
    : renew1Month(new RadioButton(_parent)),
      renew2Month(new RadioButton(_parent)),
      renew3Month(new RadioButton(_parent)),
      renew6Month(new RadioButton(_parent)),
      renew12Month(new RadioButton(_parent)),
      paypal(new RadioButton(_parent)),
      yandexMoney(new RadioButton(_parent)),
      renewButton(new Button(_parent)),
      restoreButton(new Button(_parent)),
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
    paymentGroup->add(yandexMoney);

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(renewButton);
    buttonsLayout->addWidget(restoreButton);
    buttonsLayout->addWidget(cancelButton);

#ifndef Q_OS_MAC
    renewButton->hide();
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
    contentsLayout()->setRowMinimumHeight(5, 1); // отступ между месяцами и способами оплаты
    contentsLayout()->addWidget(d->paypal, 6, 0);
    contentsLayout()->addWidget(d->yandexMoney, 7, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 8, 0);

    connect(d->cancelButton, &Button::clicked, this, &RenewSubscriptionDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

RenewSubscriptionDialog::~RenewSubscriptionDialog() = default;

QWidget* RenewSubscriptionDialog::focusedWidgetAfterShow() const
{
    return d->renew1Month;
}

void RenewSubscriptionDialog::updateTranslations()
{
    setTitle(tr("Renew cloud service subscription for"));
    d->renew1Month->setText(tr("1 month by 5$"));
    d->renew2Month->setText(tr("2 month by 10$"));
    d->renew3Month->setText(tr("3 month by 15$"));
    d->renew6Month->setText(tr("6 month by 30$ (discount 6%)"));
    d->renew12Month->setText(tr("12 month by 60$ (discount 12%)"));
    d->paypal->setText(tr("via PayPal"));
    d->yandexMoney->setText(tr("via Yandex.Money"));
    d->renewButton->setText(tr("Renew"));
    d->restoreButton->setText(tr("Restore"));
    d->cancelButton->setText(tr("Cancel"));
}

void RenewSubscriptionDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    for (auto radioButton : { d->renew1Month, d->renew2Month, d->renew3Month, d->renew6Month, d->renew12Month,
                              d->paypal, d->yandexMoney }) {
        radioButton->setBackgroundColor(Ui::DesignSystem::color().surface());
        radioButton->setTextColor(Ui::DesignSystem::color().onSurface());
    }

    contentsLayout()->setRowMinimumHeight(5, static_cast<int>(Ui::DesignSystem::layout().px16()));

    for (auto button : { d->renewButton, d->restoreButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }
}

} // namespace Ui

#include "purchase_gift_dialog.h"

#include "purchase_dialog_option_widget.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/validators/email_validator.h>

#include <QBoxLayout>


namespace Ui {

class PurchaseGiftDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Widget* content = nullptr;
    PurchaseDialogOptionWidget* paymentOption = nullptr;
    TextField* email = nullptr;
    TextField* greeting = nullptr;
    Body1Label* information = nullptr;
    IconsMidLabel* informationIcon = nullptr;
    Body2Label* informationText = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* cancelButton = nullptr;
    Button* purchaseButton = nullptr;
};

PurchaseGiftDialog::Implementation::Implementation(QWidget* _parent)
    : content(new Widget(_parent))
    , paymentOption(new PurchaseDialogOptionWidget(content))
    , email(new TextField(content))
    , greeting(new TextField(content))
    , information(new Body1Label(content))
    , informationIcon(new IconsMidLabel(content))
    , informationText(new Body2Label(content))
    , buttonsLayout(new QHBoxLayout)
    , cancelButton(new Button(_parent))
    , purchaseButton(new Button(_parent))
{
    paymentOption->setEnabled(false);
    paymentOption->setWide(true);
    paymentOption->setShowTotal(true);
    email->setCapitalizeWords(false);
    informationIcon->setIcon(u8"\U000F02FD");

    auto informationLayout = new QHBoxLayout;
    informationLayout->setContentsMargins({});
    informationLayout->setSpacing(0);
    informationLayout->addWidget(informationIcon, 0, Qt::AlignTop);
    informationLayout->addWidget(informationText, 1);
    information->setLayout(informationLayout);

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(paymentOption);
    layout->addWidget(email);
    layout->addWidget(greeting);
    layout->addWidget(information);
    content->setLayout(layout);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(purchaseButton);
}


// ****


PurchaseGiftDialog::PurchaseGiftDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->purchaseButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->content, row++, 0);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->email, &TextField::textChanged, d->email, &TextField::clearError);
    connect(d->cancelButton, &Button::clicked, this, &PurchaseGiftDialog::canceled);
    connect(d->purchaseButton, &Button::clicked, this, [this] {
        if (!EmailValidator::isValid(d->email->text())) {
            d->email->setError(tr("Email invalid"));
            d->email->setFocus();
            return;
        }

        emit purchasePressed(d->paymentOption->paymentOption(), d->email->text(),
                             d->greeting->text());
    });
}

PurchaseGiftDialog::~PurchaseGiftDialog() = default;

void PurchaseGiftDialog::setPaymentOption(const Domain::PaymentOption& _paymentOption)
{
    d->paymentOption->setPaymentOption(_paymentOption);
}

QWidget* PurchaseGiftDialog::focusedWidgetAfterShow() const
{
    return d->email;
}

QWidget* PurchaseGiftDialog::lastFocusableWidget() const
{
    return d->purchaseButton;
}

void PurchaseGiftDialog::updateTranslations()
{
    setTitle(tr("Purchasing a gift"));
    d->email->setLabel(tr("Recepient email"));
    d->email->setHelper(tr("A promo code to activate the gift will be sent here"));
    d->greeting->setLabel(tr("Greeting text"));
    d->greeting->setHelper(tr("You can leave it blank"));
    d->informationText->setText(tr("A promo code to activate the gift will be sent to the "
                                   "recipient immediately after payment.\n\n"
                                   "Promo code validity period is a 1 year"));
    d->cancelButton->setText(tr("Cancel"));
    d->purchaseButton->setText(tr("Purchase"));
}

void PurchaseGiftDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentFixedWidth(DesignSystem::dialog().maximumWidth());

    d->content->setBackgroundColor(DesignSystem::color().background());

    d->paymentOption->setBackgroundColor(DesignSystem::color().background());
    d->paymentOption->setTextColor(DesignSystem::color().onBackground());
    d->paymentOption->setContentsMargins(DesignSystem::layout().px(), DesignSystem::layout().px(),
                                         DesignSystem::layout().px(), DesignSystem::layout().px());

    for (auto textField : {
             d->email,
             d->greeting,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
        textField->setCustomMargins({ 0, DesignSystem::compactLayout().px16(), 0, 0 });
    }

    d->information->setBackgroundColor(ColorHelper::transparent(
        DesignSystem::color().accent(), DesignSystem::focusBackgroundOpacity()));
    d->information->setContentsMargins(0, DesignSystem::layout().px24(), 0, 0);
    d->information->setBorderRadius(DesignSystem::layout().px4());
    for (auto label : std::vector<AbstractLabel*>{
             d->informationIcon,
             d->informationText,
         }) {
        label->setBackgroundColor(Qt::transparent);
        label->setTextColor(DesignSystem::color().onBackground());
    }
    d->informationIcon->setContentsMargins(DesignSystem::layout().px12(),
                                           DesignSystem::layout().px16(),
                                           DesignSystem::layout().px12(), 0);
    d->informationText->setContentsMargins(0, DesignSystem::layout().px16(),
                                           DesignSystem::layout().px12(),
                                           DesignSystem::layout().px16());

    for (auto button : {
             d->cancelButton,
             d->purchaseButton,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    d->content->layout()->setContentsMargins(DesignSystem::layout().px24(), 0,
                                             DesignSystem::layout().px24(), 0);
    contentsLayout()->setSpacing(static_cast<int>(DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui

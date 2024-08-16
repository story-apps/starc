#include "purchase_dialog.h"

#include "purchase_dialog_option_widget.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>

#include <QApplication>
#include <QHBoxLayout>


namespace Ui {

class PurchaseDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить состояние опций оплат
     */
    void updateOptionsState(PurchaseDialogOptionWidget* _checkedOption);


    Body1Label* descriptionLabel = nullptr;
    Widget* content = nullptr;
    QVector<PurchaseDialogOptionWidget*> options;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* giftButton = nullptr;
    Button* cancelButton = nullptr;
    Button* purchaseButton = nullptr;
};

PurchaseDialog::Implementation::Implementation(QWidget* _parent)
    : descriptionLabel(new Body1Label(_parent))
    , content(new Widget(_parent))
    , buttonsLayout(new QHBoxLayout)
    , giftButton(new Button(_parent))
    , cancelButton(new Button(_parent))
    , purchaseButton(new Button(_parent))
{
    descriptionLabel->hide();

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addWidget(giftButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(purchaseButton);
}

void PurchaseDialog::Implementation::updateOptionsState(PurchaseDialogOptionWidget* _checkedOption)
{
    for (auto option : std::as_const(options)) {
        if (option == _checkedOption) {
            continue;
        }

        option->setChecked(false);
    }
}


// ****


PurchaseDialog::PurchaseDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->purchaseButton);
    setRejectButton(d->cancelButton);

    contentsLayout()->addWidget(d->descriptionLabel, 0, 0);
    contentsLayout()->addWidget(d->content, 1, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 2, 0);

    connect(d->giftButton, &Button::clicked, this, [this] {
        for (auto option : std::as_const(d->options)) {
            if (option->isChecked()) {
                emit giftPressed(option->paymentOption());
                return;
            }
        }
    });
    connect(d->cancelButton, &Button::clicked, this, &PurchaseDialog::canceled);
    connect(d->purchaseButton, &Button::clicked, this, [this] {
        for (auto option : std::as_const(d->options)) {
            if (option->isChecked()) {
                emit purchasePressed(option->paymentOption());
                return;
            }
        }
    });
}

PurchaseDialog::~PurchaseDialog() = default;

void PurchaseDialog::setDescription(const QString& _description)
{
    d->descriptionLabel->setText(_description);
    d->descriptionLabel->setVisible(!_description.isEmpty());
}

void PurchaseDialog::setPaymentOptions(const QVector<Domain::PaymentOption>& _paymentOptions)
{
    QGridLayout* contentLayout = nullptr;
    if (d->content->layout() == nullptr) {
        contentLayout = new QGridLayout(d->content);
    } else {
        contentLayout = qobject_cast<QGridLayout*>(d->content->layout());
        while (!contentLayout->isEmpty()) {
            auto item = contentLayout->takeAt(0);
            if (item->widget() != nullptr) {
                item->widget()->deleteLater();
            }
            delete item;
        }
    }
    d->options.clear();

    int row = 0;
    int column = 0;
    for (const auto& option : _paymentOptions) {
        auto optionWidget = new PurchaseDialogOptionWidget(this);
        optionWidget->setPaymentOption(option);
        optionWidget->setBackgroundColor(DesignSystem::color().background());
        optionWidget->setTextColor(DesignSystem::color().onBackground());
        if (option.duration == Domain::PaymentDuration::Lifetime) {
            optionWidget->setContentsMargins(
                DesignSystem::layout().px12(), DesignSystem::layout().px4(),
                DesignSystem::layout().px12(), DesignSystem::layout().px12());
            contentLayout->addWidget(optionWidget, row++, 0, 1, 2);
            column = 0;
        } else {
            if (column == 2) {
                ++row;
                column = 0;
            }
            optionWidget->setContentsMargins(
                column == 0 ? DesignSystem::layout().px12() : DesignSystem::layout().px4(),
                DesignSystem::layout().px4(),
                column == 0 ? DesignSystem::layout().px4() : DesignSystem::layout().px12(),
                DesignSystem::layout().px4());
            contentLayout->addWidget(optionWidget, row, column++);
        }

        connect(optionWidget, &PurchaseDialogOptionWidget::checkedChanged, this,
                [this, optionWidget] { d->updateOptionsState(optionWidget); });

        d->options.append(optionWidget);
    }
}

void PurchaseDialog::selectOption(const Domain::PaymentOption& _option)
{
    for (auto option : std::as_const(d->options)) {
        if (option->paymentOption().subscriptionType == _option.subscriptionType) {
            option->setChecked(true);
            break;
        }
    }
}

QWidget* PurchaseDialog::focusedWidgetAfterShow() const
{
    return d->content;
}

QWidget* PurchaseDialog::lastFocusableWidget() const
{
    return d->purchaseButton;
}

void PurchaseDialog::updateTranslations()
{
    setTitle(tr("Choose what suits you"));
    d->giftButton->setText(tr("Buy as a gift"));
    d->cancelButton->setText(tr("Cancel"));
    d->purchaseButton->setText(tr("Purchase"));
}

void PurchaseDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentFixedWidth(DesignSystem::dialog().maximumWidth());

    d->content->setBackgroundColor(DesignSystem::color().background());

    d->descriptionLabel->setBackgroundColor(DesignSystem::color().background());
    d->descriptionLabel->setTextColor(DesignSystem::color().onBackground());
    d->descriptionLabel->setContentsMargins(DesignSystem::layout().px24(), 0,
                                            DesignSystem::layout().px24(), 0);

    for (auto button : {
             d->giftButton,
             d->cancelButton,
             d->purchaseButton,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    contentsLayout()->setSpacing(static_cast<int>(DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui

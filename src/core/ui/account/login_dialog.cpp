#include "login_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/tools/debouncer.h>
#include <utils/validators/email_validator.h>

#include <QEvent>
#include <QGridLayout>

namespace Ui {

class LoginDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void checkEmail();


    Body1Label* description = nullptr;
    TextField* email = nullptr;
    Body1Label* confirmationCodeSendedInfo = nullptr;
    TextField* confirmationCode = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* signInButton = nullptr;
    Button* resendCodeButton = nullptr;
    Button* cancelButton = nullptr;
};

LoginDialog::Implementation::Implementation(QWidget* _parent)
    : description(new Body1Label(_parent))
    , email(new TextField(_parent))
    , confirmationCodeSendedInfo(new Body1Label(_parent))
    , confirmationCode(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , signInButton(new Button(_parent))
    , resendCodeButton(new Button(_parent))
    , cancelButton(new Button(_parent))
{
    email->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    email->setTabChangesFocus(true);

    confirmationCode->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    confirmationCode->setTabChangesFocus(true);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(signInButton);
    buttonsLayout->addWidget(resendCodeButton);
    buttonsLayout->addWidget(cancelButton);

    confirmationCodeSendedInfo->hide();
    confirmationCode->hide();
    signInButton->hide();
    resendCodeButton->hide();
}


// ****


LoginDialog::LoginDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    setAcceptButton(d->signInButton);
    setRejectButton(d->cancelButton);

    int row = 0;
    contentsLayout()->addWidget(d->description, row++, 0);
    contentsLayout()->addWidget(d->email, row++, 0);
    contentsLayout()->addWidget(d->confirmationCodeSendedInfo, row++, 0);
    contentsLayout()->addWidget(d->confirmationCode, row++, 0);
    contentsLayout()->setRowStretch(row++, 1);
    contentsLayout()->addLayout(d->buttonsLayout, row++, 0);

    connect(d->email, &TextField::textChanged, this, [this] {
        d->email->setError({});
        d->signInButton->setVisible(EmailValidator::isValid(d->email->text()));
        showEmailStep();
    });
    connect(d->confirmationCode, &TextField::textChanged, this, [this] {
        const auto code = d->confirmationCode->text();
        if (code.isEmpty()) {
            return;
        }

        emit confirmationCodeChanged(code);
    });
    connect(d->signInButton, &Button::clicked, this, [this] {
        d->confirmationCodeSendedInfo->hide();
        d->confirmationCode->hide();

        if (!EmailValidator::isValid(d->email->text())) {
            d->email->setError(tr("Email invalid"));
            return;
        }

        showConfirmationCodeStep();

        emit signInPressed();
    });
    connect(d->cancelButton, &Button::clicked, this, &LoginDialog::cancelPressed);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

LoginDialog::~LoginDialog() = default;

void LoginDialog::showEmailStep()
{
    d->confirmationCode->clear();
    d->confirmationCodeSendedInfo->hide();
    d->confirmationCode->hide();

    d->email->setFocus();
}

QString LoginDialog::email() const
{
    return d->email->text();
}

void LoginDialog::showConfirmationCodeStep()
{
    d->confirmationCodeSendedInfo->show();
    d->confirmationCode->clear();
    d->confirmationCode->setError({});
    d->confirmationCode->show();
    d->signInButton->hide();

    d->confirmationCode->setFocus();
}

QString LoginDialog::confirmationCode() const
{
    return d->confirmationCode->text();
}

void LoginDialog::showConfirmationCodeError()
{
    d->confirmationCode->setError(tr("Incorrect confirmation code"));
    d->confirmationCode->setFocus();
}

QWidget* LoginDialog::focusedWidgetAfterShow() const
{
    return d->email;
}

QWidget* LoginDialog::lastFocusableWidget() const
{
    return d->cancelButton;
}

void LoginDialog::updateTranslations()
{
    setTitle(tr("Get into your account"));

    d->description->setText(tr("Sign in to get access to the extended free and pro features"));
    d->email->setLabel(tr("Email"));
    d->confirmationCodeSendedInfo->setText(
        tr("We've sent a confirmation code to the e-mail above, please enter it here to verify"));
    d->confirmationCode->setLabel(tr("Confirmation code"));
    d->signInButton->setText(tr("Sign in"));
    d->resendCodeButton->setText(tr("Send code again"));
    d->cancelButton->setText(tr("Cancel"));
}

void LoginDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentFixedWidth(Ui::DesignSystem::dialog().minimumWidth());

    for (auto label : { d->description, d->confirmationCodeSendedInfo }) {
        label->setBackgroundColor(Ui::DesignSystem::color().background());
        label->setTextColor(Ui::DesignSystem::color().onBackground());
    }
    auto labelContentMargins = Ui::DesignSystem::label().margins().toMargins();
    labelContentMargins.setTop(0);
    labelContentMargins.setBottom(Ui::DesignSystem::layout().px12());
    d->description->setContentsMargins(labelContentMargins);
    labelContentMargins.setTop(Ui::DesignSystem::layout().px12());
    d->confirmationCodeSendedInfo->setContentsMargins(labelContentMargins);

    for (auto textField : { d->email, d->confirmationCode }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    for (auto button : { d->signInButton, d->resendCodeButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui

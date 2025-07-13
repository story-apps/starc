#include "login_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/ui_helper.h>
#include <utils/tools/debouncer.h>
#include <utils/validators/email_validator.h>

#include <QEvent>
#include <QGridLayout>

namespace Ui {

class LoginDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Установить текст лейбла с информацией о коде подтверждения
     *        в зависимости от того истёк ли срок годности кода
     */
    void updateConfirmationCodeInfo(bool _expired = false);


    Body1Label* description = nullptr;
    TextField* email = nullptr;
    Body1Label* authorizationError = nullptr;
    Body1Label* confirmationCodeSendedInfo = nullptr;
    TextField* confirmationCode = nullptr;
    QTimer confirmationCodeExpirationTimer;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* signInButton = nullptr;
    Button* resendCodeButton = nullptr;
    Button* cancelButton = nullptr;
};

LoginDialog::Implementation::Implementation(QWidget* _parent)
    : description(new Body1Label(_parent))
    , email(new TextField(_parent))
    , authorizationError(new Body1Label(_parent))
    , confirmationCodeSendedInfo(new Body1Label(_parent))
    , confirmationCode(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , signInButton(new Button(_parent))
    , resendCodeButton(new Button(_parent))
    , cancelButton(new Button(_parent))
{
    email->setCapitalizeWords(false);
    email->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    email->setTabChangesFocus(true);

    confirmationCode->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    confirmationCode->setTabChangesFocus(true);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(resendCodeButton);
    buttonsLayout->addWidget(signInButton);

    authorizationError->hide();
    confirmationCodeSendedInfo->hide();
    confirmationCode->hide();
    signInButton->hide();
    resendCodeButton->hide();

    confirmationCodeExpirationTimer.setSingleShot(true);
    confirmationCodeExpirationTimer.setInterval(std::chrono::minutes{ 10 });
}

void LoginDialog::Implementation::updateConfirmationCodeInfo(bool _expired)
{
    if (_expired) {
        confirmationCodeSendedInfo->setText(tr("The confirmation code we've sent expired."));
    } else {
        confirmationCodeSendedInfo->setText(tr(
            "We've sent a confirmation code to the e-mail above, please enter it here to verify"));
    }
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
    contentsLayout()->addWidget(d->authorizationError, row++, 0);
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
    connect(&d->confirmationCodeExpirationTimer, &QTimer::timeout, this, [this] {
        const bool expired = true;
        d->updateConfirmationCodeInfo(expired);
        d->resendCodeButton->show();
        d->resendCodeButton->setFocus();
        d->confirmationCode->hide();
    });
    connect(d->signInButton, &Button::clicked, this, [this] {
        d->authorizationError->hide();
        d->confirmationCodeSendedInfo->hide();
        d->confirmationCode->hide();

        if (!EmailValidator::isValid(d->email->text())) {
            d->email->setError(tr("Email invalid"));
            return;
        }

        emit signInPressed();
    });
    connect(d->resendCodeButton, &Button::clicked, this, [this] {
        d->updateConfirmationCodeInfo();

        emit signInPressed();
    });
    connect(d->cancelButton, &Button::clicked, this, &LoginDialog::cancelPressed);
}

LoginDialog::~LoginDialog() = default;

void LoginDialog::showEmailStep()
{
    d->confirmationCode->clear();
    d->authorizationError->hide();
    d->confirmationCodeSendedInfo->hide();
    d->confirmationCode->hide();
    d->resendCodeButton->hide();

    if (isVisible()) {
        d->email->setFocus();
    }
}

QString LoginDialog::email() const
{
    return d->email->text();
}

void LoginDialog::setAuthorizationError(const QString& _error)
{
    d->authorizationError->setText(_error);
    d->authorizationError->show();
}

void LoginDialog::showConfirmationCodeStep()
{
    d->authorizationError->hide();
    d->confirmationCodeSendedInfo->show();
    d->confirmationCode->clear();
    d->confirmationCode->setError({});
    d->confirmationCode->show();
    d->signInButton->hide();
    d->resendCodeButton->hide();

    d->confirmationCode->setFocus();

    d->confirmationCodeExpirationTimer.start();
}

QString LoginDialog::confirmationCode() const
{
    return d->confirmationCode->text();
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

    d->description->setText(
        tr("Sign in to get access to extended features of the FREE, PRO and CLOUD versions"));
    d->email->setLabel(tr("Email"));
    d->updateConfirmationCodeInfo();
    d->confirmationCode->setLabel(tr("Confirmation code"));
    d->signInButton->setText(tr("Sign in"));
    d->resendCodeButton->setText(tr("Send code again"));
    d->cancelButton->setText(tr("Cancel"));
}

void LoginDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    setContentFixedWidth(DesignSystem::dialog().minimumWidth());

    for (auto label : {
             d->description,
             d->authorizationError,
             d->confirmationCodeSendedInfo,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
    }
    d->authorizationError->setTextColor(DesignSystem::color().error());
    auto labelContentMargins = DesignSystem::label().margins().toMargins();
    labelContentMargins.setTop(0);
    labelContentMargins.setBottom(DesignSystem::layout().px12());
    d->description->setContentsMargins(labelContentMargins);
    labelContentMargins.setTop(DesignSystem::layout().px12());
    d->authorizationError->setContentsMargins(labelContentMargins);
    d->confirmationCodeSendedInfo->setContentsMargins(labelContentMargins);

    for (auto textField : { d->email, d->confirmationCode }) {
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setTextColor(DesignSystem::color().onBackground());
    }

    for (auto button : { d->signInButton, d->resendCodeButton, d->cancelButton }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }
    UiHelper::initColorsFor(d->cancelButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->resendCodeButton, UiHelper::DialogDefault);
    UiHelper::initColorsFor(d->signInButton, UiHelper::DialogAccept);

    contentsLayout()->setSpacing(static_cast<int>(DesignSystem::compactLayout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(DesignSystem::layout().px12(), DesignSystem::layout().px12(),
                  DesignSystem::layout().px16(), DesignSystem::layout().px16())
            .toMargins());
}

} // namespace Ui

#include "login_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
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


    Debouncer checkEmailDebouncer{ 500 };
    Debouncer notifyEmailDebouncer{ 300 };

    TextField* email = nullptr;
    TextField* password = nullptr;
    TextField* registrationConfirmationCode = nullptr;
    TextField* restorePasswordConfirmationCode = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* registrationButton = nullptr;
    Button* restorePasswordButton = nullptr;
    Button* changePasswordButton = nullptr;
    Button* loginButton = nullptr;
    Button* cancelButton = nullptr;
};

LoginDialog::Implementation::Implementation(QWidget* _parent)
    : email(new TextField(_parent))
    , password(new TextField(_parent))
    , registrationConfirmationCode(new TextField(_parent))
    , restorePasswordConfirmationCode(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , registrationButton(new Button(_parent))
    , restorePasswordButton(new Button(_parent))
    , changePasswordButton(new Button(_parent))
    , loginButton(new Button(_parent))
    , cancelButton(new Button(_parent))
{
    email->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    email->setTabChangesFocus(true);

    password->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    password->setTabChangesFocus(true);
    password->setPasswordModeEnabled(true);
    password->setTrailingIcon(u8"\U000f06d1");

    registrationConfirmationCode->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    registrationConfirmationCode->setTabChangesFocus(true);
    restorePasswordConfirmationCode->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    restorePasswordConfirmationCode->setTabChangesFocus(true);

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(registrationButton);
    buttonsLayout->addWidget(restorePasswordButton);
    buttonsLayout->addWidget(changePasswordButton);
    buttonsLayout->addWidget(loginButton);
    buttonsLayout->addWidget(cancelButton);

    registrationConfirmationCode->hide();
    restorePasswordConfirmationCode->hide();
    restorePasswordButton->hide();
    changePasswordButton->hide();
    registrationButton->hide();
    loginButton->hide();
}

void LoginDialog::Implementation::checkEmail()
{
    auto resetState = [this] {
        notifyEmailDebouncer.abortWork();
        registrationButton->hide();
        loginButton->hide();
    };

    if (email->text().isEmpty()) {
        email->setError({});
        resetState();
        return;
    }

    if (!EmailValidator::isValid(email->text())) {
        email->setError(tr("Email invalid"));
        resetState();
        return;
    }

    email->setError({});
    notifyEmailDebouncer.orderWork();
}


// ****


LoginDialog::LoginDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    d->cancelButton->installEventFilter(this);

    contentsLayout()->addWidget(d->email, 0, 0);
    contentsLayout()->addWidget(d->restorePasswordConfirmationCode, 1, 0);
    contentsLayout()->addWidget(d->password, 2, 0);
    contentsLayout()->addWidget(d->registrationConfirmationCode, 3, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 4, 0);

    connect(d->email, &TextField::textChanged, &d->checkEmailDebouncer, &Debouncer::orderWork);
    connect(&d->checkEmailDebouncer, &Debouncer::gotWork, this, [this] { d->checkEmail(); });
    connect(&d->notifyEmailDebouncer, &Debouncer::gotWork, this, &LoginDialog::emailEntered);
    connect(d->password, &TextField::trailingIconPressed, d->password, [password = d->password] {
        password->setPasswordModeEnabled(!password->isPasswordModeEnabled());
        password->setTrailingIcon(password->isPasswordModeEnabled() ? u8"\U000f06d1"
                                                                    : u8"\U000f06d0");
    });
    connect(d->registrationConfirmationCode, &TextField::textChanged, this,
            &LoginDialog::registrationConfirmationCodeEntered);
    connect(d->restorePasswordConfirmationCode, &TextField::textChanged, this,
            &LoginDialog::passwordRestoringConfirmationCodeEntered);
    connect(d->restorePasswordButton, &Button::clicked, this,
            &LoginDialog::restorePasswordRequested);
    connect(d->changePasswordButton, &Button::clicked, this, &LoginDialog::changePasswordRequested);
    connect(d->registrationButton, &Button::clicked, this, &LoginDialog::registrationRequested);
    connect(d->loginButton, &Button::clicked, this, &LoginDialog::loginRequested);
    connect(d->cancelButton, &Button::clicked, this, &LoginDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

QString LoginDialog::email() const
{
    return d->email->text();
}

QString LoginDialog::password() const
{
    return d->password->text();
}

QString LoginDialog::registractionConfirmationCode() const
{
    return d->registrationConfirmationCode->text();
}

QString LoginDialog::restorePasswordConfirmationCode() const
{
    return d->restorePasswordConfirmationCode->text();
}

void LoginDialog::showRegistrationButton()
{
    d->restorePasswordButton->hide();
    d->loginButton->hide();
    d->registrationButton->show();
}

void LoginDialog::showRegistrationConfirmationCodeField()
{
    d->registrationConfirmationCode->show();
    d->registrationConfirmationCode->setFocus();

    d->email->setEnabled(false);
    d->password->setEnabled(false);
    d->registrationButton->setEnabled(false);
}

void LoginDialog::setRegistrationConfirmationError(const QString& _error)
{
    d->registrationConfirmationCode->setError(_error);
    d->registrationConfirmationCode->setFocus();
}

void LoginDialog::showLoginButtons()
{
    d->registrationButton->hide();
    d->restorePasswordButton->show();
    d->loginButton->show();
}

void LoginDialog::showRestorePasswordConfirmationCodeField()
{
    d->password->hide();
    d->restorePasswordConfirmationCode->show();
    d->restorePasswordConfirmationCode->setFocus();
    d->restorePasswordButton->hide();
    d->loginButton->hide();

    d->email->setEnabled(false);
}

void LoginDialog::setRestorePasswordConfirmationError(const QString& _error)
{
    d->restorePasswordConfirmationCode->setError(_error);
    d->restorePasswordConfirmationCode->setFocus();
}

void LoginDialog::showChangePasswordFieldAndButton()
{
    d->password->setLabel(tr("New password"));
    d->password->show();
    d->password->setFocus();
    d->changePasswordButton->show();

    d->restorePasswordConfirmationCode->setEnabled(false);
}

void LoginDialog::setPasswordError(const QString& _error)
{
    d->password->setError(_error);
    d->password->setFocus();
}

LoginDialog::~LoginDialog() = default;

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

    d->email->setLabel(tr("Email"));
    d->password->setLabel(tr("Password"));
    d->registrationConfirmationCode->setLabel(tr("Confirmation code"));
    d->restorePasswordConfirmationCode->setLabel(tr("Confirmation code"));
    d->registrationButton->setText(tr("Sign up"));
    d->restorePasswordButton->setText(tr("Restore password"));
    d->changePasswordButton->setText(tr("Change password"));
    d->loginButton->setText(tr("Sign in"));
    d->cancelButton->setText(tr("Cancel"));
}

void LoginDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto button : { d->registrationButton, d->restorePasswordButton, d->changePasswordButton,
                         d->loginButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px8())
            .toMargins());
}

bool LoginDialog::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Зацикливаем фокус, чтобы он всегда оставался внутри диалога
    //
    if (_watched == d->cancelButton && _event->type() == QEvent::FocusOut) {
        d->email->setFocus();
    }

    return AbstractDialog::eventFilter(_watched, _event);
}

} // namespace Ui

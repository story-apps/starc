#include "login_dialog.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/button/button.h>
#include <ui/widgets/text_field/text_field.h>

#include <utils/tools/debouncer.h>

#include <utils/validators/email_validator.h>

#include <QEvent>
#include <QGridLayout>
#include <QTimer>

namespace Ui
{

class LoginDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    void checkEmail();


    Debouncer checkEmailDebouncer{500};
    Debouncer notifyEmailDebouncer{300};

    TextField* email = nullptr;
    TextField* password = nullptr;
    TextField* confirmationCode = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* registrationButton = nullptr;
    Button* loginButton = nullptr;
    Button* cancelButton = nullptr;
};

LoginDialog::Implementation::Implementation(QWidget* _parent)
    : email(new TextField(_parent)),
      password(new TextField(_parent)),
      confirmationCode(new TextField(_parent)),
      registrationButton(new Button(_parent)),
      loginButton(new Button(_parent)),
      cancelButton(new Button(_parent))
{
    email->setTabChangesFocus(true);

    password->setTabChangesFocus(true);
    password->setPasswordModeEnabled(true);
    password->setTrailingIcon("\uf6d0");

    confirmationCode->setTabChangesFocus(true);

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(registrationButton);
    buttonsLayout->addWidget(loginButton);
    buttonsLayout->addWidget(cancelButton);

    confirmationCode->hide();
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
    : AbstractDialog(_parent),
      d(new Implementation(this))
{
    d->cancelButton->installEventFilter(this);

    contentsLayout()->addWidget(d->email, 0, 0);
    contentsLayout()->addWidget(d->password, 1, 0);
    contentsLayout()->addWidget(d->confirmationCode, 2, 0);
    contentsLayout()->setRowStretch(3, 1);
    contentsLayout()->addLayout(d->buttonsLayout, 4, 0);

    connect(d->email, &TextField::textChanged, &d->checkEmailDebouncer, &Debouncer::orderWork);
    connect(&d->checkEmailDebouncer, &Debouncer::gotWork, this, [this] { d->checkEmail(); });
    connect(&d->notifyEmailDebouncer, &Debouncer::gotWork, this, [this] {
        const QString email = d->email->text();
        emit emailEntered(email);
    });
    connect(d->password, &TextField::trailingIconPressed, d->password, [password = d->password] {
        password->setPasswordModeEnabled(!password->isPasswordModeEnabled());
        password->setTrailingIcon(password->isPasswordModeEnabled() ? "\uf6d0" : "\uf6cf");
    });
    connect(d->confirmationCode, &TextField::textChanged, this, [this] {
        emit confirmationCodeEntered(d->email->text(), d->confirmationCode->text());
    });
    connect(d->registrationButton, &Button::clicked, this, [this] {
        emit registrationRequired(d->email->text(), d->password->text());
    });
    connect(d->loginButton, &Button::clicked, this, [this] {
        emit loginRequired(d->email->text(), d->password->text());
    });
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

void LoginDialog::showRegistrationButton()
{
    d->loginButton->hide();
    d->registrationButton->show();
}

void LoginDialog::showConfirmationCodeField()
{
    d->confirmationCode->show();
    d->confirmationCode->setFocus();

    d->email->setEnabled(false);
    d->password->setEnabled(false);
    d->registrationButton->setEnabled(false);
}

void LoginDialog::setConfirmationError(const QString& _error)
{
    d->confirmationCode->setError(_error);
    d->confirmationCode->setFocus();
}

void LoginDialog::showLoginButton()
{
    d->registrationButton->hide();
    d->loginButton->show();
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

void LoginDialog::updateTranslations()
{
    setTitle(tr("Get into your account"));

    d->email->setLabel(tr("Email"));
    d->password->setLabel(tr("Password"));
    d->confirmationCode->setLabel(tr("Confirmation code"));
    d->registrationButton->setText(tr("Sign up"));
    d->loginButton->setText(tr("Sign in"));
    d->cancelButton->setText(tr("Cancel"));
}

void LoginDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

    d->registrationButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->registrationButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->loginButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->loginButton->setTextColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setBackgroundColor(Ui::DesignSystem::color().secondary());
    d->cancelButton->setTextColor(Ui::DesignSystem::color().secondary());

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(QMarginsF(Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px12(),
                                                   Ui::DesignSystem::layout().px16(),
                                                   Ui::DesignSystem::layout().px8()).toMargins());
}

bool LoginDialog::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Зацикливаем фокус, чтобы он всегда оставался внутри диалога
    //
    if (_watched == d->cancelButton
        && _event->type() == QEvent::FocusOut) {
        d->email->setFocus();
    }

    return AbstractDialog::eventFilter(_watched, _event);
}

}

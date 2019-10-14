#include "login_dialog.h"

#include <ui/design_system/design_system.h>

#include <ui/widgets/button/button.h>
#include <ui/widgets/text_field/text_field.h>

#include <QEvent>
#include <QGridLayout>
#include <QTimer>

namespace Ui
{

class LoginDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    TextField* email = nullptr;
    TextField* password = nullptr;
    TextField* confirmationCode = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* loginButton = nullptr;
    Button* cancelButton = nullptr;
};

LoginDialog::Implementation::Implementation(QWidget* _parent)
    : email(new TextField(_parent)),
      password(new TextField(_parent)),
      confirmationCode(new TextField(_parent)),
      loginButton(new Button(_parent)),
      cancelButton(new Button(_parent))
{
    email->setTabChangesFocus(true);

    password->setTabChangesFocus(true);
    password->setPasswordModeEnabled(true);
    password->setTrailingIcon("\uf6d0");

    buttonsLayout = new QHBoxLayout;
    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(cancelButton);
    buttonsLayout->addWidget(loginButton);

    confirmationCode->hide();
    loginButton->hide();
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

    connect(d->password, &TextField::trailingIconPressed, d->password, [password = d->password] {
        password->setPasswordModeEnabled(!password->isPasswordModeEnabled());
        password->setTrailingIcon(password->isPasswordModeEnabled() ? "\uf6d0" : "\uf6cf");
    });
    connect(d->cancelButton, &Button::clicked, this, &LoginDialog::hideDialog);

    updateTranslations();
    designSystemChangeEvent(nullptr);
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
    d->loginButton->setText(tr("Login"));
    d->cancelButton->setText(tr("Cancel"));
}

void LoginDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Q_UNUSED(_event)

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

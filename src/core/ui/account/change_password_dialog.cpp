#include "change_password_dialog.h"

#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/text_field/text_field.h>

#include <QEvent>
#include <QGridLayout>

namespace Ui {

class ChangePasswordDialog::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    TextField* restorePasswordConfirmationCode = nullptr;
    TextField* password = nullptr;

    QHBoxLayout* buttonsLayout = nullptr;
    Button* changePasswordButton = nullptr;
    Button* cancelButton = nullptr;
};

ChangePasswordDialog::Implementation::Implementation(QWidget* _parent)
    : restorePasswordConfirmationCode(new TextField(_parent))
    , password(new TextField(_parent))
    , buttonsLayout(new QHBoxLayout)
    , changePasswordButton(new Button(_parent))
    , cancelButton(new Button(_parent))
{
    restorePasswordConfirmationCode->setTabChangesFocus(true);

    password->setTabChangesFocus(true);
    password->setPasswordModeEnabled(true);
    password->setTrailingIcon(u8"\U000f06d1");

    buttonsLayout->setContentsMargins({});
    buttonsLayout->setSpacing(0);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(changePasswordButton);
    buttonsLayout->addWidget(cancelButton);

    password->hide();
    changePasswordButton->hide();
}


// ****


ChangePasswordDialog::ChangePasswordDialog(QWidget* _parent)
    : AbstractDialog(_parent)
    , d(new Implementation(this))
{
    d->cancelButton->installEventFilter(this);

    contentsLayout()->addWidget(d->restorePasswordConfirmationCode, 0, 0);
    contentsLayout()->addWidget(d->password, 1, 0);
    contentsLayout()->addLayout(d->buttonsLayout, 2, 0);

    connect(d->restorePasswordConfirmationCode, &TextField::textChanged, this,
            &ChangePasswordDialog::confirmationCodeEntered);
    connect(d->password, &TextField::trailingIconPressed, d->password, [password = d->password] {
        password->setPasswordModeEnabled(!password->isPasswordModeEnabled());
        password->setTrailingIcon(password->isPasswordModeEnabled() ? u8"\U000f06d1"
                                                                    : u8"\U000f06d0");
    });
    connect(d->changePasswordButton, &Button::clicked, this,
            &ChangePasswordDialog::changePasswordRequested);
    connect(d->cancelButton, &Button::clicked, this, &ChangePasswordDialog::canceled);

    updateTranslations();
    designSystemChangeEvent(nullptr);
}

ChangePasswordDialog::~ChangePasswordDialog() = default;

QString ChangePasswordDialog::code() const
{
    return d->restorePasswordConfirmationCode->text();
}

QString ChangePasswordDialog::password() const
{
    return d->password->text();
}

void ChangePasswordDialog::setConfirmationError(const QString& _error)
{
    d->restorePasswordConfirmationCode->setError(_error);
}

void ChangePasswordDialog::showChangePasswordFieldAndButton()
{
    d->password->show();
    d->password->setFocus();
    d->changePasswordButton->show();

    d->restorePasswordConfirmationCode->setEnabled(false);
}

QWidget* ChangePasswordDialog::focusedWidgetAfterShow() const
{
    return d->restorePasswordConfirmationCode;
}

QWidget* ChangePasswordDialog::lastFocusableWidget() const
{
    return d->cancelButton;
}

void ChangePasswordDialog::updateTranslations()
{
    setTitle(tr("Change password"));

    d->restorePasswordConfirmationCode->setLabel(tr("Confirmation code"));
    d->password->setLabel(tr("New password"));
    d->changePasswordButton->setText(tr("Change password"));
    d->cancelButton->setText(tr("Cancel"));
}

void ChangePasswordDialog::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    AbstractDialog::designSystemChangeEvent(_event);

    for (auto button : { d->changePasswordButton, d->cancelButton }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    contentsLayout()->setSpacing(static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->buttonsLayout->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px12(),
                  Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px8())
            .toMargins());
}

bool ChangePasswordDialog::eventFilter(QObject* _watched, QEvent* _event)
{
    //
    // Зацикливаем фокус, чтобы он всегда оставался внутри диалога
    //
    if (_watched == d->cancelButton && _event->type() == QEvent::FocusOut) {
        d->restorePasswordConfirmationCode->setFocus();
    }

    return AbstractDialog::eventFilter(_watched, _event);
}

} // namespace Ui

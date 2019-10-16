#include "account_manager.h"

#include <ui/design_system/design_system.h>

#include <ui/account/account_bar.h>
#include <ui/account/login_dialog.h>

#include <QWidget>


namespace ManagementLayer
{

class AccountManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QWidget* topLevelWidget = nullptr;

    Ui::AccountBar* accountBar = nullptr;
    Ui::LoginDialog* loginDialog = nullptr;

    QWidget* toolBar = nullptr;
    QWidget* navigator = nullptr;
    QWidget* view = nullptr;

    QPixmap avatar;
};

AccountManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent),
      accountBar(new Ui::AccountBar(topLevelWidget)),
      toolBar(new QWidget(topLevelWidget)),
      navigator(new QWidget(topLevelWidget)),
      view(new QWidget(topLevelWidget))
{
}


// ****


AccountManager::AccountManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->accountBar, &Ui::AccountBar::accountPressed, this, [this] {
        if (d->loginDialog == nullptr) {
            d->loginDialog = new Ui::LoginDialog(d->topLevelWidget);
            connect(d->loginDialog, &Ui::LoginDialog::emailEntered, this, &AccountManager::emailEntered);
            connect(d->loginDialog, &Ui::LoginDialog::registrationRequired, this, &AccountManager::registrationRequired);
            connect(d->loginDialog, &Ui::LoginDialog::confirmationCodeEntered,
                    this, &AccountManager::registrationConfirmationCodeEntered);
            connect(d->loginDialog, &Ui::LoginDialog::loginRequired, this, &AccountManager::loginRequired);
            connect(d->loginDialog, &Ui::LoginDialog::canceled, d->loginDialog, &Ui::LoginDialog::hideDialog);
            connect(d->loginDialog, &Ui::LoginDialog::disappeared, this, [this] {
                d->loginDialog->deleteLater();
                d->loginDialog = nullptr;
            });
        }

        d->loginDialog->showDialog();
    });
}

Widget* AccountManager::accountBar() const
{
    return d->accountBar;
}

AccountManager::~AccountManager() = default;

QWidget* AccountManager::toolBar() const
{
    return d->toolBar;
}

QWidget* AccountManager::navigator() const
{
    return d->navigator;
}

QWidget* AccountManager::view() const
{
    return d->view;
}

void AccountManager::login()
{
    Q_ASSERT(d->loginDialog);
    emit loginRequired(d->loginDialog->email(), d->loginDialog->password());
}

void AccountManager::allowRegistration()
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->showRegistrationButton();
}

void AccountManager::prepareToEnterRegistrationConfirmationCode()
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->showConfirmationCodeField();
}

void AccountManager::setRegistrationConfirmationError(const QString& _error)
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->setConfirmationError(_error);
}

void AccountManager::allowLogin()
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->showLoginButton();
}

void AccountManager::setLoginPasswordError(const QString& _error)
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->setPasswordError(_error);
}

void AccountManager::completeLogin()
{
    if (d->loginDialog != nullptr) {
        d->loginDialog->hideDialog();
    }

    notifyConnected();
}

void AccountManager::setAccountParameters(qint64 _availableSpace, const QString& _email,
    qint64 _monthPrice, bool _needNotify, const QString& _username, const QByteArray& _avatar)
{
    d->avatar.loadFromData(_avatar);

    if (!d->avatar.isNull()) {
        d->accountBar->setAvatar(d->avatar);
    }
}

void AccountManager::notifyConnected()
{
    d->accountBar->notify(Ui::DesignSystem::color().secondary());
}

void AccountManager::notifyDisconnected()
{
    d->accountBar->notify(Ui::DesignSystem::color().error());
}

} // namespace ManagementLayer

#include "account_manager.h"

#include <ui/design_system/design_system.h>

#include <ui/account/account_bar.h>
#include <ui/account/account_navigator.h>
#include <ui/account/account_tool_bar.h>
#include <ui/account/account_view.h>
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

    Ui::AccountToolBar* toolBar = nullptr;
    Ui::AccountNavigator* navigator = nullptr;
    Ui::AccountView* view = nullptr;

    /**
     * @biref Данные о пользователе
     */
    /** @{ */
    QString username;
    QString email;
    qint64 availableSpace = 0;
    qint64 monthPrice = 0;
    bool needNotify = true;
    QPixmap avatar;
    /** @} */
};

AccountManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent),
      accountBar(new Ui::AccountBar(topLevelWidget)),
      toolBar(new Ui::AccountToolBar(topLevelWidget)),
      navigator(new Ui::AccountNavigator(topLevelWidget)),
      view(new Ui::AccountView(topLevelWidget))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}


// ****


AccountManager::AccountManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->accountBar, &Ui::AccountBar::accountPressed, this, [this] {
        //
        // Если авторизованы
        //
        if (!d->email.isEmpty()) {
            //
            // Перейти в личный кабинет
            //
            emit showAccountRequired();
            return;
        }

        //
        // TODO: Если нет связи с сервером, показать сообщение, что авторизация временно недоступна
        //

        //
        // В противном случае, авторизоваться
        //
        if (d->loginDialog == nullptr) {
            d->loginDialog = new Ui::LoginDialog(d->topLevelWidget);
            connect(d->loginDialog, &Ui::LoginDialog::emailEntered, this, &AccountManager::emailEntered);
            connect(d->loginDialog, &Ui::LoginDialog::restorePasswordRequired, this, &AccountManager::restorePasswordRequired);
            connect(d->loginDialog, &Ui::LoginDialog::passwordRestoringConfirmationCodeEntered, this, &AccountManager::passwordRestoringConfirmationCodeEntered);
            connect(d->loginDialog, &Ui::LoginDialog::changePasswordRequested, this, &AccountManager::changePasswordRequested);
            connect(d->loginDialog, &Ui::LoginDialog::registrationRequired, this, &AccountManager::registrationRequired);
            connect(d->loginDialog, &Ui::LoginDialog::registrationConfirmationCodeEntered,
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

    connect(d->toolBar, &Ui::AccountToolBar::backPressed, this, &AccountManager::closeAccountRequired);
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
    d->loginDialog->showRegistrationConfirmationCodeField();
}

void AccountManager::setRegistrationConfirmationError(const QString& _error)
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->setRegistrationConfirmationError(_error);
}

void AccountManager::allowLogin()
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->showLoginButtons();
}

void AccountManager::prepareToEnterRestorePasswordConfirmationCode()
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->showRestorePasswordConfirmationCodeField();
}

void AccountManager::allowChangePassword()
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->showChangePasswordFiledAndButton();
}

void AccountManager::setRestorePasswordConfirmationError(const QString& _error)
{
    Q_ASSERT(d->loginDialog);
    d->loginDialog->setRestorePasswordConfirmationError(_error);
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
    d->availableSpace = _availableSpace;
    d->email = _email;
    d->monthPrice = _monthPrice;
    d->needNotify = _needNotify;
    d->username = _username;
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

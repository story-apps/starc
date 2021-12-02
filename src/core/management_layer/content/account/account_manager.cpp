#include "account_manager.h"

#include <ui/account/account_bar.h>
#include <ui/account/account_navigator.h>
#include <ui/account/account_tool_bar.h>
#include <ui/account/account_view.h>
#include <ui/account/login_dialog.h>
#include <ui/account/renew_subscription_dialog.h>
#include <ui/account/upgrade_to_pro_dialog.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/image_helper.h>

#include <QFileDialog>
#include <QWidget>


namespace ManagementLayer {

namespace {
constexpr int kInvalidConfirmationCodeLength = -1;
}

class AccountManager::Implementation
{
public:
    Implementation(AccountManager* _q, QWidget* _parent);

    /**
     * @brief Инициилизировать диалог авторизации
     */
    void initLoginDialog();


    AccountManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::LoginDialog* loginDialog = nullptr;
    int confirmationCodeLength = kInvalidConfirmationCodeLength;

    Ui::UpgradeToProDialog* upgradeToProDialog = nullptr;
    Ui::RenewSubscriptionDialog* renewSubscriptionDialog = nullptr;

    Ui::AccountToolBar* toolBar = nullptr;
    Ui::AccountNavigator* navigator = nullptr;
    Ui::AccountView* view = nullptr;

    /**
     * @biref Данные о пользователе
     */
    struct {
        QString email;
        QString name;
        QString description;
        QPixmap avatar;
    } account;
};

AccountManager::Implementation::Implementation(AccountManager* _q, QWidget* _parent)
    : q(_q)
    , topLevelWidget(_parent)
    , toolBar(new Ui::AccountToolBar(topLevelWidget))
    , navigator(new Ui::AccountNavigator(topLevelWidget))
    , view(new Ui::AccountView(topLevelWidget))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}

void AccountManager::Implementation::initLoginDialog()
{
    if (loginDialog) {
        return;
    }

    loginDialog = new Ui::LoginDialog(topLevelWidget);
    connect(loginDialog, &Ui::LoginDialog::signInPressed, q,
            [this] { emit q->askConfirmationCodeRequested(loginDialog->email()); });
    connect(loginDialog, &Ui::LoginDialog::confirmationCodeChanged, q,
            [this](const QString& _code) {
                if (confirmationCodeLength == kInvalidConfirmationCodeLength
                    || _code.length() != confirmationCodeLength) {
                    return;
                }

                emit q->checkConfirmationCodeRequested(_code);
            });
    connect(loginDialog, &Ui::LoginDialog::cancelPressed, loginDialog,
            &Ui::LoginDialog::hideDialog);
    connect(loginDialog, &Ui::LoginDialog::disappeared, loginDialog, [this] {
        loginDialog->deleteLater();
        loginDialog = nullptr;
    });
}


// ****


AccountManager::AccountManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(this, _parentWidget))
{
    initToolBarConnections();
    initNavigatorConnections();
    initViewConnections();
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

void AccountManager::signIn()
{
    d->initLoginDialog();
    d->loginDialog->showEmailStep();
    d->loginDialog->showDialog();
}

void AccountManager::setConfirmationCodeInfo(int _codeLength)
{
    d->confirmationCodeLength = _codeLength;
}

void AccountManager::completeSignIn(bool _openAccount)
{
    if (d->loginDialog != nullptr) {
        d->loginDialog->hideDialog();
    }

    if (_openAccount) {
        emit showAccountRequested();
    }
}

void AccountManager::setAccountInfo(const QString& _email, const QString& _name,
                                    const QString& _description, const QByteArray& _avatar)
{
    d->account.email = _email;
    d->view->setEmail(d->account.email);
    setName(_name);
    setDescription(_description);
    setAvatar(_avatar);
}

QString AccountManager::email() const
{
    return d->account.email;
}

QString AccountManager::name() const
{
    return d->account.name;
}

void AccountManager::setName(const QString& _name)
{
    d->account.name = _name;
    d->view->setName(d->account.name);
}

void AccountManager::setDescription(const QString& _description)
{
    d->account.description = _description;
    d->view->setDescription(d->account.description);
}

QPixmap AccountManager::avatar() const
{
    return d->account.avatar;
}

void AccountManager::setAvatar(const QByteArray& _avatar)
{
    setAvatar(ImageHelper::imageFromBytes(_avatar));
}

void AccountManager::setAvatar(const QPixmap& _avatar)
{
    d->account.avatar = _avatar;
    d->view->setAvatar(d->account.avatar);
}

void AccountManager::removeAvatar()
{
    d->account.avatar = {};
    d->view->setAvatar({});
}

void AccountManager::completeLogout()
{
    //    setAccountInfo(0, {}, 0, true, {}, {});
    removeAvatar();

    emit cloudProjectsCreationAvailabilityChanged(false, false);
    emit closeAccountRequested();
}

void AccountManager::setPaymentInfo(const PaymentInfo& _info)
{
    Q_UNUSED(_info)

    //
    // TODO: ждём реализацию валют, а пока просто не используем эту тему
    //
}

void AccountManager::setSubscriptionEnd(const QString& _subscriptionEnd)
{
    //    d->subscriptionEnd = _subscriptionEnd;
    //    d->navigator->setSubscriptionEnd(d->subscriptionEnd);
}

void AccountManager::setReceiveEmailNotifications(bool _receive)
{
    //    d->receiveEmailNotifications = _receive;
    //    d->view->setReceiveEmailNotifications(d->receiveEmailNotifications);
}

void AccountManager::initToolBarConnections()
{
    connect(d->toolBar, &Ui::AccountToolBar::backPressed, this,
            &AccountManager::closeAccountRequested);
}

void AccountManager::initNavigatorConnections()
{
    connect(d->navigator, &Ui::AccountNavigator::upgradeToProPressed, this, [this] {
        if (d->upgradeToProDialog == nullptr) {
            d->upgradeToProDialog = new Ui::UpgradeToProDialog(d->topLevelWidget);
            connect(d->upgradeToProDialog, &Ui::UpgradeToProDialog::canceled, d->upgradeToProDialog,
                    &Ui::UpgradeToProDialog::hideDialog);
            connect(d->upgradeToProDialog, &Ui::UpgradeToProDialog::disappeared, this, [this] {
                d->upgradeToProDialog->deleteLater();
                d->upgradeToProDialog = nullptr;
            });
        }

        d->upgradeToProDialog->showDialog();
    });
    connect(d->navigator, &Ui::AccountNavigator::renewSubscriptionPressed, this, [this] {
        if (d->renewSubscriptionDialog == nullptr) {
            d->renewSubscriptionDialog = new Ui::RenewSubscriptionDialog(d->topLevelWidget);
            connect(d->renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::renewPressed, this,
                    [this] {
                        emit renewSubscriptionRequested(d->renewSubscriptionDialog->monthCount(),
                                                        d->renewSubscriptionDialog->paymentType());
                        d->renewSubscriptionDialog->hideDialog();
                    });
            connect(d->renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::canceled,
                    d->renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::hideDialog);
            connect(d->renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::disappeared, this,
                    [this] {
                        d->renewSubscriptionDialog->deleteLater();
                        d->renewSubscriptionDialog = nullptr;
                    });
        }

        d->renewSubscriptionDialog->showDialog();
    });
}

void AccountManager::initViewConnections()
{
    //    connect(d->view, &Ui::AccountView::changePasswordPressed, this, [this] {
    //        if (d->changePasswordDialog == nullptr) {
    //            d->changePasswordDialog = new Ui::ChangePasswordDialog(d->topLevelWidget);
    //            connect(d->changePasswordDialog,
    //            &Ui::ChangePasswordDialog::confirmationCodeEntered,
    //                    this, [this] {
    //                        emit passwordRestoringConfirmationCodeEntered(
    //                            d->email, d->changePasswordDialog->code());
    //                    });
    //            connect(d->changePasswordDialog,
    //            &Ui::ChangePasswordDialog::changePasswordRequested,
    //                    this, [this] {
    //                        emit changePasswordRequested(d->email,
    //                        d->changePasswordDialog->code(),
    //                                                     d->changePasswordDialog->password());
    //                    });
    //            connect(d->changePasswordDialog, &Ui::ChangePasswordDialog::canceled,
    //                    d->changePasswordDialog, &Ui::ChangePasswordDialog::hideDialog);
    //            connect(d->changePasswordDialog, &Ui::ChangePasswordDialog::disappeared, this,
    //            [this] {
    //                d->changePasswordDialog->deleteLater();
    //                d->changePasswordDialog = nullptr;
    //            });
    //        }

    //        d->changePasswordDialog->showDialog();

    //        emit restorePasswordRequested(d->email);
    //    });
    connect(d->view, &Ui::AccountView::logoutPressed, this, &AccountManager::logoutRequested);
    connect(d->view, &Ui::AccountView::userNameChanged, this,
            &AccountManager::changeUserNameRequested);
    connect(d->view, &Ui::AccountView::receiveEmailNotificationsChanged, this,
            &AccountManager::changeReceiveEmailNotificationsRequested);
    connect(d->view, &Ui::AccountView::avatarChoosePressed, this, [this] {
        const QString avatarPath
            = QFileDialog::getOpenFileName(d->view, tr("Choose avatar"), {}, "");
        if (avatarPath.isEmpty()) {
            return;
        }

        QPixmap avatar(avatarPath);
        if (avatar.isNull()) {
            return;
        }

        setAvatar(avatar.scaled(150, 150, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));

        emit changeAvatarRequested(ImageHelper::bytesFromImage(d->account.avatar));
    });
}

} // namespace ManagementLayer

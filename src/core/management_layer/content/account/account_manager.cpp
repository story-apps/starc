#include "account_manager.h"

#include <ui/account/account_bar.h>
#include <ui/account/account_navigator.h>
#include <ui/account/account_tool_bar.h>
#include <ui/account/account_view.h>
#include <ui/account/change_password_dialog.h>
#include <ui/account/login_dialog.h>
#include <ui/account/renew_subscription_dialog.h>
#include <ui/account/upgrade_to_pro_dialog.h>
#include <ui/design_system/design_system.h>
#include <utils/helpers/image_helper.h>

#include <QFileDialog>
#include <QWidget>


namespace ManagementLayer {

class AccountManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QWidget* topLevelWidget = nullptr;

    Ui::AccountBar* accountBar = nullptr;
    Ui::LoginDialog* loginDialog = nullptr;
    Ui::UpgradeToProDialog* upgradeToProDialog = nullptr;
    Ui::RenewSubscriptionDialog* renewSubscriptionDialog = nullptr;
    Ui::ChangePasswordDialog* changePasswordDialog = nullptr;

    Ui::AccountToolBar* toolBar = nullptr;
    Ui::AccountNavigator* navigator = nullptr;
    Ui::AccountView* view = nullptr;

    /**
     * @biref Данные о пользователе
     */
    /** @{ */
    qint64 availableSpace = 0;
    qint64 monthPrice = 0;
    QString subscriptionEnd;
    QString email;
    QString userName;
    bool receiveEmailNotifications = true;
    QPixmap avatar;
    /** @} */
};

AccountManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent)
    , accountBar(new Ui::AccountBar(topLevelWidget))
    , toolBar(new Ui::AccountToolBar(topLevelWidget))
    , navigator(new Ui::AccountNavigator(topLevelWidget))
    , view(new Ui::AccountView(topLevelWidget))
{
    toolBar->hide();
    navigator->hide();
    view->hide();
}


// ****


AccountManager::AccountManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(_parentWidget))
{
    initAccountBarConnections();
    initToolBarConnections();
    initNavigatorConnections();
    initViewConnections();
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
    Q_ASSERT(d->loginDialog || d->changePasswordDialog);
    if (d->loginDialog != nullptr) {
        emit loginRequested(d->loginDialog->email(), d->loginDialog->password());
    } else if (d->changePasswordDialog != nullptr) {
        emit loginRequested(d->email, d->changePasswordDialog->password());
    }
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
    Q_ASSERT(d->loginDialog || d->changePasswordDialog);
    if (d->loginDialog != nullptr) {
        d->loginDialog->showRestorePasswordConfirmationCodeField();
    }
}

void AccountManager::allowChangePassword()
{
    Q_ASSERT(d->loginDialog || d->changePasswordDialog);
    if (d->loginDialog != nullptr) {
        d->loginDialog->showChangePasswordFieldAndButton();
    } else if (d->changePasswordDialog != nullptr) {
        d->changePasswordDialog->showChangePasswordFieldAndButton();
    }
}

void AccountManager::setRestorePasswordConfirmationError(const QString& _error)
{
    Q_ASSERT(d->loginDialog || d->changePasswordDialog);
    if (d->loginDialog != nullptr) {
        d->loginDialog->setRestorePasswordConfirmationError(_error);
    } else if (d->changePasswordDialog != nullptr) {
        d->changePasswordDialog->setConfirmationError(_error);
    }
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
    if (d->changePasswordDialog != nullptr) {
        d->changePasswordDialog->hideDialog();
    }

    notifyConnected();
}

void AccountManager::completeLogout()
{
    setAccountParameters(0, {}, 0, true, {}, {});
    removeAvatar();

    emit cloudProjectsCreationAvailabilityChanged(false, false);
    emit closeAccountRequested();
}

void AccountManager::setAccountParameters(qint64 _availableSpace, const QString& _email,
                                          qint64 _monthPrice, bool _receiveEmailNotifications,
                                          const QString& _userName, const QByteArray& _avatar)
{
    d->monthPrice = _monthPrice;
    d->availableSpace = _availableSpace;
    d->email = _email;
    d->view->setEmail(d->email);
    setUserName(_userName);
    setReceiveEmailNotifications(_receiveEmailNotifications);
    setAvatar(_avatar);

    //
    // TODO
    //
    bool _subscriptionIsActive = false;
    emit cloudProjectsCreationAvailabilityChanged(true, _subscriptionIsActive);
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
    d->subscriptionEnd = _subscriptionEnd;
    d->navigator->setSubscriptionEnd(d->subscriptionEnd);
}

void AccountManager::setUserName(const QString& _userName)
{
    d->userName = _userName;
    d->view->setUserName(d->userName);
}

void AccountManager::setReceiveEmailNotifications(bool _receive)
{
    d->receiveEmailNotifications = _receive;
    d->view->setReceiveEmailNotifications(d->receiveEmailNotifications);
}

void AccountManager::setAvatar(const QByteArray& _avatar)
{
    setAvatar(ImageHelper::imageFromBytes(_avatar));
}

void AccountManager::setAvatar(const QPixmap& _avatar)
{
    d->avatar = _avatar;
    if (d->avatar.isNull()) {
        d->avatar = QPixmap(":/images/default-avatar");
    }

    d->accountBar->setAvatar(d->avatar);
    d->view->setAvatar(d->avatar);
}

void AccountManager::removeAvatar()
{
    d->avatar = {};
    d->accountBar->setAvatar({});
    d->view->setAvatar({});
}

void AccountManager::notifyConnected()
{
    d->accountBar->notify(Ui::DesignSystem::color().secondary());
}

void AccountManager::notifyDisconnected()
{
    d->accountBar->notify(Ui::DesignSystem::color().error());
}

void AccountManager::initAccountBarConnections()
{
    connect(d->accountBar, &Ui::AccountBar::accountPressed, this, [this] {
        //
        // Если авторизованы
        //
        if (!d->email.isEmpty()) {
            //
            // Перейти в личный кабинет
            //
            emit showAccountRequested();
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
            connect(d->loginDialog, &Ui::LoginDialog::emailEntered, this,
                    [this] { emit emailEntered(d->loginDialog->email()); });
            connect(d->loginDialog, &Ui::LoginDialog::restorePasswordRequested, this,
                    [this] { emit restorePasswordRequested(d->loginDialog->email()); });
            connect(d->loginDialog, &Ui::LoginDialog::passwordRestoringConfirmationCodeEntered,
                    this, [this] {
                        emit passwordRestoringConfirmationCodeEntered(
                            d->loginDialog->email(),
                            d->loginDialog->restorePasswordConfirmationCode());
                    });
            connect(d->loginDialog, &Ui::LoginDialog::changePasswordRequested, this, [this] {
                emit changePasswordRequested(d->loginDialog->email(),
                                             d->loginDialog->restorePasswordConfirmationCode(),
                                             d->loginDialog->password());
            });
            connect(d->loginDialog, &Ui::LoginDialog::registrationRequested, this, [this] {
                emit registrationRequested(d->loginDialog->email(), d->loginDialog->password());
            });
            connect(d->loginDialog, &Ui::LoginDialog::registrationConfirmationCodeEntered, this,
                    [this] {
                        emit registrationConfirmationCodeEntered(
                            d->loginDialog->email(),
                            d->loginDialog->registractionConfirmationCode());
                    });
            connect(d->loginDialog, &Ui::LoginDialog::loginRequested, this, [this] {
                emit loginRequested(d->loginDialog->email(), d->loginDialog->password());
            });
            connect(d->loginDialog, &Ui::LoginDialog::canceled, d->loginDialog,
                    &Ui::LoginDialog::hideDialog);
            connect(d->loginDialog, &Ui::LoginDialog::disappeared, this, [this] {
                d->loginDialog->deleteLater();
                d->loginDialog = nullptr;
            });
        }

        d->loginDialog->showDialog();
    });
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
    connect(d->view, &Ui::AccountView::changePasswordPressed, this, [this] {
        if (d->changePasswordDialog == nullptr) {
            d->changePasswordDialog = new Ui::ChangePasswordDialog(d->topLevelWidget);
            connect(d->changePasswordDialog, &Ui::ChangePasswordDialog::confirmationCodeEntered,
                    this, [this] {
                        emit passwordRestoringConfirmationCodeEntered(
                            d->email, d->changePasswordDialog->code());
                    });
            connect(d->changePasswordDialog, &Ui::ChangePasswordDialog::changePasswordRequested,
                    this, [this] {
                        emit changePasswordRequested(d->email, d->changePasswordDialog->code(),
                                                     d->changePasswordDialog->password());
                    });
            connect(d->changePasswordDialog, &Ui::ChangePasswordDialog::canceled,
                    d->changePasswordDialog, &Ui::ChangePasswordDialog::hideDialog);
            connect(d->changePasswordDialog, &Ui::ChangePasswordDialog::disappeared, this, [this] {
                d->changePasswordDialog->deleteLater();
                d->changePasswordDialog = nullptr;
            });
        }

        d->changePasswordDialog->showDialog();

        emit restorePasswordRequested(d->email);
    });
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

        emit changeAvatarRequested(ImageHelper::bytesFromImage(d->avatar));
    });
}

} // namespace ManagementLayer

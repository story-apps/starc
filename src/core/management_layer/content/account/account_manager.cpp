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
     * @brief Настроить соединения зависящие от действий пользователя в интерфейсе
     */
    void initToolBarConnections();
    void initNavigatorConnections();
    void initViewConnections();

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

    initToolBarConnections();
    initNavigatorConnections();
    initViewConnections();
}

void AccountManager::Implementation::initToolBarConnections()
{
    connect(toolBar, &Ui::AccountToolBar::backPressed, q, &AccountManager::closeAccountRequested);
}

void AccountManager::Implementation::initNavigatorConnections()
{
    //    connect(navigator, &Ui::AccountNavigator::upgradeToProPressed, q, [this] {
    //        if (upgradeToProDialog == nullptr) {
    //            upgradeToProDialog = new Ui::UpgradeToProDialog(topLevelWidget);
    //            connect(upgradeToProDialog, &Ui::UpgradeToProDialog::canceled, upgradeToProDialog,
    //                    &Ui::UpgradeToProDialog::hideDialog);
    //            connect(upgradeToProDialog, &Ui::UpgradeToProDialog::disappeared, q, [this] {
    //                upgradeToProDialog->deleteLater();
    //                upgradeToProDialog = nullptr;
    //            });
    //        }

    //        upgradeToProDialog->showDialog();
    //    });
    //    connect(navigator, &Ui::AccountNavigator::renewSubscriptionPressed, q, [this] {
    //        if (renewSubscriptionDialog == nullptr) {
    //            renewSubscriptionDialog = new Ui::RenewSubscriptionDialog(topLevelWidget);
    //            connect(renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::renewPressed, q,
    //                    [this] {
    //                        emit renewSubscriptionRequested(renewSubscriptionDialog->monthCount(),
    //                                                        renewSubscriptionDialog->paymentType());
    //                        renewSubscriptionDialog->hideDialog();
    //                    });
    //            connect(renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::canceled,
    //                    renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::hideDialog);
    //            connect(renewSubscriptionDialog, &Ui::RenewSubscriptionDialog::disappeared, q,
    //                    [this] {
    //                        renewSubscriptionDialog->deleteLater();
    //                        renewSubscriptionDialog = nullptr;
    //                    });
    //        }

    //        renewSubscriptionDialog->showDialog();
    //    });
    //    connect(view, &Ui::AccountView::logoutPressed, q, &AccountManager::logoutRequested);
}

void AccountManager::Implementation::initViewConnections()
{
    auto notifyUpdateAccountInfoRequested = [this] {
        emit q->updateAccountInfoRequested(account.name, account.description,
                                           ImageHelper::bytesFromImage(account.avatar));
    };

    connect(view, &Ui::AccountView::nameChanged, q,
            [this, notifyUpdateAccountInfoRequested](const QString& _name) {
                account.name = _name;
                notifyUpdateAccountInfoRequested();
            });
    connect(view, &Ui::AccountView::descriptionChanged, q,
            [this, notifyUpdateAccountInfoRequested](const QString& _description) {
                account.description = _description;
                notifyUpdateAccountInfoRequested();
            });
    connect(view, &Ui::AccountView::avatarChanged, q,
            [this, notifyUpdateAccountInfoRequested](const QPixmap& _avatar) {
                if (!_avatar.isNull()) {
                    account.avatar
                        = _avatar.scaled(288, 288, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
                } else {
                    account.avatar = {};
                }

                notifyUpdateAccountInfoRequested();
            });
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

} // namespace ManagementLayer

#include "account_manager.h"

#include <domain/payent_info.h>
#include <domain/subscription_info.h>
#include <ui/account/account_navigator.h>
#include <ui/account/account_tool_bar.h>
#include <ui/account/account_view.h>
#include <ui/account/login_dialog.h>
#include <ui/account/renew_subscription_dialog.h>
#include <ui/account/upgrade_to_pro_dialog.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/dialog/dialog.h>
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
     * @brief Методы работы с аватаром
     */
    void setAvatar(const QByteArray& _avatar);
    void setAvatar(const QPixmap& _avatar);
    void removeAvatar();

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
     * @biref Данные о пользователе, которые могут быть изменены непосредственно в личном кабинете
     */
    struct {
        QString email;
        QString name;
        QString description;
        QPixmap avatar;
    } account;

    /**
     * @brief Доступные опции для покупки
     */
    QVector<Domain::PaymentOption> paymentOptions;
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
    connect(navigator, &Ui::AccountNavigator::accountPressed, view, &Ui::AccountView::showAccount);
    connect(navigator, &Ui::AccountNavigator::subscriptionPressed, view,
            &Ui::AccountView::showSubscription);
    connect(navigator, &Ui::AccountNavigator::sessionsPressed, view,
            &Ui::AccountView::showSessions);

    connect(navigator, &Ui::AccountNavigator::upgradeToProPressed, q,
            &AccountManager::upgradeAccount);

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

    connect(navigator, &Ui::AccountNavigator::logoutPressed, q, [this] {
        account = {};
        emit q->logoutRequested();
        emit q->closeAccountRequested();
    });
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

    connect(view, &Ui::AccountView::upgradeToProPressed, q, &AccountManager::upgradeAccount);

    connect(view, &Ui::AccountView::terminateSessionRequested, q,
            &AccountManager::terminateSessionRequested);
}

void AccountManager::Implementation::setAvatar(const QByteArray& _avatar)
{
    setAvatar(ImageHelper::imageFromBytes(_avatar));
}

void AccountManager::Implementation::setAvatar(const QPixmap& _avatar)
{
    account.avatar = _avatar;
    view->setAvatar(account.avatar);
}

void AccountManager::Implementation::removeAvatar()
{
    account.avatar = {};
    view->setAvatar({});
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
                                    const QString& _description, const QByteArray& _avatar,
                                    Domain::SubscriptionType _subscriptionType,
                                    const QDateTime& _subscriptionEnds,
                                    const QVector<Domain::PaymentOption>& _paymentOptions,
                                    const QVector<Domain::SessionInfo>& _sessions)
{
    d->account.email = _email;
    d->view->setEmail(d->account.email);
    d->account.name = _name;
    d->view->setName(d->account.name);
    d->account.description = _description;
    d->view->setDescription(d->account.description);
    d->setAvatar(_avatar);
    d->paymentOptions = _paymentOptions;

    d->navigator->setSubscriptionInfo(_subscriptionType, _subscriptionEnds, _paymentOptions);
    d->view->setSubscriptionInfo(_subscriptionType, _subscriptionEnds, _paymentOptions);

    d->view->setSessions(_sessions);
}

QString AccountManager::email() const
{
    return d->account.email;
}

QString AccountManager::name() const
{
    return d->account.name;
}

QPixmap AccountManager::avatar() const
{
    return d->account.avatar;
}

void AccountManager::upgradeAccount()
{
    //
    // Если ещё не авторизован, то отправим на авторизацию
    //
    if (d->account.email.isEmpty()) {
        signIn();
    }
    //
    // Иначе, покажем диалог с апгрейдом аккаунта
    //
    else {
        //
        // Ищем бесплатную активацию
        //
        Domain::PaymentOption freeOption;
        for (const auto& paymentOption : std::as_const(d->paymentOptions)) {
            if (paymentOption.amount != 0
                || paymentOption.subscriptionType != Domain::SubscriptionType::ProMonthly) {
                continue;
            }

            freeOption = paymentOption;
            break;
        }

        //
        // Если удалось найти бесплатную версию, то предлагаем пользователю активировать бесплатно
        //
        if (freeOption.isValid()) {
            auto dialog = new Dialog(d->view->topLevelWidget());
            dialog->setContentMaximumWidth(Ui::DesignSystem::dialog().maximumWidth());
            dialog->showDialog(tr("Try PRO version for free"),
                               tr("Since we are in the beta test, you can activate the PRO version "
                                  "for free. When the beta test ends, you'll have the ability to "
                                  "activate the PRO version for 30 days for free."),
                               { { 0, tr("Continue with free version"), Dialog::RejectButton },
                                 { 1, tr("Activate PRO"), Dialog::AcceptButton } });
            QObject::connect(dialog, &Dialog::finished, this,
                             [this, dialog, freeOption](const Dialog::ButtonInfo& _presedButton) {
                                 dialog->hideDialog();
                                 if (_presedButton.type == Dialog::AcceptButton) {
                                     emit activatePaymentOptionRequested(freeOption);
                                 }
                             });
            QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
            return;
        }
    }
}

} // namespace ManagementLayer

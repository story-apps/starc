#include "account_manager.h"

#include <domain/starcloud_api.h>
#include <ui/account/account_navigator.h>
#include <ui/account/account_tool_bar.h>
#include <ui/account/account_view.h>
#include <ui/account/login_dialog.h>
#include <ui/account/purchase_dialog.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <utils/helpers/image_helper.h>

#include <QTimer>
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

    /**
     * @brief Инициилизировать диалог поккупки услуг
     */
    void initPurchaseDialog();


    AccountManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::LoginDialog* loginDialog = nullptr;
    int confirmationCodeLength = kInvalidConfirmationCodeLength;

    Ui::PurchaseDialog* purchaseDialog = nullptr;

    Ui::AccountToolBar* toolBar = nullptr;
    Ui::AccountNavigator* navigator = nullptr;
    Ui::AccountView* view = nullptr;

    /**
     * @biref Данные о пользователе, которые могут быть изменены непосредственно в личном кабинете
     */
    Domain::AccountInfo accountInfo;

    /**
     * @brief Таймер проверки окончания подписки (срабатывает в момент завершения подписки)
     */
    QTimer subscriptionEndsTimer;
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

    subscriptionEndsTimer.setSingleShot(true);
    connect(&subscriptionEndsTimer, &QTimer::timeout, q, &AccountManager::askAccountInfoRequested);
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

    connect(navigator, &Ui::AccountNavigator::tryProForFreePressed, q,
            &AccountManager::tryProForFree);
    connect(navigator, &Ui::AccountNavigator::buyProLifetimePressed, q,
            &AccountManager::buyProLifetme);
    connect(navigator, &Ui::AccountNavigator::renewProPressed, q, &AccountManager::renewPro);
    connect(navigator, &Ui::AccountNavigator::tryTeamForFreePressed, q,
            &AccountManager::tryTeamForFree);
    connect(navigator, &Ui::AccountNavigator::renewTeamPressed, q, &AccountManager::renewTeam);
    connect(navigator, &Ui::AccountNavigator::buyCreditsPressed, q, &AccountManager::buyCredits);
    connect(navigator, &Ui::AccountNavigator::logoutPressed, q, [this] {
        q->clearAccountInfo();
        emit q->logoutRequested();
    });
}

void AccountManager::Implementation::initViewConnections()
{
    auto notifyUpdateAccountInfoRequested = [this] {
        emit q->updateAccountInfoRequested(accountInfo.email, accountInfo.name,
                                           accountInfo.description, accountInfo.newsletterLanguage,
                                           accountInfo.newsletterSubscribed, accountInfo.avatar);
    };

    connect(view, &Ui::AccountView::nameChanged, q,
            [this, notifyUpdateAccountInfoRequested](const QString& _name) {
                accountInfo.name = _name;
                notifyUpdateAccountInfoRequested();
            });
    connect(view, &Ui::AccountView::descriptionChanged, q,
            [this, notifyUpdateAccountInfoRequested](const QString& _description) {
                accountInfo.description = _description;
                notifyUpdateAccountInfoRequested();
            });
    connect(view, &Ui::AccountView::newsletterSubscriptionChanged, q,
            [this, notifyUpdateAccountInfoRequested](bool _subscribed) {
                accountInfo.newsletterLanguage
                    = QLocale().language() == QLocale::Russian ? "ru" : "en";
                accountInfo.newsletterSubscribed = _subscribed;
                notifyUpdateAccountInfoRequested();
            });
    connect(view, &Ui::AccountView::avatarChanged, q,
            [this, notifyUpdateAccountInfoRequested](const QPixmap& _avatar) {
                if (!_avatar.isNull()) {
                    accountInfo.avatar = ImageHelper::bytesFromImage(
                        _avatar.scaled(288, 288, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
                } else {
                    accountInfo.avatar = {};
                }

                notifyUpdateAccountInfoRequested();
            });

    connect(view, &Ui::AccountView::tryProForFreePressed, q, &AccountManager::tryProForFree);
    connect(view, &Ui::AccountView::tryTeamForFreePressed, q, &AccountManager::tryTeamForFree);
    connect(view, &Ui::AccountView::buyProLifetimePressed, q, &AccountManager::buyProLifetme);
    connect(view, &Ui::AccountView::renewProPressed, q, &AccountManager::renewPro);
    connect(view, &Ui::AccountView::renewTeamPressed, q, &AccountManager::renewTeam);
    connect(view, &Ui::AccountView::activatePromocodePressed, q,
            &AccountManager::activatePromocodeRequested);

    connect(view, &Ui::AccountView::terminateSessionRequested, q,
            &AccountManager::terminateSessionRequested);
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

void AccountManager::Implementation::initPurchaseDialog()
{
    if (purchaseDialog == nullptr) {
        purchaseDialog = new Ui::PurchaseDialog(view->topLevelWidget());
        connect(purchaseDialog, &Ui::PurchaseDialog::purchasePressed, q,
                &AccountManager::activatePaymentOptionRequested);
        connect(purchaseDialog, &Ui::PurchaseDialog::canceled, purchaseDialog,
                &Ui::PurchaseDialog::hideDialog);
        connect(purchaseDialog, &Ui::PurchaseDialog::disappeared, purchaseDialog, [this] {
            purchaseDialog->deleteLater();
            purchaseDialog = nullptr;
        });
    }

    purchaseDialog->setPaymentOptions(accountInfo.paymentOptions);
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

void AccountManager::setConnected(bool _connected)
{
    d->navigator->setConnected(_connected);
    d->view->setConnected(_connected);
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

void AccountManager::setAccountInfo(const Domain::AccountInfo& _accountInfo)
{
    d->accountInfo = _accountInfo;

    d->view->setEmail(d->accountInfo.email);
    d->view->setName(d->accountInfo.name);
    d->view->setDescription(d->accountInfo.description);
    d->view->setNewsletterSubscribed(d->accountInfo.newsletterSubscribed);
    d->view->setAvatar(ImageHelper::imageFromBytes(d->accountInfo.avatar));

    d->navigator->setAccountInfo(d->accountInfo);
    d->view->setAccountInfo(d->accountInfo);
    d->view->setSessions(d->accountInfo.sessions);

    //
    // Взводим таймер проверки окончания сессии
    //
    if (d->subscriptionEndsTimer.isActive()) {
        d->subscriptionEndsTimer.stop();
    }
    const auto currentDateTime = QDateTime::currentDateTimeUtc();
    //
    // Тут для простоты берём последнюю из действующих подписок, т.к. если их несколько,
    // то первая будет PRO Lifetime
    //
    const auto& subscription = d->accountInfo.subscriptions.constLast();
    if (currentDateTime < subscription.end) {
        //
        // Т.к. таймер запускается на int миллисекунд, нельзя в него передавать слишком большое
        // значение (количество миллисекунд до самого конца подписки), выходящее за пределы INT_MAX
        //
        const qint64 maxInterval = INT_MAX;
        const auto interval = std::min(currentDateTime.msecsTo(subscription.end), maxInterval);
        d->subscriptionEndsTimer.start(interval);
    }

    //
    // Если информация обновилась во время оплаты лицензии, то закроем диалог платежей
    //
    if (d->purchaseDialog != nullptr && d->purchaseDialog->isVisible()) {
        d->purchaseDialog->hideDialog();
    }
}

void AccountManager::clearAccountInfo()
{
    d->accountInfo = {};
}

void AccountManager::upgradeAccountToPro()
{
    //
    // Если ещё не авторизован, то отправим на авторизацию
    //
    if (d->accountInfo.email.isEmpty()) {
        signIn();
    }
    //
    // Иначе, покажем диалог с апгрейдом аккаунта
    //
    else {
        const auto canBeUpdatedForFree = tryProForFree();
        if (canBeUpdatedForFree) {
            return;
        }

        buyProLifetme();
    }
}

void AccountManager::upgradeAccountToTeam()
{
    //
    // Если ещё не авторизован, то отправим на авторизацию
    //
    if (d->accountInfo.email.isEmpty()) {
        signIn();
    }
    //
    // Иначе, покажем диалог с апгрейдом аккаунта
    //
    else {
        const auto canBeUpdatedForFree = tryTeamForFree();
        if (canBeUpdatedForFree) {
            return;
        }

        renewTeam();
    }
}

bool AccountManager::tryProForFree()
{
    //
    // Ищем бесплатную активацию
    //
    Domain::PaymentOption freeOption;
    for (const auto& paymentOption : std::as_const(d->accountInfo.paymentOptions)) {
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
        dialog->showDialog(
            tr("Try PRO version for free"),
            tr("You can try all the features of the PRO version during 30 days for free. After "
               "trial period, you can continue to use the PRO version by renewing your "
               "subscription. Otherwise, you'll be returned to the FREE version automatically."),
            { { 0, tr("Continue with FREE version"), Dialog::RejectButton },
              { 1, tr("Activate PRO"), Dialog::AcceptButton } });
        QObject::connect(dialog, &Dialog::finished, this,
                         [this, dialog, freeOption](const Dialog::ButtonInfo& _presedButton) {
                             dialog->hideDialog();
                             if (_presedButton.type == Dialog::AcceptButton) {
                                 emit activatePaymentOptionRequested(freeOption);
                             }
                         });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
        return true;
    }

    return false;
}

void AccountManager::buyProLifetme()
{
    d->initPurchaseDialog();

    auto proPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = proPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = proPaymentOptions.at(index);
        if (option.amount == 0
            || (option.subscriptionType != Domain::SubscriptionType::ProMonthly
                && option.subscriptionType != Domain::SubscriptionType::ProLifetime)) {
            proPaymentOptions.removeAt(index);
        }
    }

    d->purchaseDialog->setPaymentOptions(proPaymentOptions);
    d->purchaseDialog->selectOption(proPaymentOptions.constFirst());
    d->purchaseDialog->showDialog();
}

void AccountManager::renewPro()
{
    auto proPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = proPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = proPaymentOptions.at(index);
        if (option.amount == 0 || option.type != Domain::PaymentType::Subscription
            || (option.subscriptionType != Domain::SubscriptionType::ProMonthly
                && option.subscriptionType != Domain::SubscriptionType::ProLifetime)) {
            proPaymentOptions.removeAt(index);
        }
    }
    if (proPaymentOptions.isEmpty()) {
        return;
    }

    d->initPurchaseDialog();
    d->purchaseDialog->setPaymentOptions(proPaymentOptions);
    d->purchaseDialog->selectOption(proPaymentOptions.constLast());
    d->purchaseDialog->showDialog();
}

bool AccountManager::tryTeamForFree()
{
    //
    // Ищем бесплатную активацию
    //
    Domain::PaymentOption freeOption;
    for (const auto& paymentOption : std::as_const(d->accountInfo.paymentOptions)) {
        if (paymentOption.amount != 0
            || paymentOption.subscriptionType != Domain::SubscriptionType::TeamMonthly) {
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
        dialog->showDialog(
            tr("Try TEAM version for free"),
            tr("You can try all the features of the TEAM version during 30 days for free. After "
               "trial period, you can continue to use the TEAM version by renewing your "
               "subscription. Otherwise, you'll be returned to the FREE version automatically."),
            { { 0, tr("Continue with FREE version"), Dialog::RejectButton },
              { 1, tr("Activate TEAM"), Dialog::AcceptButton } });
        QObject::connect(dialog, &Dialog::finished, this,
                         [this, dialog, freeOption](const Dialog::ButtonInfo& _presedButton) {
                             dialog->hideDialog();
                             if (_presedButton.type == Dialog::AcceptButton) {
                                 emit activatePaymentOptionRequested(freeOption);
                             }
                         });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
        return true;
    }

    return false;
}

void AccountManager::renewTeam()
{
    auto teamPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = teamPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = teamPaymentOptions.at(index);
        if (option.amount == 0 || option.type != Domain::PaymentType::Subscription
            || (option.subscriptionType != Domain::SubscriptionType::TeamMonthly
                && option.subscriptionType != Domain::SubscriptionType::TeamLifetime)) {
            teamPaymentOptions.removeAt(index);
        }
    }
    if (teamPaymentOptions.isEmpty()) {
        return;
    }

    d->initPurchaseDialog();
    d->purchaseDialog->setPaymentOptions(teamPaymentOptions);
    d->purchaseDialog->selectOption(teamPaymentOptions.constLast());
    d->purchaseDialog->showDialog();
}

void AccountManager::buyCredits()
{
    auto teamPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = teamPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = teamPaymentOptions.at(index);
        if (option.type != Domain::PaymentType::Credits) {
            teamPaymentOptions.removeAt(index);
        }
    }
    if (teamPaymentOptions.isEmpty()) {
        return;
    }

    d->initPurchaseDialog();
    d->purchaseDialog->setPaymentOptions(teamPaymentOptions);
    d->purchaseDialog->selectOption(teamPaymentOptions.constLast());
    d->purchaseDialog->showDialog();
}

void AccountManager::showPromocodeActivationMessage(const QString& _message)
{
    StandardDialog::information(d->view->topLevelWidget(), {}, _message);
    d->view->clearPromocode();
}

void AccountManager::setPromocodeError(const QString& _error)
{
    d->view->setPromocodeError(_error);
}

} // namespace ManagementLayer

#include "account_manager.h"

#include <domain/starcloud_api.h>
#include <ui/account/account_navigator.h>
#include <ui/account/account_tool_bar.h>
#include <ui/account/account_view.h>
#include <ui/account/login_dialog.h>
#include <ui/account/purchase_dialog.h>
#include <ui/account/purchase_gift_dialog.h>
#include <ui/account/team_dialog.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/dialog/dialog.h>
#include <ui/widgets/dialog/standard_dialog.h>
#include <utils/helpers/image_helper.h>

#include <QTimer>
#include <QWidget>


namespace ManagementLayer {

namespace {
constexpr int kInvalidConfirmationCodeLength = -1;

QByteArray avatarData(const QPixmap& _avatar)
{
    return ImageHelper::bytesFromImage(
        _avatar.scaled(288, 288, Qt::IgnoreAspectRatio, Qt::SmoothTransformation));
}
} // namespace

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
    void initCrossConnections();

    /**
     * @brief Инициилизировать диалог авторизации
     */
    void initLoginDialog();

    /**
     * @brief Инициилизировать диалог поккупки услуг
     */
    void initPurchaseDialog();

    /**
     * @brief Инициилизировать диалог поккупки услуг в подарок
     */
    void initPurchaseGiftDialog();


    AccountManager* q = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::LoginDialog* loginDialog = nullptr;
    int confirmationCodeLength = kInvalidConfirmationCodeLength;

    Ui::PurchaseDialog* purchaseDialog = nullptr;
    Ui::PurchaseGiftDialog* purchaseGiftDialog = nullptr;

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

    /**
     * @brief Команды пользователя
     */
    QVector<Domain::TeamInfo> accountTeams;
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
    initCrossConnections();

    subscriptionEndsTimer.setSingleShot(true);
    connect(&subscriptionEndsTimer, &QTimer::timeout, q, &AccountManager::askAccountInfoRequested);
}

void AccountManager::Implementation::initToolBarConnections()
{
    connect(toolBar, &Ui::AccountToolBar::backPressed, q, &AccountManager::closeAccountRequested);
    connect(toolBar, &Ui::AccountToolBar::accountPressed, q, [this] {
        navigator->showAccountPage();
        view->showAccountPage();
    });
    connect(toolBar, &Ui::AccountToolBar::teamPressed, q, [this] {
        navigator->showTeamPage();
        view->showTeamPage();
    });
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
    connect(navigator, &Ui::AccountNavigator::tryCloudForFreePressed, q,
            &AccountManager::tryCloudForFree);
    connect(navigator, &Ui::AccountNavigator::renewCloudPressed, q, &AccountManager::renewCloud);
    connect(navigator, &Ui::AccountNavigator::buyCreditsPressed, q, &AccountManager::buyCredits);
    connect(navigator, &Ui::AccountNavigator::logoutPressed, q, [this] {
        q->clearAccountInfo();
        emit q->logoutRequested();
    });
    //
    connect(navigator, &Ui::AccountNavigator::createTeamPressed, q, [this] {
        auto dialog = new Ui::TeamDialog(view->topLevelWidget());
        dialog->setDialogType(Ui::TeamDialog::DialogType::CreateNew);
        connect(dialog, &Ui::TeamDialog::savePressed, view, [this, dialog] {
            if (dialog->teamName().isEmpty()) {
                dialog->setTeamNameError(tr("The team should have a name"));
                return;
            }

            //
            // Отправляем уведомление о создании новой команды
            //
            emit q->createTeamRequested(dialog->teamName(), dialog->teamDescription(),
                                        avatarData(dialog->teamAvatar()));

            dialog->hideDialog();
        });
        connect(dialog, &Ui::TeamDialog::disappeared, dialog, &Ui::TeamDialog::deleteLater);

        //
        // Отображаем диалог
        //
        dialog->showDialog();
    });
    connect(navigator, &Ui::AccountNavigator::editTeamPressed, q, [this](int _teamId) {
        Domain::TeamInfo team;
        for (const auto& accountTeam : std::as_const(accountTeams)) {
            if (accountTeam.id == _teamId) {
                team = accountTeam;
                break;
            }
        }
        if (!team.isValid()) {
            return;
        }

        auto dialog = new Ui::TeamDialog(view->topLevelWidget());
        dialog->setDialogType(Ui::TeamDialog::DialogType::Edit);
        dialog->setTeamName(team.name);
        dialog->setteamDescription(team.description);
        dialog->setTeamAvatar(ImageHelper::imageFromBytes(team.avatar));
        connect(dialog, &Ui::TeamDialog::savePressed, view, [this, team, dialog] {
            if (dialog->teamName().isEmpty()) {
                dialog->setTeamNameError(tr("The team should have a name"));
                return;
            }

            //
            // Отправляем уведомление об обновлении команды
            //
            emit q->updateTeamRequested(team.id, dialog->teamName(), dialog->teamDescription(),
                                        avatarData(dialog->teamAvatar()));

            dialog->hideDialog();
        });
        connect(dialog, &Ui::TeamDialog::disappeared, dialog, &Ui::TeamDialog::deleteLater);

        //
        // Отображаем диалог
        //
        dialog->showDialog();
    });
    connect(navigator, &Ui::AccountNavigator::removeTeamPressed, q, [this](int _teamId) {
        auto dialog = new Dialog(view->topLevelWidget());
        constexpr int cancelButtonId = 0;
        constexpr int removeButtonId = 1;
        dialog->showDialog({}, tr("Do you really want to remove this team?"),
                           { { cancelButtonId, tr("No"), Dialog::RejectButton },
                             { removeButtonId, tr("Yes, remove"), Dialog::AcceptButton } });
        connect(dialog, &Dialog::finished, q,
                [this, _teamId, cancelButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
                    dialog->hideDialog();

                    //
                    // Пользователь передумал выходить
                    //
                    if (_buttonInfo.id == cancelButtonId) {
                        return;
                    }
                    //
                    // Если таки хочет, то уведомляем, чтобы отписаться от
                    // команды на сервере
                    //
                    emit q->removeTeamRequested(_teamId);
                });
        connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
    });
    connect(navigator, &Ui::AccountNavigator::exitTeamPressed, q, [this](int _teamId) {
        auto dialog = new Dialog(view->topLevelWidget());
        constexpr int cancelButtonId = 0;
        constexpr int exitButtonId = 1;
        dialog->showDialog({}, tr("Do you really want to leave this team?"),
                           { { cancelButtonId, tr("No"), Dialog::RejectButton },
                             { exitButtonId, tr("Yes, leave"), Dialog::AcceptButton } });
        connect(dialog, &Dialog::finished, q,
                [this, _teamId, cancelButtonId, dialog](const Dialog::ButtonInfo& _buttonInfo) {
                    dialog->hideDialog();

                    //
                    // Пользователь передумал выходить
                    //
                    if (_buttonInfo.id == cancelButtonId) {
                        return;
                    }
                    //
                    // Если таки хочет, то уведомляем, чтобы отписаться от
                    // команды на сервере
                    //
                    emit q->exitFromTeamRequested(_teamId);
                });
        connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
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
                    accountInfo.avatar = avatarData(_avatar);
                } else {
                    accountInfo.avatar = {};
                }

                notifyUpdateAccountInfoRequested();
            });

    connect(view, &Ui::AccountView::tryProForFreePressed, q, &AccountManager::tryProForFree);
    connect(view, &Ui::AccountView::tryCloudForFreePressed, q, &AccountManager::tryCloudForFree);
    connect(view, &Ui::AccountView::buyProLifetimePressed, q, &AccountManager::buyProLifetme);
    connect(view, &Ui::AccountView::renewProPressed, q, &AccountManager::renewPro);
    connect(view, &Ui::AccountView::giftProPressed, q, &AccountManager::giftPro);
    connect(view, &Ui::AccountView::renewCloudPressed, q, &AccountManager::renewCloud);
    connect(view, &Ui::AccountView::giftCloudPressed, q, &AccountManager::giftCloud);
    connect(view, &Ui::AccountView::activatePromocodePressed, q,
            &AccountManager::activatePromocodeRequested);

    connect(view, &Ui::AccountView::terminateSessionRequested, q,
            &AccountManager::terminateSessionRequested);

    connect(view, &Ui::AccountView::addMemberRequested, q,
            [this](int _teamId, const QString& _email, const QString& _nameForTeam) {
                constexpr int memberRole = 1;
                emit q->addMemberRequested(_teamId, _email, _nameForTeam, memberRole);
            });
    connect(view, &Ui::AccountView::changeMemberRequested, q,
            &AccountManager::changeMemberRequested);
    connect(view, &Ui::AccountView::removeMemberRequested, q,
            &AccountManager::removeMemberRequested);
}

void AccountManager::Implementation::initCrossConnections()
{
    connect(navigator, &Ui::AccountNavigator::teamSelected, view, &Ui::AccountView::showTeam);
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
        connect(purchaseDialog, &Ui::PurchaseDialog::giftPressed, q,
                [this](const Domain::PaymentOption& _option) {
                    initPurchaseGiftDialog();
                    purchaseGiftDialog->setPaymentOption(_option);
                    purchaseGiftDialog->showDialog();
                });
        connect(purchaseDialog, &Ui::PurchaseDialog::canceled, purchaseDialog,
                &Ui::PurchaseDialog::hideDialog);
        connect(purchaseDialog, &Ui::PurchaseDialog::disappeared, purchaseDialog, [this] {
            purchaseDialog->deleteLater();
            purchaseDialog = nullptr;
        });
    }

    purchaseDialog->setPaymentOptions(accountInfo.paymentOptions);
}

void AccountManager::Implementation::initPurchaseGiftDialog()
{
    if (purchaseGiftDialog == nullptr) {
        purchaseGiftDialog = new Ui::PurchaseGiftDialog(view->topLevelWidget());
        connect(purchaseGiftDialog, &Ui::PurchaseGiftDialog::purchasePressed, q,
                &AccountManager::activatePaymentOptionAsGiftRequested);
        connect(purchaseGiftDialog, &Ui::PurchaseGiftDialog::canceled, purchaseGiftDialog,
                &Ui::PurchaseGiftDialog::hideDialog);
        connect(purchaseGiftDialog, &Ui::PurchaseGiftDialog::disappeared, purchaseGiftDialog,
                [this] {
                    purchaseGiftDialog->deleteLater();
                    purchaseGiftDialog = nullptr;
                });
    }
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

void AccountManager::setAuthorizationError(const QString& _error)
{
    if (d->loginDialog != nullptr) {
        d->loginDialog->setAuthorizationError(_error);
    }
}

void AccountManager::setConfirmationCodeInfo(int _codeLength)
{
    d->confirmationCodeLength = _codeLength;
    if (d->loginDialog != nullptr) {
        d->loginDialog->showConfirmationCodeStep();
    }
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

const Domain::AccountInfo& AccountManager::accountInfo() const
{
    return d->accountInfo;
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
    d->view->setSubscriptions(d->accountInfo.subscriptions);
    d->view->setPaymentOptions(d->accountInfo.paymentOptions);
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
    if (!d->accountInfo.subscriptions.isEmpty()) {
        const auto& subscription = d->accountInfo.subscriptions.constLast();
        if (currentDateTime < subscription.end) {
            //
            // Т.к. таймер запускается на int миллисекунд, нельзя в него передавать слишком большое
            // значение (количество миллисекунд до самого конца подписки), выходящее за INT_MAX
            //
            const qint64 maxInterval = INT_MAX;
            const auto interval = std::min(currentDateTime.msecsTo(subscription.end), maxInterval);
            d->subscriptionEndsTimer.start(interval);
        }
    }

    //
    // Если информация обновилась во время оплаты лицензии, то закроем диалог платежей
    //
    if (d->purchaseDialog != nullptr && d->purchaseDialog->isVisible()) {
        d->purchaseDialog->hideDialog();
    }
    if (d->purchaseGiftDialog != nullptr && d->purchaseGiftDialog->isVisible()) {
        d->purchaseGiftDialog->hideDialog();
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

void AccountManager::upgradeAccountToCloud()
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
        const auto canBeUpdatedForFree = tryCloudForFree();
        if (canBeUpdatedForFree) {
            return;
        }

        renewCloud();
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
    auto proPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = proPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = proPaymentOptions.at(index);
        if (option.amount == 0
            || (option.subscriptionType != Domain::SubscriptionType::ProMonthly
                && option.subscriptionType != Domain::SubscriptionType::ProLifetime)) {
            proPaymentOptions.removeAt(index);
        }
    }

    const int discount = proPaymentOptions.constFirst().discount;
    const auto discountInfo = discount > 0
        ? tr("You have an additional %1% discount due to the promo code activation").arg(discount)
        : "";

    d->initPurchaseDialog();
    d->purchaseDialog->setDiscountInfo(discountInfo);
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

    const int discount = proPaymentOptions.constFirst().discount;
    const auto discountInfo = discount > 0
        ? tr("You have an additional %1% discount due to the promo code activation").arg(discount)
        : "";

    d->initPurchaseDialog();
    d->purchaseDialog->setDiscountInfo(discountInfo);
    d->purchaseDialog->setPaymentOptions(proPaymentOptions);
    d->purchaseDialog->selectOption(proPaymentOptions.constLast());
    d->purchaseDialog->showDialog();
}

void AccountManager::giftPro()
{
    auto proPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = proPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = proPaymentOptions.at(index);
        if (option.amount == 0
            || (option.subscriptionType != Domain::SubscriptionType::ProMonthly
                && option.subscriptionType != Domain::SubscriptionType::ProLifetime)) {
            proPaymentOptions.removeAt(index);
        }
    }
    if (proPaymentOptions.isEmpty()) {
        return;
    }

    const int discount = proPaymentOptions.constFirst().discount;
    const auto discountInfo = discount > 0
        ? tr("You have an additional %1% discount due to the promo code activation").arg(discount)
        : "";

    d->initPurchaseDialog();
    d->purchaseDialog->setDiscountInfo(discountInfo);
    d->purchaseDialog->setPaymentOptions(proPaymentOptions);
    d->purchaseDialog->setPurchaseAvailable(false);
    d->purchaseDialog->selectOption(proPaymentOptions.constFirst());
    d->purchaseDialog->showDialog();
}

bool AccountManager::tryCloudForFree()
{
    //
    // Ищем бесплатную активацию
    //
    Domain::PaymentOption freeOption;
    for (const auto& paymentOption : std::as_const(d->accountInfo.paymentOptions)) {
        if (paymentOption.amount != 0
            || paymentOption.subscriptionType != Domain::SubscriptionType::CloudMonthly) {
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
            tr("Try CLOUD version for free"),
            tr("You can try all the features of the CLOUD version during 30 days for free. After "
               "trial period, you can continue to use the CLOUD version by renewing your "
               "subscription. Otherwise, you'll be returned to the FREE version automatically."),
            { { 0, tr("Continue with FREE version"), Dialog::RejectButton },
              { 1, tr("Activate CLOUD"), Dialog::AcceptButton } });
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

void AccountManager::renewCloud()
{
    auto cloudPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = cloudPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = cloudPaymentOptions.at(index);
        if (option.amount == 0 || option.type != Domain::PaymentType::Subscription
            || (option.subscriptionType != Domain::SubscriptionType::CloudMonthly
                && option.subscriptionType != Domain::SubscriptionType::CloudLifetime)) {
            cloudPaymentOptions.removeAt(index);
        }
    }
    if (cloudPaymentOptions.isEmpty()) {
        return;
    }

    const auto hasProLifetime
        = std::find_if(d->accountInfo.subscriptions.begin(), d->accountInfo.subscriptions.end(),
                       [](const Domain::SubscriptionInfo& _subscription) {
                           return _subscription.type == Domain::SubscriptionType::ProLifetime;
                       });
    const int discount = cloudPaymentOptions.constFirst().discount;
    QString discountInfo;
    if (hasProLifetime) {
        if (discount > 20) {
            discountInfo
                = tr("You have an additional 20% discount due to PRO lifetime subscription "
                     "purchase, and %1% discount due to the promo code activation")
                      .arg(discount - 20);
        } else {
            discountInfo = tr("You have an additional 20% discount due to the purchasing of the "
                              "PRO lifetime subscription");
        }
    } else if (discount > 0) {
        discountInfo = tr("You have an additional %1% discount due to the promo code activation")
                           .arg(discount);
    }

    d->initPurchaseDialog();
    d->purchaseDialog->setDiscountInfo(discountInfo);
    d->purchaseDialog->setPaymentOptions(cloudPaymentOptions);
    d->purchaseDialog->selectOption(cloudPaymentOptions.constLast());
    d->purchaseDialog->showDialog();
}

void AccountManager::giftCloud()
{
    auto cloudPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = cloudPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = cloudPaymentOptions.at(index);
        if (option.amount == 0 || option.type != Domain::PaymentType::Subscription
            || (option.subscriptionType != Domain::SubscriptionType::CloudMonthly
                && option.subscriptionType != Domain::SubscriptionType::CloudLifetime)) {
            cloudPaymentOptions.removeAt(index);
        }
    }
    if (cloudPaymentOptions.isEmpty()) {
        return;
    }

    const auto hasProLifetime
        = std::find_if(d->accountInfo.subscriptions.begin(), d->accountInfo.subscriptions.end(),
                       [](const Domain::SubscriptionInfo& _subscription) {
                           return _subscription.type == Domain::SubscriptionType::ProLifetime;
                       });
    const int discount = cloudPaymentOptions.constFirst().discount;
    QString discountInfo;
    if (hasProLifetime) {
        if (discount > 20) {
            discountInfo
                = tr("You have an additional 20% discount due to PRO lifetime subscription "
                     "purchase, and %1% discount due to the promo code activation")
                      .arg(discount - 20);
        } else {
            discountInfo = tr("You have an additional 20% discount due to the purchasing of the "
                              "PRO lifetime subscription");
        }
    } else if (discount > 0) {
        discountInfo = tr("You have an additional %1% discount due to the promo code activation")
                           .arg(discount);
    }

    d->initPurchaseDialog();
    d->purchaseDialog->setDiscountInfo(discountInfo);
    d->purchaseDialog->setPaymentOptions(cloudPaymentOptions);
    d->purchaseDialog->setPurchaseAvailable(false);
    d->purchaseDialog->selectOption(cloudPaymentOptions.constLast());
    d->purchaseDialog->showDialog();
}

void AccountManager::buyCredits()
{
    //
    // Если ещё не авторизован, то отправим на авторизацию
    //
    if (d->accountInfo.email.isEmpty()) {
        signIn();
        return;
    }

    auto creditPaymentOptions = d->accountInfo.paymentOptions;
    for (int index = creditPaymentOptions.size() - 1; index >= 0; --index) {
        const auto& option = creditPaymentOptions.at(index);
        if (option.type != Domain::PaymentType::Credits) {
            creditPaymentOptions.removeAt(index);
        }
    }
    if (creditPaymentOptions.isEmpty()) {
        return;
    }

    const int discount = creditPaymentOptions.constFirst().discount;
    const auto discountInfo = discount > 0
        ? tr("You have an additional %1% discount due to the promo code activation").arg(discount)
        : "";

    d->initPurchaseDialog();
    d->purchaseDialog->setDescription(
        tr("Credits are our internal currency. They are used for AI tools, such as text "
           "generation.\n\n1 credit equals 1000 words processed by AI.\n1 credit equals 10 images "
           "generated by AI."));
    d->purchaseDialog->setDiscountInfo(discountInfo);
    d->purchaseDialog->setPaymentOptions(creditPaymentOptions);
    d->purchaseDialog->selectOption(creditPaymentOptions.constLast());
    d->purchaseDialog->showDialog();
}

void AccountManager::showGiftSentMessage(const QString& _message)
{
    StandardDialog::information(d->view->topLevelWidget(), {}, _message);

    d->initPurchaseGiftDialog();
    d->purchaseGiftDialog->hideDialog();
    d->initPurchaseDialog();
    d->purchaseDialog->hideDialog();
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

void AccountManager::setAccountTeams(const QVector<Domain::TeamInfo>& _teams)
{
    d->accountTeams = _teams;

    d->navigator->setAccountTeams(d->accountTeams);
    d->view->setAccountTeams(d->accountTeams);
}

void AccountManager::addAccountTeam(const Domain::TeamInfo& _team)
{
    for (auto& team : d->accountTeams) {
        if (team.id == _team.id) {
            updateAccountTeam(_team);
            return;
        }
    }

    d->accountTeams.append(_team);

    d->navigator->setAccountTeams(d->accountTeams);
    d->view->setAccountTeams(d->accountTeams);
}

void AccountManager::updateAccountTeam(const Domain::TeamInfo& _team)
{
    for (auto& team : d->accountTeams) {
        if (team.id == _team.id) {
            team = _team;
            break;
        }
    }

    d->navigator->setAccountTeams(d->accountTeams);
    d->view->setAccountTeams(d->accountTeams);
}

void AccountManager::removeAccountTeam(int _teamId)
{
    d->accountTeams.erase(std::remove_if(
        d->accountTeams.begin(), d->accountTeams.end(),
        [_teamId](const Domain::TeamInfo& _accountTeam) { return _accountTeam.id == _teamId; }));

    d->navigator->setAccountTeams(d->accountTeams);
    d->view->setAccountTeams(d->accountTeams);
}

} // namespace ManagementLayer

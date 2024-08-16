#include "account_view.h"

#include "account_view_teams.h"
#include "session_widget.h"
#include "subscription_widget.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/check_box/check_box.h>
#include <ui/widgets/floating_tool_bar/floating_tool_bar.h>
#include <ui/widgets/image/image_card.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/label/link_label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <utils/helpers/color_helper.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QGridLayout>
#include <QScrollArea>
#include <QVariantAnimation>


namespace Ui {

class AccountView::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Проскролить представление до заданного виджета
     */
    void scrollToTitle(AbstractLabel* title);


    //
    // Страница аккаунта
    //

    Widget* accountPage = nullptr;
    QScrollArea* accountContent = nullptr;
    QVariantAnimation scrollAnimation;

    QVariantAnimation colorAnimation;
    AbstractLabel* colorableTitle = nullptr;

    Card* accountInfo = nullptr;
    QGridLayout* accountInfoLayout = nullptr;
    int accountInfoLastRow = 0;
    H6Label* accountTitle = nullptr;
    TextField* name = nullptr;
    Debouncer changeNameDebouncer{ 500 };
    TextField* description = nullptr;
    CheckBox* newsletterSubscription = nullptr;
    Debouncer changeDescriptionDebouncer{ 500 };
    ImageCard* avatar = nullptr;

    H5Label* subscriptionsTitle = nullptr;
    Body1LinkLabel* compareSubscriptions = nullptr;
    SubscriptionWidget* proSubscription = nullptr;
    SubscriptionWidget* cloudSubscription = nullptr;

    Card* promocodeInfo = nullptr;
    QGridLayout* promocodeInfoLayout = nullptr;
    TextField* promocodeName = nullptr;
    Button* activatePromocode = nullptr;

    H5Label* sessionsTitle = nullptr;
    QVector<SessionWidget*> sessions;

    //
    // Страница команд
    //

    AccountViewTeams* teamPage = nullptr;
};

AccountView::Implementation::Implementation(QWidget* _parent)
    : accountPage(new Widget(_parent))
    //
    , accountContent(new QScrollArea(accountPage))
    , accountInfo(new Card(accountPage))
    , accountInfoLayout(new QGridLayout)
    , accountTitle(new H6Label(accountInfo))
    , name(new TextField(accountInfo))
    , description(new TextField(accountInfo))
    , newsletterSubscription(new CheckBox(accountInfo))
    , avatar(new ImageCard(accountInfo))
    //
    , subscriptionsTitle(new H5Label(accountPage))
    , compareSubscriptions(new Body1LinkLabel(accountPage))
    , proSubscription(new SubscriptionWidget(accountPage))
    , cloudSubscription(new SubscriptionWidget(accountPage))
    //
    , promocodeInfo(new Card(accountPage))
    , promocodeInfoLayout(new QGridLayout)
    , promocodeName(new TextField(promocodeInfo))
    , activatePromocode(new Button(promocodeInfo))
    //
    , sessionsTitle(new H5Label(accountPage))
    //
    //
    , teamPage(new AccountViewTeams(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    accountContent->setPalette(palette);
    accountContent->setFrameShape(QFrame::NoFrame);
    accountContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    accountContent->setVerticalScrollBar(new ScrollBar);
    scrollAnimation.setEasingCurve(QEasingCurve::OutQuad);
    scrollAnimation.setDuration(180);
    colorAnimation.setEasingCurve(QEasingCurve::InBack);
    colorAnimation.setDuration(1400);

    name->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    name->setCapitalizeWords(false);
    description->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    avatar->setDecorationIcon(u8"\U000F0004");
    //
    accountInfoLayout->setContentsMargins({});
    accountInfoLayout->setSpacing(0);
    int row = 0;
    accountInfoLayout->addWidget(accountTitle, row++, 0);
    accountInfoLayout->addWidget(name, row++, 0);
    accountInfoLayout->addWidget(description, row++, 0);
    accountInfoLayout->addWidget(newsletterSubscription, row++, 0);
    accountInfoLastRow = row;
    accountInfoLayout->setRowMinimumHeight(accountInfoLastRow,
                                           1); // добавляем пустую строку, вместо отступа снизу
    accountInfo->setContentLayout(accountInfoLayout);
    //
    promocodeName->setCapitalizeWords(false);
    promocodeInfoLayout->addWidget(promocodeName, 0, 0);
    promocodeInfoLayout->addWidget(activatePromocode, 0, 1, Qt::AlignBottom);
    promocodeInfoLayout->setColumnStretch(0, 1);
    promocodeInfo->setContentLayout(promocodeInfoLayout);

    auto accountContentWidget = new QWidget;
    accountContent->setWidget(accountContentWidget);
    accountContent->setWidgetResizable(true);
    auto accountContentLayout = new QGridLayout;
    accountContentLayout->setContentsMargins({});
    accountContentLayout->setSpacing(0);
    row = 0;
    accountContentLayout->addWidget(accountInfo, row, 0, 1, 2);
    accountContentLayout->addWidget(avatar, row++, 2, 3, 1, Qt::AlignTop);
    accountContentLayout->addWidget(subscriptionsTitle, row, 0);
    accountContentLayout->addWidget(compareSubscriptions, row++, 1,
                                    Qt::AlignRight | Qt::AlignBottom);
    accountContentLayout->addWidget(proSubscription, row, 0);
    accountContentLayout->addWidget(cloudSubscription, row++, 1);
    accountContentLayout->addWidget(promocodeInfo, row++, 0, 1, 2);
    accountContentLayout->addWidget(sessionsTitle, row++, 0, 1, 2);
    accountContentLayout->setColumnStretch(0, 1);
    accountContentLayout->setColumnStretch(1, 1);
    accountContentWidget->setLayout(accountContentLayout);

    auto accountLayout = new QVBoxLayout;
    accountLayout->setContentsMargins({});
    accountLayout->setSpacing(0);
    accountLayout->addWidget(accountContent);
    accountPage->setLayout(accountLayout);
}

void AccountView::Implementation::scrollToTitle(AbstractLabel* title)
{
    const QRect microFocus = title->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    const QRect defaultMicroFocus
        = title->QWidget::inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QRect focusRect = (microFocus != defaultMicroFocus)
        ? QRect(title->mapTo(accountContent->widget(), microFocus.topLeft()), microFocus.size())
        : QRect(title->mapTo(accountContent->widget(), QPoint(0, 0)), title->size());

    focusRect.adjust(-50, -50, 50, 50);

    scrollAnimation.setStartValue(accountContent->verticalScrollBar()->value());
    scrollAnimation.setEndValue(focusRect.top());
    scrollAnimation.start();

    colorAnimation.stop();
    if (colorableTitle != nullptr) {
        colorableTitle->setTextColor(colorAnimation.endValue().value<QColor>());
    }
    colorableTitle = title;
    colorAnimation.setEndValue(colorableTitle->textColor());
    colorableTitle->setTextColor(colorAnimation.startValue().value<QColor>());
    colorAnimation.start();
}


// ****


AccountView::AccountView(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::FadeThrough);
    setCurrentWidget(d->accountPage);
    addWidget(d->teamPage);


    connect(&d->scrollAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                d->accountContent->verticalScrollBar()->setValue(_value.toInt());
            });
    connect(&d->colorAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                if (d->colorableTitle != nullptr) {
                    d->colorableTitle->setTextColor(_value.value<QColor>());
                }
            });
    //
    // Аккаунт
    //
    connect(d->name, &TextField::textChanged, &d->changeNameDebouncer, &Debouncer::orderWork);
    connect(&d->changeNameDebouncer, &Debouncer::gotWork, this, [this] {
        if (d->name->text().isEmpty()) {
            d->name->setError(tr("Username can't be empty, please fill it"));
            return;
        }

        d->name->setError({});
        emit nameChanged(d->name->text());
    });
    //
    connect(d->description, &TextField::textChanged, &d->changeDescriptionDebouncer,
            &Debouncer::orderWork);
    connect(&d->changeDescriptionDebouncer, &Debouncer::gotWork, this,
            [this] { emit descriptionChanged(d->description->text()); });
    //
    connect(d->newsletterSubscription, &CheckBox::checkedChanged, this,
            &AccountView::newsletterSubscriptionChanged);
    //
    connect(d->avatar, &ImageCard::imageChanged, this, &AccountView::avatarChanged);

    //
    // Подписка
    //
    connect(d->proSubscription, &SubscriptionWidget::tryPressed, this,
            &AccountView::tryProForFreePressed);
    connect(d->proSubscription, &SubscriptionWidget::buyPressed, this,
            &AccountView::renewProPressed);
    connect(d->proSubscription, &SubscriptionWidget::buyLifetimePressed, this,
            &AccountView::buyProLifetimePressed);
    connect(d->proSubscription, &SubscriptionWidget::giftPressed, this,
            &AccountView::giftProPressed);
    connect(d->cloudSubscription, &SubscriptionWidget::tryPressed, this,
            &AccountView::tryCloudForFreePressed);
    connect(d->cloudSubscription, &SubscriptionWidget::buyPressed, this,
            &AccountView::renewCloudPressed);
    connect(d->cloudSubscription, &SubscriptionWidget::giftPressed, this,
            &AccountView::giftCloudPressed);

    connect(d->promocodeName, &TextField::textChanged, d->promocodeName,
            [this] { d->promocodeName->setError({}); });
    connect(d->promocodeName, &TextField::enterPressed, d->activatePromocode, &Button::click);
    connect(d->activatePromocode, &Button::clicked, this, [this] {
        if (d->promocodeName->text().isEmpty()) {
            return;
        }

        emit activatePromocodePressed(d->promocodeName->text());
    });

    //
    // Команды
    //
    connect(d->teamPage, &AccountViewTeams::addMemberPressed, this,
            &AccountView::addMemberRequested);
    connect(d->teamPage, &AccountViewTeams::changeMemberRequested, this,
            &AccountView::changeMemberRequested);
    connect(d->teamPage, &AccountViewTeams::removeMemberPressed, this,
            &AccountView::removeMemberRequested);
}

AccountView::~AccountView() = default;

void AccountView::showAccountPage()
{
    setCurrentWidget(d->accountPage);
}

void AccountView::showTeamPage()
{
    setCurrentWidget(d->teamPage);
}

void AccountView::showAccount()
{
    d->scrollToTitle(d->accountTitle);
}

void AccountView::showSubscription()
{
    d->scrollToTitle(d->subscriptionsTitle);
}

void AccountView::showSessions()
{
    d->scrollToTitle(d->sessionsTitle);
}

void AccountView::setConnected(bool _connected)
{
    d->name->setEnabled(_connected);
    d->description->setEnabled(_connected);
    d->newsletterSubscription->setEnabled(_connected);
    d->avatar->setEnabled(_connected);
    d->proSubscription->setEnabled(_connected);
    d->cloudSubscription->setEnabled(_connected);
    d->promocodeName->setEnabled(_connected);
    d->activatePromocode->setEnabled(_connected);
    //
    d->teamPage->setConnected(_connected);
}

void AccountView::setEmail(const QString& _email)
{
    d->accountTitle->setText(_email);
}

void AccountView::setName(const QString& _name)
{
    if (d->changeNameDebouncer.hasPendingWork() || d->name->text() == _name) {
        return;
    }

    QSignalBlocker blocker(d->name);
    d->name->setText(_name);
}

void AccountView::setDescription(const QString& _description)
{
    if (d->changeDescriptionDebouncer.hasPendingWork() || d->description->text() == _description) {
        return;
    }

    QSignalBlocker blocker(d->description);
    d->description->setText(_description);
}

void AccountView::setNewsletterSubscribed(bool _subscribed)
{
    if (d->newsletterSubscription->isChecked() == _subscribed) {
        return;
    }

    QSignalBlocker blocker(d->newsletterSubscription);
    d->newsletterSubscription->setChecked(_subscribed);
}

void AccountView::setAvatar(const QPixmap& _avatar)
{
    QSignalBlocker blocker(d->avatar);
    d->avatar->setImage(_avatar);
}

void AccountView::setAccountInfo(const Domain::AccountInfo& _account)
{
    d->teamPage->setAccountInfo(_account);
}

void AccountView::setSubscriptions(const QVector<Domain::SubscriptionInfo>& _subscriptions)
{
    bool isActive = true;
    bool isLifetime = true;
    d->proSubscription->setStatus(!isActive, !isLifetime, {});
    d->cloudSubscription->setStatus(!isActive, !isLifetime, {});

    for (const auto& subscription : _subscriptions) {
        switch (subscription.type) {
        case Domain::SubscriptionType::ProMonthly: {
            d->proSubscription->setStatus(isActive, !isLifetime, subscription.end);
            break;
        }
        case Domain::SubscriptionType::ProLifetime: {
            d->proSubscription->setStatus(isActive, isLifetime, {});
            break;
        }
        case Domain::SubscriptionType::CloudMonthly: {
            d->cloudSubscription->setStatus(isActive, !isLifetime, subscription.end);
            break;
        }
        case Domain::SubscriptionType::CloudLifetime: {
            d->cloudSubscription->setStatus(isActive, isLifetime, {});
            //
            // Когда активна клауд, считаем, что и про тоже активна навсегда
            //
            d->proSubscription->setStatus(isActive, isLifetime, {});
            break;
        }
        default: {
            break;
        }
        }
    }
}

void AccountView::setPaymentOptions(const QVector<Domain::PaymentOption>& _paymentOptions)
{
    QVector<Domain::PaymentOption> pro, cloud;
    for (const auto& option : _paymentOptions) {
        if (option.subscriptionType == Domain::SubscriptionType::ProMonthly
            || option.subscriptionType == Domain::SubscriptionType::ProLifetime) {
            pro.append(option);
        } else if (option.subscriptionType == Domain::SubscriptionType::CloudMonthly
                   || option.subscriptionType == Domain::SubscriptionType::CloudLifetime) {
            cloud.append(option);
        }
    }
    d->proSubscription->setPaymentOptions(pro);
    d->cloudSubscription->setPaymentOptions(cloud);
}

void AccountView::clearPromocode()
{
    d->promocodeName->clear();
}

void AccountView::setPromocodeError(const QString& _error)
{
    d->promocodeName->setError(_error);
    d->promocodeName->selectAll();
}

void AccountView::setSessions(const QVector<Domain::SessionInfo>& _sessions)
{
    while (!d->sessions.isEmpty()) {
        d->sessions.takeLast()->deleteLater();
    }

    auto layout = qobject_cast<QGridLayout*>(d->accountContent->widget()->layout());
    int row = layout->rowCount() - 1;
    int column = 0;
    for (const auto& sessionInfo : _sessions) {
        auto sessionWidget = new SessionWidget(d->accountContent->widget());
        sessionWidget->setSessionInfo(sessionInfo);
        if (_sessions.size() == 1) {
            sessionWidget->hideTerminateButton();
        }

        connect(sessionWidget, &SessionWidget::terminateOthersRequested, this,
                [this, sessionInfo, _sessions] {
                    for (const auto& session : _sessions) {
                        if (session.sessionKey == sessionInfo.sessionKey) {
                            continue;
                        }

                        emit terminateSessionRequested(session.sessionKey);
                    }
                });
        connect(sessionWidget, &SessionWidget::terminateRequested, this,
                [this, sessionInfo] { emit terminateSessionRequested(sessionInfo.sessionKey); });

        layout->addWidget(sessionWidget, row, column++);
        layout->setRowStretch(row, 0);
        if (column > 1) {
            ++row;
            column = 0;
        }

        d->sessions.append(sessionWidget);
    }

    //
    // Растягиваем строку под сессиями, чтобы нижний из виджетов сессий не растягивался
    //
    if (column != 0) {
        ++row;
    }
    layout->setRowStretch(row, 1);
}

void AccountView::setAccountTeams(const QVector<Domain::TeamInfo>& _teams)
{
    d->teamPage->setTeams(_teams);

    if (_teams.isEmpty()) {
        d->teamPage->showEmptyPage();
    }
}

void AccountView::showTeam(int _teamId)
{
    d->teamPage->showTeam(_teamId);
}

void AccountView::updateTranslations()
{
    const auto isRussianSpeaking = QLocale().language() == QLocale::Russian
        || QLocale().language() == QLocale::Belarusian
        || QLocale().language() == QLocale::Ukrainian;

    d->name->setLabel(tr("Your name"));
    d->description->setLabel(tr("Your bio"));
    d->newsletterSubscription->setText(tr("I want to receive STARC news"));
    d->avatar->setSupportingText(tr("Add avatar +"), tr("Change avatar..."),
                                 tr("Do you want to delete your avatar?"));
    d->avatar->setImageCroppingText(tr("Select an area for the avatar"));
    d->subscriptionsTitle->setText(tr("Subscriptions"));
    d->compareSubscriptions->setText(tr("Compare subscriptions"));
    d->compareSubscriptions->setLink(
        QUrl(isRussianSpeaking ? "https://starc.app/ru/pricing/" : "https://starc.app/pricing/"));
    d->proSubscription->setInfo("PRO",
                                tr("Advanced tools for professionals\n"
                                   "• Characters relations\n"
                                   "• Corkboard\n"
                                   "• Timeline\n"
                                   "• Mind maps\n"
                                   "• and more writer's tools..."));
    d->cloudSubscription->setInfo("CLOUD",
                                  tr("For those who are always on the move\n"
                                     "• 5GB cloud storage\n"
                                     "• Seamless synchronization across all your devices\n"
                                     "• Realtime collaboration\n"
                                     "• Production tools"));
    d->sessionsTitle->setText(tr("Active sessions"));
    d->promocodeName->setLabel(tr("Promotional or gift code"));
    d->activatePromocode->setText(tr("Activate"));
}

void AccountView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());

    for (auto page : std::vector<Widget*>{
             d->accountPage,
             d->teamPage,
         }) {
        page->setBackgroundColor(DesignSystem::color().surface());
    }

    d->accountContent->widget()->layout()->setContentsMargins(
        QMarginsF(DesignSystem::layout().px24(), DesignSystem::compactLayout().topContentMargin(),
                  DesignSystem::layout().px24(), DesignSystem::compactLayout().px24())
            .toMargins());

    d->colorAnimation.setStartValue(DesignSystem::color().accent());

    for (auto card : {
             d->accountInfo,
             d->promocodeInfo,
         }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    auto titleMargins = DesignSystem::label().margins().toMargins();
    titleMargins.setBottom(0);
    for (auto title : {
             d->accountTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().background());
        title->setTextColor(DesignSystem::color().onBackground());
        title->setContentsMargins(titleMargins);
    }

    for (auto title : {
             d->subscriptionsTitle,
             d->sessionsTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().surface());
        title->setTextColor(DesignSystem::color().onSurface());
        title->setContentsMargins(titleMargins);
    }

    auto labelMargins = DesignSystem::label().margins().toMargins();
    labelMargins.setTop(0);
    labelMargins.setBottom(0);
    for (auto subtitle : std::vector<Widget*>{
             d->compareSubscriptions,
         }) {
        subtitle->setContentsMargins(labelMargins);
        subtitle->setBackgroundColor(DesignSystem::color().surface());
        subtitle->setTextColor(ColorHelper::transparent(DesignSystem::color().onBackground(),
                                                        DesignSystem::inactiveTextOpacity()));
    }

    for (auto textField : {
             d->name,
             d->description,
             d->promocodeName,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setTextColor(DesignSystem::color().onBackground());
    }

    d->name->setCustomMargins({ DesignSystem::layout().px24(), DesignSystem::compactLayout().px24(),
                                DesignSystem::layout().px24(),
                                DesignSystem::compactLayout().px16() });

    d->newsletterSubscription->setBackgroundColor(DesignSystem::color().background());
    d->newsletterSubscription->setTextColor(DesignSystem::color().onBackground());

    for (auto button : {
             d->activatePromocode,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    d->avatar->setBackgroundColor(DesignSystem::color().background());
    d->avatar->setTextColor(DesignSystem::color().onBackground());
    d->avatar->setFixedSize((QSizeF(288, 288) * DesignSystem::scaleFactor()).toSize());

    d->accountInfoLayout->setRowMinimumHeight(d->accountInfoLastRow,
                                              static_cast<int>(DesignSystem::layout().px8()));
    d->promocodeInfoLayout->setContentsMargins(0, DesignSystem::layout().px24(),
                                               DesignSystem::layout().px24(),
                                               DesignSystem::layout().px24());
}

} // namespace Ui

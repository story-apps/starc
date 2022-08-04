#include "account_view.h"

#include "session_widget.h"

#include <domain/account_info.h>
#include <domain/session_info.h>
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

    /**
     * @brief Обновить текст лейбла окончания подписки
     */
    void updateSubscriptionEndsLabel();


    QScrollArea* content = nullptr;
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

    Card* subscriptionInfo = nullptr;
    QGridLayout* subscriptionInfoLayout = nullptr;
    int subscriptionInfoLastRow = 0;
    H6Label* subscriptionTitle = nullptr;
    QDateTime subscriptionEnds;
    Subtitle2Label* subscriptionEndsLabel = nullptr;
    Body1LinkLabel* subscriptionDetails = nullptr;
    Button* subscriptionTryProForFree = nullptr;
    Button* subscriptionTryTeamForFree = nullptr;
    Button* subscriptionBuyProLifetime = nullptr;
    Button* subscriptionRenewPro = nullptr;
    Button* subscriptionRenewTeam = nullptr;
    Button* subscriptionUpgradeToPro = nullptr;
    Button* subscriptionUpgradeToTeam = nullptr;

    Card* promocodeInfo = nullptr;
    QGridLayout* promocodeInfoLayout = nullptr;
    TextField* promocodeName = nullptr;
    Button* activatePromocode = nullptr;

    H5Label* sessionsTitle = nullptr;
    QVector<SessionWidget*> sessions;
};

AccountView::Implementation::Implementation(QWidget* _parent)
    : content(new QScrollArea(_parent))
    , accountInfo(new Card(_parent))
    , accountInfoLayout(new QGridLayout)
    , accountTitle(new H6Label(accountInfo))
    , name(new TextField(accountInfo))
    , description(new TextField(accountInfo))
    , newsletterSubscription(new CheckBox(accountInfo))
    , avatar(new ImageCard(accountInfo))
    //
    , subscriptionInfo(new Card(_parent))
    , subscriptionInfoLayout(new QGridLayout)
    , subscriptionTitle(new H6Label(subscriptionInfo))
    , subscriptionEndsLabel(new Subtitle2Label(subscriptionInfo))
    , subscriptionDetails(new Body1LinkLabel(subscriptionInfo))
    , subscriptionTryProForFree(new Button(subscriptionInfo))
    , subscriptionTryTeamForFree(new Button(subscriptionInfo))
    , subscriptionBuyProLifetime(new Button(subscriptionInfo))
    , subscriptionRenewPro(new Button(subscriptionInfo))
    , subscriptionRenewTeam(new Button(subscriptionInfo))
    , subscriptionUpgradeToPro(new Button(subscriptionInfo))
    , subscriptionUpgradeToTeam(new Button(subscriptionInfo))
    //
    , promocodeInfo(new Card(_parent))
    , promocodeInfoLayout(new QGridLayout)
    , promocodeName(new TextField(promocodeInfo))
    , activatePromocode(new Button(promocodeInfo))
    //
    , sessionsTitle(new H5Label(_parent))
{
    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    content->setPalette(palette);
    content->setFrameShape(QFrame::NoFrame);
    content->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    content->setVerticalScrollBar(new ScrollBar);
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
    accountInfo->setLayoutReimpl(accountInfoLayout);

    subscriptionDetails->setLink(QUrl("https://starc.app/pricing"));
    //
    subscriptionInfoLayout->setContentsMargins({});
    subscriptionInfoLayout->setSpacing(0);
    row = 0;
    subscriptionInfoLayout->addWidget(subscriptionTitle, row++, 0);
    subscriptionInfoLayout->addWidget(subscriptionEndsLabel, row++, 0);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(subscriptionDetails);
        layout->addStretch();
        layout->addWidget(subscriptionTryProForFree);
        layout->addWidget(subscriptionTryTeamForFree);
        layout->addWidget(subscriptionBuyProLifetime);
        layout->addWidget(subscriptionRenewPro);
        layout->addWidget(subscriptionRenewTeam);
        layout->addWidget(subscriptionUpgradeToPro);
        layout->addWidget(subscriptionUpgradeToTeam);
        subscriptionInfoLayout->addLayout(layout, row++, 0);
    }
    subscriptionInfoLastRow = row;
    subscriptionInfoLayout->setRowMinimumHeight(subscriptionInfoLastRow,
                                                1); // добавляем пустую строку, вместо отступа снизу
    subscriptionInfo->setLayoutReimpl(subscriptionInfoLayout);
    //
    promocodeInfoLayout->addWidget(promocodeName, 0, 0);
    promocodeInfoLayout->addWidget(activatePromocode, 0, 1, Qt::AlignBottom);
    promocodeInfoLayout->setColumnStretch(0, 1);
    promocodeInfo->setLayoutReimpl(promocodeInfoLayout);

    auto contentWidget = new QWidget;
    content->setWidget(contentWidget);
    content->setWidgetResizable(true);
    auto layout = new QGridLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    row = 0;
    layout->addWidget(accountInfo, row, 0, 1, 2);
    layout->addWidget(avatar, row++, 2, 3, 1, Qt::AlignTop);
    layout->addWidget(subscriptionInfo, row++, 0, 1, 2);
    layout->addWidget(promocodeInfo, row++, 0, 1, 2);
    layout->addWidget(sessionsTitle, row++, 0, 1, 2);
    ++row; // оставляем строку для сессий
    layout->setRowStretch(row, 1);
    layout->setColumnStretch(0, 1);
    layout->setColumnStretch(1, 1);
    contentWidget->setLayout(layout);
}

void AccountView::Implementation::scrollToTitle(AbstractLabel* title)
{
    const QRect microFocus = title->inputMethodQuery(Qt::ImCursorRectangle).toRect();
    const QRect defaultMicroFocus
        = title->QWidget::inputMethodQuery(Qt::ImCursorRectangle).toRect();
    QRect focusRect = (microFocus != defaultMicroFocus)
        ? QRect(title->mapTo(content->widget(), microFocus.topLeft()), microFocus.size())
        : QRect(title->mapTo(content->widget(), QPoint(0, 0)), title->size());

    focusRect.adjust(-50, -50, 50, 50);

    scrollAnimation.setStartValue(content->verticalScrollBar()->value());
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

void AccountView::Implementation::updateSubscriptionEndsLabel()
{
    subscriptionEndsLabel->setText(
        subscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(subscriptionEnds.toString("dd.MM.yyyy")));
}


// ****


AccountView::AccountView(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    QVBoxLayout* layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(d->content);
    setLayout(layout);


    connect(&d->scrollAnimation, &QVariantAnimation::valueChanged, this,
            [this](const QVariant& _value) {
                d->content->verticalScrollBar()->setValue(_value.toInt());
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
    ;
    //
    connect(d->avatar, &ImageCard::imageChanged, this, &AccountView::avatarChanged);

    //
    // Подписка
    //
    connect(d->subscriptionTryProForFree, &Button::clicked, this,
            &AccountView::tryProForFreePressed);
    connect(d->subscriptionTryTeamForFree, &Button::clicked, this,
            &AccountView::tryTeamForFreePressed);
    connect(d->subscriptionBuyProLifetime, &Button::clicked, this,
            &AccountView::buyProLifetimePressed);
    connect(d->subscriptionRenewPro, &Button::clicked, this, &AccountView::renewProPressed);
    connect(d->subscriptionRenewTeam, &Button::clicked, this, &AccountView::renewTeamPressed);
    connect(d->subscriptionUpgradeToPro, &Button::clicked, this, &AccountView::renewProPressed);
    connect(d->subscriptionUpgradeToTeam, &Button::clicked, this, &AccountView::renewTeamPressed);

    connect(d->promocodeName, &TextField::textChanged, d->promocodeName,
            [this] { d->promocodeName->setError({}); });
    connect(d->promocodeName, &TextField::enterPressed, d->activatePromocode, &Button::click);
    connect(d->activatePromocode, &Button::clicked, this, [this] {
        if (d->promocodeName->text().isEmpty()) {
            return;
        }

        emit activatePromocodePressed(d->promocodeName->text());
    });
}

AccountView::~AccountView() = default;

void AccountView::showAccount()
{
    d->scrollToTitle(d->accountTitle);
}

void AccountView::showSubscription()
{
    d->scrollToTitle(d->subscriptionTitle);
}

void AccountView::showSessions()
{
    d->scrollToTitle(d->sessionsTitle);
}

void AccountView::setConnected(bool _connected)
{
    d->name->setEnabled(_connected);
    d->description->setEnabled(_connected);
    d->avatar->setEnabled(_connected);
    d->subscriptionTryProForFree->setEnabled(_connected);
    d->subscriptionTryTeamForFree->setEnabled(_connected);
    d->subscriptionBuyProLifetime->setEnabled(_connected);
    d->subscriptionRenewPro->setEnabled(_connected);
    d->subscriptionRenewTeam->setEnabled(_connected);
    d->subscriptionUpgradeToPro->setEnabled(_connected);
    d->subscriptionUpgradeToTeam->setEnabled(_connected);
}

void AccountView::setEmail(const QString& _email)
{
    d->accountTitle->setText(_email);
}

void AccountView::setName(const QString& _name)
{
    if (d->name->text() == _name) {
        return;
    }

    QSignalBlocker blocker(d->name);
    d->name->setText(_name);
}

void AccountView::setDescription(const QString& _description)
{
    if (d->description->text() == _description) {
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
    d->subscriptionTryProForFree->hide();
    d->subscriptionTryTeamForFree->hide();
    d->subscriptionBuyProLifetime->hide();
    d->subscriptionRenewPro->hide();
    d->subscriptionRenewTeam->hide();
    d->subscriptionUpgradeToPro->hide();
    d->subscriptionUpgradeToTeam->hide();

    const auto subscription = _account.subscriptions.constLast();
    switch (subscription.type) {
    case Domain::SubscriptionType::Free: {
        d->subscriptionTitle->setText(tr("FREE version"));
        d->subscriptionEndsLabel->setText(tr("Lifetime access"));
        break;
    }

    case Domain::SubscriptionType::ProMonthly: {
        d->subscriptionTitle->setText(tr("PRO version"));
        d->subscriptionEnds = subscription.end;
        d->updateSubscriptionEndsLabel();
        break;
    }
    case Domain::SubscriptionType::ProLifetime: {
        d->subscriptionTitle->setText(tr("PRO version"));
        d->subscriptionEnds = {};
        d->updateSubscriptionEndsLabel();
        break;
    }

    case Domain::SubscriptionType::TeamMonthly: {
        d->subscriptionTitle->setText(tr("TEAM version"));
        d->subscriptionEnds = subscription.end;
        d->updateSubscriptionEndsLabel();
        break;
    }

    case Domain::SubscriptionType::TeamLifetime: {
        d->subscriptionTitle->setText(tr("TEAM version"));
        d->subscriptionEnds = {};
        d->updateSubscriptionEndsLabel();
        break;
    }

    default: {
        break;
    }
    }

    //
    // Если есть бесплатные опции, покажем только их
    //
    if (std::find_if(_account.paymentOptions.begin(), _account.paymentOptions.end(),
                     [](const Domain::PaymentOption& _option) { return _option.amount == 0; })
        != _account.paymentOptions.end()) {
        for (const auto& option : _account.paymentOptions) {
            if (option.amount != 0) {
                continue;
            }

            if (option.subscriptionType == Domain::SubscriptionType::ProMonthly) {
                d->subscriptionTryProForFree->show();
            } else if (option.subscriptionType == Domain::SubscriptionType::TeamMonthly) {
                d->subscriptionTryTeamForFree->show();
            }
        }
    }
    //
    // В противном случае показываем в зависимости от текущей подписки
    //
    else {
        switch (subscription.type) {
        case Domain::SubscriptionType::Free: {
            for (const auto& option : _account.paymentOptions) {
                if (option.subscriptionType == Domain::SubscriptionType::ProMonthly
                    || option.subscriptionType == Domain::SubscriptionType::ProLifetime) {
                    d->subscriptionUpgradeToPro->show();
                } else if (option.subscriptionType == Domain::SubscriptionType::TeamMonthly
                           || option.subscriptionType == Domain::SubscriptionType::TeamLifetime) {
                    d->subscriptionUpgradeToTeam->show();
                }
            }
            break;
        }

        case Domain::SubscriptionType::ProMonthly: {
            for (const auto& option : _account.paymentOptions) {
                if (option.subscriptionType == Domain::SubscriptionType::ProMonthly) {
                    d->subscriptionRenewPro->show();
                } else if (option.subscriptionType == Domain::SubscriptionType::ProLifetime) {
                    d->subscriptionBuyProLifetime->show();
                } else if (option.subscriptionType == Domain::SubscriptionType::TeamMonthly
                           || option.subscriptionType == Domain::SubscriptionType::TeamLifetime) {
                    d->subscriptionUpgradeToTeam->show();
                }
            }
            break;
        }

        case Domain::SubscriptionType::ProLifetime: {
            for (const auto& option : _account.paymentOptions) {
                if (option.subscriptionType == Domain::SubscriptionType::TeamMonthly
                    || option.subscriptionType == Domain::SubscriptionType::TeamLifetime) {
                    d->subscriptionUpgradeToTeam->show();
                }
            }
            break;
        }

        case Domain::SubscriptionType::TeamMonthly: {
            for (const auto& option : _account.paymentOptions) {
                if (option.subscriptionType == Domain::SubscriptionType::TeamMonthly) {
                    d->subscriptionRenewTeam->show();
                }
            }
            break;
        }

        default: {
            break;
        }
        }
    }
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

    auto layout = qobject_cast<QGridLayout*>(d->content->widget()->layout());
    int row = layout->rowCount() - 2;
    int column = 0;
    for (const auto& sessionInfo : _sessions) {
        auto sessionWidget = new SessionWidget(d->content->widget());
        sessionWidget->setSessionInfo(sessionInfo);
        if (_sessions.size() == 1) {
            sessionWidget->hideterminateButton();
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
        if (column > 1) {
            ++row;
            column = 0;
        }

        d->sessions.append(sessionWidget);
    }
}

void AccountView::updateTranslations()
{
    d->name->setLabel(tr("Your name"));
    d->description->setLabel(tr("Your bio"));
    d->newsletterSubscription->setText(tr("I want to receive project's news"));
    d->avatar->setSupportingText(tr("Add avatar +"), tr("Change avatar..."),
                                 tr("Do you want to delete your avatar?"));
    d->avatar->setImageCroppingText(tr("Select an area for the avatar"));
    d->subscriptionTitle->setText(tr("Subscription type"));
    d->updateSubscriptionEndsLabel();
    d->subscriptionDetails->setText(tr("Compare versions"));
    d->subscriptionTryProForFree->setText(tr("Try PRO for free"));
    d->subscriptionTryTeamForFree->setText(tr("Try TEAM for free"));
    d->subscriptionBuyProLifetime->setText(tr("Buy lifetime"));
    d->subscriptionRenewPro->setText(tr("Renew"));
    d->subscriptionRenewTeam->setText(tr("Renew"));
    d->subscriptionUpgradeToPro->setText(tr("Upgrade to PRO"));
    d->subscriptionUpgradeToTeam->setText(tr("Upgrade to TEAM"));
    d->sessionsTitle->setText(tr("Active sessions"));
    d->promocodeName->setLabel(tr("Promotional or gift code"));
    d->activatePromocode->setText(tr("Activate"));
}

void AccountView::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());

    d->content->widget()->layout()->setContentsMargins(
        QMarginsF(Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().topContentMargin(),
                  Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24())
            .toMargins());

    d->colorAnimation.setStartValue(Ui::DesignSystem::color().secondary());

    for (auto card : {
             d->accountInfo,
             d->subscriptionInfo,
             d->promocodeInfo,
         }) {
        card->setBackgroundColor(DesignSystem::color().background());
    }

    auto titleColor = DesignSystem::color().onBackground();
    titleColor.setAlphaF(DesignSystem::inactiveTextOpacity());
    auto titleMargins = Ui::DesignSystem::label().margins().toMargins();
    titleMargins.setBottom(0);
    for (auto title : {
             d->accountTitle,
             d->subscriptionTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().background());
        title->setTextColor(DesignSystem::color().onBackground());
        title->setContentsMargins(titleMargins);
    }

    for (auto title : {
             d->sessionsTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().surface());
        title->setTextColor(titleColor);
        title->setContentsMargins(titleMargins);
    }

    auto labelMargins = Ui::DesignSystem::label().margins().toMargins();
    labelMargins.setTop(0);
    labelMargins.setBottom(0);
    for (auto subtitle : std::vector<Widget*>{
             d->subscriptionEndsLabel,
             d->subscriptionDetails,
         }) {
        subtitle->setContentsMargins(labelMargins);
        subtitle->setBackgroundColor(Ui::DesignSystem::color().background());
        subtitle->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onBackground(),
                                                        Ui::DesignSystem::inactiveTextOpacity()));
    }

    for (auto textField : {
             d->name,
             d->description,
             d->promocodeName,
         }) {
        textField->setBackgroundColor(Ui::DesignSystem::color().onBackground());
        textField->setTextColor(Ui::DesignSystem::color().onBackground());
    }

    d->name->setCustomMargins(
        { Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px24(),
          Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px16() });

    d->newsletterSubscription->setBackgroundColor(Ui::DesignSystem::color().background());
    d->newsletterSubscription->setTextColor(Ui::DesignSystem::color().onBackground());

    for (auto button : {
             d->subscriptionTryProForFree,
             d->subscriptionTryTeamForFree,
             d->subscriptionBuyProLifetime,
             d->subscriptionRenewPro,
             d->subscriptionRenewTeam,
             d->subscriptionUpgradeToPro,
             d->subscriptionUpgradeToTeam,
             d->activatePromocode,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->avatar->setBackgroundColor(Ui::DesignSystem::color().background());
    d->avatar->setTextColor(Ui::DesignSystem::color().onBackground());
    d->avatar->setFixedSize((QSizeF(288, 288) * Ui::DesignSystem::scaleFactor()).toSize());

    d->accountInfoLayout->setRowMinimumHeight(d->accountInfoLastRow,
                                              static_cast<int>(Ui::DesignSystem::layout().px8()));
    d->subscriptionInfoLayout->setVerticalSpacing(Ui::DesignSystem::layout().px24());
    d->subscriptionInfoLayout->setRowMinimumHeight(
        d->subscriptionInfoLastRow, static_cast<int>(Ui::DesignSystem::layout().px16()));
    d->subscriptionInfoLayout->setContentsMargins(0, 0, Ui::DesignSystem::layout().px16(), 0);
    d->promocodeInfoLayout->setContentsMargins(0, Ui::DesignSystem::layout().px24(),
                                               Ui::DesignSystem::layout().px24(),
                                               Ui::DesignSystem::layout().px24());
}

} // namespace Ui

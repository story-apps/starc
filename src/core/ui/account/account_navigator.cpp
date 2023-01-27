#include "account_navigator.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/progress_bar/progress_bar.h>
#include <ui/widgets/tree/tree.h>
#include <utils/helpers/color_helper.h>

#include <QDate>
#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
const int kAccountIndex = 0;
const int kSubscriptionIndex = 1;
const int kSessionsIndex = 2;
} // namespace

class AccountNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить текст лейбла окончания подписки
     */
    void updateProSubtitleLabel();
    void updateTeamSubtitleLabel();
    void updateCreditsSubtitleLabel();


    quint64 cloudStorageSize = 0;
    quint64 cloudStorageSizeUsed = 0;
    QDateTime proSubscriptionEnds;
    QDateTime teamSubscriptionEnds;
    int creditsAvailable = 0;

    Tree* tree = nullptr;

    ButtonLabel* freeTitle = nullptr;
    Subtitle2Label* freeSubtitle = nullptr;

    IconsMidLabel* proTitleIcon = nullptr;
    ButtonLabel* proTitle = nullptr;
    Subtitle2Label* proSubtitle = nullptr;
    Button* tryProButton = nullptr;
    Button* upgradeToProLifetimeButton = nullptr;
    Button* renewProSubscriptionButton = nullptr;

    IconsMidLabel* teamTitleIcon = nullptr;
    ButtonLabel* teamTitle = nullptr;
    ProgressBar* teamSpaceStats = nullptr;
    Subtitle2Label* teamSpaceInfo = nullptr;
    Subtitle2Label* teamSubtitle = nullptr;
    Button* tryTeamButton = nullptr;
    Button* renewTeamSubscriptionButton = nullptr;

    IconsMidLabel* creditsTitleIcon = nullptr;
    ButtonLabel* creditsTitle = nullptr;
    Subtitle2Label* creditsSubtitle = nullptr;
    Button* buyCreditsButton = nullptr;

    Button* logoutButton = nullptr;

    QGridLayout* layout = nullptr;
};

AccountNavigator::Implementation::Implementation(QWidget* _parent)
    : tree(new Tree(_parent))
    , freeTitle(new ButtonLabel(_parent))
    , freeSubtitle(new Subtitle2Label(_parent))
    //
    , proTitleIcon(new IconsMidLabel(_parent))
    , proTitle(new ButtonLabel(_parent))
    , proSubtitle(new Subtitle2Label(_parent))
    , tryProButton(new Button(_parent))
    , upgradeToProLifetimeButton(new Button(_parent))
    , renewProSubscriptionButton(new Button(_parent))
    //
    , teamTitleIcon(new IconsMidLabel(_parent))
    , teamTitle(new ButtonLabel(_parent))
    , teamSpaceStats(new ProgressBar(_parent))
    , teamSpaceInfo(new Subtitle2Label(_parent))
    , teamSubtitle(new Subtitle2Label(_parent))
    , tryTeamButton(new Button(_parent))
    , renewTeamSubscriptionButton(new Button(_parent))
    //
    , creditsTitleIcon(new IconsMidLabel(_parent))
    , creditsTitle(new ButtonLabel(_parent))
    , creditsSubtitle(new Subtitle2Label(_parent))
    , buyCreditsButton(new Button(_parent))
    //
    , logoutButton(new Button(_parent))
    , layout(new QGridLayout)
{
    auto createItem = [](const QString& _icon) {
        auto item = new QStandardItem;
        item->setData(_icon, Qt::DecorationRole);
        item->setEditable(false);
        return item;
    };
    QStandardItemModel* model = new QStandardItemModel(tree);
    model->appendRow(createItem(u8"\U000F0004"));
    model->appendRow(createItem(u8"\U000F01C1"));
    model->appendRow(createItem(u8"\U000F09A7"));
    tree->setModel(model);
    tree->setCurrentIndex(model->index(0, 0));

    proTitleIcon->setIcon(u8"\U000F18BC");
    teamTitleIcon->setIcon(u8"\U000F015F");
    creditsTitleIcon->setIcon(u8"\U000F133C");

    logoutButton->setIcon(u8"\U000F0343");
}

void AccountNavigator::Implementation::updateProSubtitleLabel()
{
    proSubtitle->setText(
        proSubscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(proSubscriptionEnds.toString("dd.MM.yyyy")));
}

void AccountNavigator::Implementation::updateTeamSubtitleLabel()
{
    if (cloudStorageSize > 0) {
        teamSpaceStats->setProgress(cloudStorageSizeUsed / static_cast<qreal>(cloudStorageSize));
        const qreal divider = 1024. * 1024. * 1024.;
        teamSpaceInfo->setText(
            tr("Used %1 GB from %2 GB")
                .arg(QString::number(static_cast<qreal>(cloudStorageSizeUsed) / divider, 'f', 2),
                     QString::number(static_cast<qreal>(cloudStorageSize) / divider, 'f', 2)));
    }
    teamSubtitle->setText(
        teamSubscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(teamSubscriptionEnds.toString("dd.MM.yyyy")));
}

void AccountNavigator::Implementation::updateCreditsSubtitleLabel()
{
    creditsSubtitle->setText(creditsAvailable > 0
                                 ? tr("%n credits available", nullptr, creditsAvailable)
                                 : tr("No credits available"));
}


// ****

AccountNavigator::AccountNavigator(QWidget* _parent)
    : Widget(_parent)
    , d(new Implementation(this))
{
    d->layout->setContentsMargins({});
    d->layout->setSpacing(0);
    int row = 0;
    d->layout->addWidget(d->tree, row++, 0, 1, 4);
    d->layout->addWidget(d->freeTitle, row++, 2);
    d->layout->addWidget(d->freeSubtitle, row++, 2);
    //
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->proTitleIcon);
        layout->addWidget(d->proTitle, 1);
        d->layout->addLayout(layout, row++, 2);
    }
    d->layout->addWidget(d->proSubtitle, row++, 2);
    d->layout->addWidget(d->tryProButton, row++, 2);
    d->layout->addWidget(d->upgradeToProLifetimeButton, row++, 2);
    d->layout->addWidget(d->renewProSubscriptionButton, row++, 2);
    //
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->teamTitleIcon);
        layout->addWidget(d->teamTitle, 1);
        d->layout->addLayout(layout, row++, 2);
    }
    d->layout->addWidget(d->teamSpaceStats, row++, 2);
    d->layout->addWidget(d->teamSpaceInfo, row++, 2);
    d->layout->addWidget(d->teamSubtitle, row++, 2);
    d->layout->addWidget(d->tryTeamButton, row++, 2);
    d->layout->addWidget(d->renewTeamSubscriptionButton, row++, 2);
    //
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(d->creditsTitleIcon);
        layout->addWidget(d->creditsTitle, 1);
        d->layout->addLayout(layout, row++, 2);
    }
    d->layout->addWidget(d->creditsSubtitle, row++, 2);
    d->layout->addWidget(d->buyCreditsButton, row++, 2);
    //
    d->layout->setRowStretch(row++, 1);
    d->layout->addWidget(d->logoutButton, row++, 1, 1, 2);
    setLayout(d->layout);

    connect(d->tree, &Tree::currentIndexChanged, this, [this](const QModelIndex& _index) {
        switch (_index.row()) {
        case kAccountIndex: {
            emit accountPressed();
            break;
        }
        case kSubscriptionIndex: {
            emit subscriptionPressed();
            break;
        }
        case kSessionsIndex: {
            emit sessionsPressed();
            break;
        }
        default: {
            break;
        }
        }
    });
    connect(d->tryProButton, &Button::clicked, this, &AccountNavigator::tryProForFreePressed);
    connect(d->upgradeToProLifetimeButton, &Button::clicked, this,
            &AccountNavigator::buyProLifetimePressed);
    connect(d->renewProSubscriptionButton, &Button::clicked, this,
            &AccountNavigator::renewProPressed);
    connect(d->tryTeamButton, &Button::clicked, this, &AccountNavigator::tryTeamForFreePressed);
    connect(d->renewTeamSubscriptionButton, &Button::clicked, this,
            &AccountNavigator::renewTeamPressed);
    connect(d->buyCreditsButton, &Button::clicked, this, &AccountNavigator::buyCreditsPressed);
    connect(d->logoutButton, &Button::clicked, this, &AccountNavigator::logoutPressed);
}

AccountNavigator::~AccountNavigator() = default;

void AccountNavigator::setConnected(bool _connected)
{
    d->tryProButton->setEnabled(_connected);
    d->upgradeToProLifetimeButton->setEnabled(_connected);
    d->renewProSubscriptionButton->setEnabled(_connected);
    d->tryTeamButton->setEnabled(_connected);
    d->renewTeamSubscriptionButton->setEnabled(_connected);
    d->buyCreditsButton->setEnabled(_connected);
}

void AccountNavigator::setAccountInfo(const Domain::AccountInfo& _account)
{
    //
    // Преднастроим видимость разных элементов
    //
    d->freeTitle->show();
    d->freeSubtitle->show();
    d->proTitleIcon->show();
    d->proTitle->show();
    //
    d->proSubtitle->hide();
    d->tryProButton->hide();
    d->upgradeToProLifetimeButton->hide();
    d->renewProSubscriptionButton->hide();
    //
    d->teamSpaceStats->hide();
    d->teamSpaceInfo->hide();
    d->teamSubtitle->hide();
    d->tryTeamButton->hide();
    d->renewTeamSubscriptionButton->hide();

    //
    // А потом показываем, в зависимости от активных подписок и доступных опций
    //
    d->cloudStorageSize = _account.cloudStorageSize;
    d->cloudStorageSizeUsed = _account.cloudStorageSizeUsed;
    for (const auto& subscription : _account.subscriptions) {
        switch (subscription.type) {
        case Domain::SubscriptionType::Free:
        case Domain::SubscriptionType::Corporate: {
            break;
        }

        case Domain::SubscriptionType::ProMonthly: {
            d->proSubscriptionEnds = subscription.end;
            d->updateProSubtitleLabel();
            d->proSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::ProLifetime: {
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            d->proSubscriptionEnds = {};
            d->updateProSubtitleLabel();
            d->proSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::TeamMonthly: {
            d->teamSubscriptionEnds = subscription.end;
            d->updateTeamSubtitleLabel();
            d->teamSpaceStats->show();
            d->teamSpaceInfo->show();
            d->teamSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::TeamLifetime: {
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            d->proTitleIcon->hide();
            d->proTitle->hide();
            d->teamSubscriptionEnds = {};
            d->updateTeamSubtitleLabel();
            d->teamSpaceStats->show();
            d->teamSpaceInfo->show();
            d->teamSubtitle->show();
            break;
        }
        }
    }
    for (const auto& paymentOption : _account.paymentOptions) {
        switch (paymentOption.subscriptionType) {
        case Domain::SubscriptionType::ProLifetime: {
            d->upgradeToProLifetimeButton->show();
            break;
        }

        case Domain::SubscriptionType::ProMonthly: {
            if (paymentOption.amount == 0) {
                d->tryProButton->show();
            } else {
                d->renewProSubscriptionButton->show();
            }
            break;
        }

        case Domain::SubscriptionType::TeamMonthly: {
            if (paymentOption.amount == 0) {
                d->tryTeamButton->show();
            } else {
                d->renewTeamSubscriptionButton->show();
            }
            break;
        }

        default: {
            break;
        }
        }
    }

    //
    // Также настроим информацию о доступных кредитах
    //
    d->creditsAvailable = _account.credits;
    d->updateCreditsSubtitleLabel();
}

void AccountNavigator::updateTranslations()
{
    auto model = qobject_cast<QStandardItemModel*>(d->tree->model());
    model->item(kAccountIndex)->setText(tr("Account"));
    model->item(kSubscriptionIndex)->setText(tr("Subscription"));
    model->item(kSessionsIndex)->setText(tr("Sessions"));
    d->freeTitle->setText(tr("FREE version"));
    d->freeSubtitle->setText(tr("Lifetime access"));
    d->proTitle->setText(tr("PRO version"));
    d->updateProSubtitleLabel();
    d->tryProButton->setText(tr("Try for free"));
    d->upgradeToProLifetimeButton->setText(tr("Buy lifetime"));
    d->renewProSubscriptionButton->setText(tr("Renew"));
    d->teamTitle->setText(tr("TEAM version"));
    d->updateTeamSubtitleLabel();
    d->tryTeamButton->setText(tr("Try for free"));
    d->renewTeamSubscriptionButton->setText(tr("Renew"));
    d->creditsTitle->setText(tr("Credits for Ai tools"));
    d->buyCreditsButton->setText(tr("Buy credits"));
    d->logoutButton->setText(tr("Logout"));
}

void AccountNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());

    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());

    auto titleIconMargins
        = QMarginsF(Ui::DesignSystem::layout().px(18), Ui::DesignSystem::layout().px(20),
                    Ui::DesignSystem::layout().px12(), Ui::DesignSystem::layout().px4());
    for (auto icon : {
             d->proTitleIcon,
             d->teamTitleIcon,
             d->creditsTitleIcon,
         }) {
        icon->setBackgroundColor(Ui::DesignSystem::color().primary());
        icon->setTextColor(Ui::DesignSystem::color().onPrimary());
        icon->setContentsMargins(titleIconMargins.toMargins());
    }
    auto titleMargins = Ui::DesignSystem::label().margins();
    titleMargins.setLeft(0);
    titleMargins.setTop(Ui::DesignSystem::layout().px24());
    titleMargins.setBottom(0);
    for (auto title : {
             d->freeTitle,
             d->proTitle,
             d->teamTitle,
             d->creditsTitle,
         }) {
        title->setBackgroundColor(Ui::DesignSystem::color().primary());
        title->setTextColor(Ui::DesignSystem::color().onPrimary());
        title->setContentsMargins(titleMargins.toMargins());
    }

    auto freeTitleMargins = titleMargins;
    freeTitleMargins.setLeft(Ui::DesignSystem::layout().px(18));
    freeTitleMargins.setBottom(Ui::DesignSystem::layout().px12());
    d->freeTitle->setContentsMargins(freeTitleMargins.toMargins());

    auto subtitleMargins = freeTitleMargins;
    subtitleMargins.setTop(Ui::DesignSystem::layout().px2());
    for (auto subtitle : {
             d->freeSubtitle,
             d->proSubtitle,
             d->teamSpaceInfo,
             d->teamSubtitle,
             d->creditsSubtitle,
         }) {
        subtitle->setBackgroundColor(Ui::DesignSystem::color().primary());
        subtitle->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                        Ui::DesignSystem::inactiveItemOpacity()));
        subtitle->setContentsMargins(subtitleMargins.toMargins());
    }

    for (auto button : {
             d->tryProButton,
             d->upgradeToProLifetimeButton,
             d->renewProSubscriptionButton,
             d->tryTeamButton,
             d->renewTeamSubscriptionButton,
             d->buyCreditsButton,
             d->logoutButton,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    d->teamSpaceStats->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->teamSpaceStats->setContentsMargins(
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::layout().px16(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::layout().px4());

    d->layout->setVerticalSpacing(Ui::DesignSystem::layout().px4());
    d->layout->setColumnMinimumWidth(0, Ui::DesignSystem::layout().px12());
    d->layout->setColumnMinimumWidth(1, Ui::DesignSystem::layout().px16());
    d->layout->setColumnMinimumWidth(3, Ui::DesignSystem::layout().px12());
    d->layout->setContentsMargins(0, 0, 0, Ui::DesignSystem::layout().px12());
}

} // namespace Ui

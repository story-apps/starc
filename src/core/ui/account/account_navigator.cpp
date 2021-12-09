#include "account_navigator.h"

#include <domain/payent_info.h>
#include <domain/subscription_info.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/label/label.h>
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
     * @brief Обновить текст лейбла окончания ПРО подписки
     */
    void updateProSubscriptionEndsLabel();


    Tree* tree = nullptr;

    ButtonLabel* freeTitle = nullptr;
    Button* tryProButton = nullptr;
    Button* upgradeToProButton = nullptr;

    ButtonLabel* proTitle = nullptr;
    QDateTime proSubscriptionEnds;
    Subtitle2Label* proSubscriptionEndsLabel = nullptr;
    Button* renewProSubscriptionButton = nullptr;
    Button* upgradeToProLifetimeButton = nullptr;
    Button* upgradeToTeamButton = nullptr;

    ButtonLabel* teamTitle = nullptr;

    Button* logoutButton = nullptr;

    QGridLayout* layout = nullptr;
};

AccountNavigator::Implementation::Implementation(QWidget* _parent)
    : tree(new Tree(_parent))
    , freeTitle(new ButtonLabel(_parent))
    , tryProButton(new Button(_parent))
    , upgradeToProButton(new Button(_parent))
    , proTitle(new ButtonLabel(_parent))
    , proSubscriptionEndsLabel(new Subtitle2Label(_parent))
    , renewProSubscriptionButton(new Button(_parent))
    , upgradeToProLifetimeButton(new Button(_parent))
    , upgradeToTeamButton(new Button(_parent))
    , teamTitle(new ButtonLabel(_parent))
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

    logoutButton->setIcon(u8"\U000F0343");
}

void AccountNavigator::Implementation::updateProSubscriptionEndsLabel()
{
    proSubscriptionEndsLabel->setText(
        tr("Active until %1").arg(proSubscriptionEnds.toString("dd.MM.yyyy")));
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
    d->layout->addWidget(d->tryProButton, row++, 2);
    d->layout->addWidget(d->upgradeToProButton, row++, 2);
    d->layout->addWidget(d->proTitle, row++, 2);
    d->layout->addWidget(d->proSubscriptionEndsLabel, row++, 2);
    d->layout->addWidget(d->renewProSubscriptionButton, row++, 2);
    d->layout->addWidget(d->upgradeToProLifetimeButton, row++, 2);
    d->layout->addWidget(d->upgradeToTeamButton, row++, 2);
    d->layout->addWidget(d->teamTitle, row++, 2);
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
    connect(d->tryProButton, &Button::clicked, this, &AccountNavigator::upgradeToProPressed);
    connect(d->upgradeToProButton, &Button::clicked, this, &AccountNavigator::upgradeToProPressed);
    connect(d->logoutButton, &Button::clicked, this, &AccountNavigator::logoutPressed);


    updateTranslations();
    designSystemChangeEvent(nullptr);
}

AccountNavigator::~AccountNavigator() = default;

void AccountNavigator::setSubscriptionInfo(Domain::SubscriptionType _subscriptionType,
                                           const QDateTime& _subscriptionEnds,
                                           const QVector<Domain::PaymentOption>& _paymentOptions)
{
    switch (_subscriptionType) {
    case Domain::SubscriptionType::Free: {
        d->freeTitle->show();
        d->tryProButton->hide();
        d->upgradeToProButton->show();
        for (const auto& paymentOption : _paymentOptions) {
            if (paymentOption.amount != 0
                || paymentOption.subscriptionType != Domain::SubscriptionType::ProMonthly) {
                continue;
            }

            d->tryProButton->show();
            d->upgradeToProButton->hide();
            break;
        }
        d->proTitle->hide();
        d->proSubscriptionEndsLabel->hide();
        d->renewProSubscriptionButton->hide();
        d->upgradeToProLifetimeButton->hide();
        d->upgradeToTeamButton->hide();
        d->teamTitle->hide();
        break;
    }

    case Domain::SubscriptionType::ProMonthly: {
        d->freeTitle->hide();
        d->tryProButton->hide();
        d->upgradeToProButton->hide();
        d->proTitle->show();
        d->proSubscriptionEnds = _subscriptionEnds;
        d->updateProSubscriptionEndsLabel();
        d->proSubscriptionEndsLabel->show();
        d->renewProSubscriptionButton->hide(); // TODO:
        d->upgradeToProLifetimeButton->hide(); // TODO:
        d->upgradeToTeamButton->hide(); // TODO:
        d->teamTitle->hide();
        break;
    }

    case Domain::SubscriptionType::ProLifetime: {
        d->freeTitle->hide();
        d->tryProButton->hide();
        d->upgradeToProButton->hide();
        d->proTitle->show();
        d->proSubscriptionEndsLabel->hide();
        d->renewProSubscriptionButton->hide();
        d->upgradeToProLifetimeButton->hide();
        d->upgradeToTeamButton->hide(); // TODO:
        d->teamTitle->hide();
        break;
    }

    case Domain::SubscriptionType::TeamMonthly:
    case Domain::SubscriptionType::TeamLifetime: {
        d->freeTitle->hide();
        d->tryProButton->hide();
        d->upgradeToProButton->hide();
        d->proTitle->hide();
        d->proSubscriptionEndsLabel->hide();
        d->renewProSubscriptionButton->hide();
        d->upgradeToProLifetimeButton->hide();
        d->upgradeToTeamButton->hide();
        d->teamTitle->show();
        break;
    }

    case Domain::SubscriptionType::Corporate: {
        d->freeTitle->hide();
        d->tryProButton->hide();
        d->upgradeToProButton->hide();
        d->proTitle->hide();
        d->proSubscriptionEndsLabel->hide();
        d->renewProSubscriptionButton->hide();
        d->upgradeToProLifetimeButton->hide();
        d->upgradeToTeamButton->hide();
        d->teamTitle->hide();
        break;
    }
    }
}

void AccountNavigator::updateTranslations()
{
    auto model = qobject_cast<QStandardItemModel*>(d->tree->model());
    model->item(kAccountIndex)->setText(tr("Account"));
    model->item(kSubscriptionIndex)->setText(tr("Subscription"));
    model->item(kSessionsIndex)->setText(tr("Sessions"));
    d->freeTitle->setText(tr("FREE version"));
    d->tryProButton->setText(tr("Try PRO for free"));
    d->upgradeToProButton->setText(tr("Upgrade to PRO"));
    d->proTitle->setText(tr("PRO version"));
    d->updateProSubscriptionEndsLabel();
    d->renewProSubscriptionButton->setText(tr("Renew"));
    d->upgradeToProLifetimeButton->setText(tr("Buy lifetime"));
    d->upgradeToTeamButton->setText(tr("Upgrade to TEAM"));
    d->teamTitle->setText(tr("TEAM version"));
    d->logoutButton->setText(tr("Logout"));
}

void AccountNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    Widget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());

    d->tree->setBackgroundColor(DesignSystem::color().primary());
    d->tree->setTextColor(DesignSystem::color().onPrimary());

    auto titleMargins = Ui::DesignSystem::label().margins();
    titleMargins.setLeft(Ui::DesignSystem::layout().px(18));
    titleMargins.setBottom(0);
    for (auto title : {
             d->freeTitle,
             d->proTitle,
             d->teamTitle,
         }) {
        title->setBackgroundColor(Ui::DesignSystem::color().primary());
        title->setTextColor(Ui::DesignSystem::color().onPrimary());
        title->setContentsMargins(titleMargins.toMargins());
    }

    auto subtitleMargins = titleMargins;
    subtitleMargins.setTop(Ui::DesignSystem::layout().px2());
    for (auto subtitle : {
             d->proSubscriptionEndsLabel,
         }) {
        subtitle->setBackgroundColor(Ui::DesignSystem::color().primary());
        subtitle->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                        Ui::DesignSystem::inactiveItemOpacity()));
        subtitle->setContentsMargins(subtitleMargins.toMargins());
    }

    for (auto button : {
             d->tryProButton,
             d->upgradeToProButton,
             d->renewProSubscriptionButton,
             d->upgradeToProLifetimeButton,
             d->upgradeToTeamButton,
             d->logoutButton,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().secondary());
        button->setTextColor(Ui::DesignSystem::color().secondary());
    }

    d->layout->setVerticalSpacing(Ui::DesignSystem::layout().px4());
    d->layout->setColumnMinimumWidth(0, Ui::DesignSystem::layout().px12());
    d->layout->setColumnMinimumWidth(1, Ui::DesignSystem::layout().px16());
    d->layout->setColumnMinimumWidth(3, Ui::DesignSystem::layout().px12());
    d->layout->setContentsMargins(0, 0, 0, Ui::DesignSystem::layout().px12());
}

} // namespace Ui

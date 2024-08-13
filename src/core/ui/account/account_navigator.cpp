#include "account_navigator.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/progress_bar/progress_bar.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/tree/tree.h>
#include <ui/widgets/tree/tree_delegate.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/image_helper.h>

#include <QAction>
#include <QDate>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QVBoxLayout>


namespace Ui {

namespace {
enum {
    kAccountIndex = 0,
    kSubscriptionIndex,
    kSessionsIndex,
};

const int kTeamIdRole = Qt::UserRole + 100;

enum {
    kEditTeamAction = 0,
    kRemoveTeamAction,
    kExitTeamAction,
};
} // namespace

class AccountNavigator::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Обновить текст лейбла окончания подписки
     */
    void updateProSubtitleLabel();
    void updateCloudSubtitleLabel();
    void updateStudioSubtitleLabel();
    void updateCreditsSubtitleLabel();

    /**
     * @brief Задать возможность добавления команд
     */
    void setAccountTeamsCanBeAdded(bool _can);


    quint64 cloudStorageSize = 0;
    quint64 cloudStorageSizeUsed = 0;
    QDateTime proSubscriptionEnds;
    QDateTime cloudSubscriptionEnds;
    QDateTime studioSubscriptionEnds;
    int creditsAvailable = 0;
    bool isTeamsCanBeAdded = false;

    //
    // Страница аккаунта
    //

    Widget* accountPage = nullptr;

    Tree* tree = nullptr;

    ButtonLabel* freeTitle = nullptr;
    Subtitle2Label* freeSubtitle = nullptr;

    IconsMidLabel* proTitleIcon = nullptr;
    ButtonLabel* proTitle = nullptr;
    Subtitle2Label* proSubtitle = nullptr;
    Button* tryProButton = nullptr;
    Button* renewProSubscriptionButton = nullptr;

    IconsMidLabel* cloudTitleIcon = nullptr;
    ButtonLabel* cloudTitle = nullptr;
    ProgressBar* cloudSpaceStats = nullptr;
    Subtitle2Label* cloudSpaceInfo = nullptr;
    Subtitle2Label* cloudSubtitle = nullptr;
    Button* tryCloudButton = nullptr;
    Button* renewCloudSubscriptionButton = nullptr;

    IconsMidLabel* studioTitleIcon = nullptr;
    ButtonLabel* studioTitle = nullptr;
    ProgressBar* studioSpaceStats = nullptr;
    Subtitle2Label* studioSpaceInfo = nullptr;
    Subtitle2Label* studioSubtitle = nullptr;

    IconsMidLabel* creditsTitleIcon = nullptr;
    ButtonLabel* creditsTitle = nullptr;
    Subtitle2Label* creditsSubtitle = nullptr;
    Button* buyCreditsButton = nullptr;

    Button* logoutButton = nullptr;

    QGridLayout* accountLayout = nullptr;

    //
    // Страница команд
    //

    Widget* teamsPage = nullptr;
    QScrollArea* teamsContent = nullptr;

    CaptionLabel* teamsOwnerLabel = nullptr;
    Subtitle2Label* teamsOwnerEmptyLabel = nullptr;
    Tree* teamsOwner = nullptr;
    QStandardItemModel* teamsOwnerModel = nullptr;
    CaptionLabel* teamsMemberLabel = nullptr;
    Subtitle2Label* teamsMemberEmptyLabel = nullptr;
    Tree* teamsMember = nullptr;
    QStandardItemModel* teamsMemberModel = nullptr;
    Button* addTeamButton = nullptr;
    QHBoxLayout* addTeamButtonLayout = nullptr;

    ContextMenu* teamsContextMenu = nullptr;
};

AccountNavigator::Implementation::Implementation(QWidget* _parent)
    : accountPage(new Widget(_parent))
    //
    , tree(new Tree(accountPage))
    , freeTitle(new ButtonLabel(accountPage))
    , freeSubtitle(new Subtitle2Label(accountPage))
    //
    , proTitleIcon(new IconsMidLabel(accountPage))
    , proTitle(new ButtonLabel(accountPage))
    , proSubtitle(new Subtitle2Label(accountPage))
    , tryProButton(new Button(accountPage))
    , renewProSubscriptionButton(new Button(accountPage))
    //
    , cloudTitleIcon(new IconsMidLabel(accountPage))
    , cloudTitle(new ButtonLabel(accountPage))
    , cloudSpaceStats(new ProgressBar(accountPage))
    , cloudSpaceInfo(new Subtitle2Label(accountPage))
    , cloudSubtitle(new Subtitle2Label(accountPage))
    , tryCloudButton(new Button(accountPage))
    , renewCloudSubscriptionButton(new Button(accountPage))
    //
    , studioTitleIcon(new IconsMidLabel(accountPage))
    , studioTitle(new ButtonLabel(accountPage))
    , studioSpaceStats(new ProgressBar(accountPage))
    , studioSpaceInfo(new Subtitle2Label(accountPage))
    , studioSubtitle(new Subtitle2Label(accountPage))
    //
    , creditsTitleIcon(new IconsMidLabel(accountPage))
    , creditsTitle(new ButtonLabel(accountPage))
    , creditsSubtitle(new Subtitle2Label(accountPage))
    , buyCreditsButton(new Button(accountPage))
    //
    , logoutButton(new Button(accountPage))
    , accountLayout(new QGridLayout)
    //
    //
    , teamsPage(new Widget(_parent))
    , teamsContent(new QScrollArea(teamsPage))
    //
    , teamsOwnerLabel(new CaptionLabel(teamsPage))
    , teamsOwnerEmptyLabel(new Subtitle2Label(teamsPage))
    , teamsOwner(new Tree(teamsPage))
    , teamsOwnerModel(new QStandardItemModel(teamsPage))
    //
    , teamsMemberLabel(new CaptionLabel(teamsPage))
    , teamsMemberEmptyLabel(new Subtitle2Label(teamsPage))
    , teamsMember(new Tree(teamsPage))
    , teamsMemberModel(new QStandardItemModel(teamsPage))
    //
    , addTeamButton(new Button(teamsPage))
    , addTeamButtonLayout(new QHBoxLayout)
    //
    , teamsContextMenu(new ContextMenu(teamsPage))
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
    tree->setRootIsDecorated(false);

    proTitleIcon->setIcon(u8"\U000F18BC");
    cloudTitleIcon->setIcon(u8"\U000F015F");
    studioTitleIcon->setIcon(u8"\U000F0381");
    creditsTitleIcon->setIcon(u8"\U000F133C");

    logoutButton->setIcon(u8"\U000F0343");

    accountLayout->setContentsMargins({});
    accountLayout->setSpacing(0);
    int row = 0;
    accountLayout->addWidget(tree, row++, 0, 1, 4);
    accountLayout->addWidget(freeTitle, row++, 2);
    accountLayout->addWidget(freeSubtitle, row++, 2);
    //
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(proTitleIcon);
        layout->addWidget(proTitle, 1);
        accountLayout->addLayout(layout, row++, 2);
    }
    accountLayout->addWidget(proSubtitle, row++, 2);
    accountLayout->addWidget(tryProButton, row++, 2);
    accountLayout->addWidget(renewProSubscriptionButton, row++, 2);
    //
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(cloudTitleIcon);
        layout->addWidget(cloudTitle, 1);
        accountLayout->addLayout(layout, row++, 2);
    }
    accountLayout->addWidget(cloudSpaceStats, row++, 2);
    accountLayout->addWidget(cloudSpaceInfo, row++, 2);
    accountLayout->addWidget(cloudSubtitle, row++, 2);
    accountLayout->addWidget(tryCloudButton, row++, 2);
    accountLayout->addWidget(renewCloudSubscriptionButton, row++, 2);
    //
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(studioTitleIcon);
        layout->addWidget(studioTitle, 1);
        accountLayout->addLayout(layout, row++, 2);
    }
    accountLayout->addWidget(studioSpaceStats, row++, 2);
    accountLayout->addWidget(studioSpaceInfo, row++, 2);
    accountLayout->addWidget(studioSubtitle, row++, 2);
    //
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(creditsTitleIcon);
        layout->addWidget(creditsTitle, 1);
        accountLayout->addLayout(layout, row++, 2);
    }
    accountLayout->addWidget(creditsSubtitle, row++, 2);
    accountLayout->addWidget(buyCreditsButton, row++, 2);
    //
    accountLayout->setRowStretch(row++, 1);
    accountLayout->addWidget(logoutButton, row++, 1, 1, 2);
    //
    accountPage->setLayout(accountLayout);


    QPalette palette;
    palette.setColor(QPalette::Base, Qt::transparent);
    palette.setColor(QPalette::Window, Qt::transparent);
    teamsContent->setPalette(palette);
    teamsContent->setFrameShape(QFrame::NoFrame);
    teamsContent->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    teamsContent->setVerticalScrollBar(new ScrollBar);
    teamsOwnerLabel->hide();
    teamsOwnerEmptyLabel->hide();
    teamsOwner->setRootIsDecorated(false);
    teamsOwner->setContextMenuPolicy(Qt::CustomContextMenu);
    teamsOwner->setItemDelegate(new TreeDelegate(teamsOwner));
    teamsOwner->setModel(teamsOwnerModel);
    teamsOwner->hide();
    teamsMember->setRootIsDecorated(false);
    teamsMember->setContextMenuPolicy(Qt::CustomContextMenu);
    teamsMember->setItemDelegate(new TreeDelegate(teamsMember));
    teamsMember->setModel(teamsMemberModel);
    teamsMember->hide();
    addTeamButton->setIcon(u8"\U000F0415");
    addTeamButton->hide();

    auto teamsContentWidget = new QWidget;
    teamsContent->setWidget(teamsContentWidget);
    teamsContent->setWidgetResizable(true);
    auto teamsContentLayout = new QVBoxLayout;
    teamsContentLayout->setContentsMargins({});
    teamsContentLayout->setSpacing(0);
    teamsContentLayout->addWidget(teamsOwnerLabel);
    teamsContentLayout->addWidget(teamsOwnerEmptyLabel);
    teamsContentLayout->addWidget(teamsOwner);
    teamsContentLayout->addWidget(teamsMemberLabel);
    teamsContentLayout->addWidget(teamsMemberEmptyLabel);
    teamsContentLayout->addWidget(teamsMember);
    teamsContentLayout->addStretch();
    teamsContentWidget->setLayout(teamsContentLayout);

    addTeamButtonLayout->setContentsMargins({});
    addTeamButtonLayout->setSpacing(0);
    addTeamButtonLayout->addWidget(addTeamButton);

    auto teamsLayout = new QVBoxLayout;
    teamsLayout->setContentsMargins({});
    teamsLayout->setSpacing(0);
    teamsLayout->addWidget(teamsContent, 1);
    teamsLayout->addLayout(addTeamButtonLayout);
    teamsPage->setLayout(teamsLayout);
}

void AccountNavigator::Implementation::updateProSubtitleLabel()
{
    proSubtitle->setText(
        proSubscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(proSubscriptionEnds.toString("dd.MM.yyyy")));
}

void AccountNavigator::Implementation::updateCloudSubtitleLabel()
{
    if (cloudStorageSize > 0) {
        cloudSpaceStats->setProgress(cloudStorageSizeUsed / static_cast<qreal>(cloudStorageSize));
        const qreal divider = 1024. * 1024. * 1024.;
        cloudSpaceInfo->setText(
            tr("Used %1 GB from %2 GB")
                .arg(QString::number(static_cast<qreal>(cloudStorageSizeUsed) / divider, 'f', 2),
                     QString::number(static_cast<qreal>(cloudStorageSize) / divider, 'f', 2)));
    }
    cloudSubtitle->setText(
        cloudSubscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(cloudSubscriptionEnds.toString("dd.MM.yyyy")));
}

void AccountNavigator::Implementation::updateStudioSubtitleLabel()
{
    if (cloudStorageSize > 0) {
        studioSpaceStats->setProgress(cloudStorageSizeUsed / static_cast<qreal>(cloudStorageSize));
        const qreal divider = 1024. * 1024. * 1024.;
        studioSpaceInfo->setText(
            tr("Used %1 GB from %2 GB")
                .arg(QString::number(static_cast<qreal>(cloudStorageSizeUsed) / divider, 'f', 2),
                     QString::number(static_cast<qreal>(cloudStorageSize) / divider, 'f', 2)));
    }
    studioSubtitle->setText(
        studioSubscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(studioSubscriptionEnds.toString("dd.MM.yyyy")));
}

void AccountNavigator::Implementation::updateCreditsSubtitleLabel()
{
    creditsSubtitle->setText(creditsAvailable > 0
                                 ? tr("%n credits available", nullptr, creditsAvailable / 1000)
                                 : tr("No credits available"));
}

void AccountNavigator::Implementation::setAccountTeamsCanBeAdded(bool _can)
{
    if (isTeamsCanBeAdded == _can) {
        return;
    }

    isTeamsCanBeAdded = _can;

    teamsOwnerLabel->setVisible(_can);
    teamsOwnerEmptyLabel->setVisible(_can);
    teamsOwner->setVisible(_can);
    addTeamButton->setVisible(_can);
}


// ****

AccountNavigator::AccountNavigator(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setAnimationType(StackWidget::AnimationType::Slide);

    setCurrentWidget(d->accountPage);
    addWidget(d->teamsPage);

    auto editTeamAction = new QAction(d->teamsContextMenu);
    editTeamAction->setIconText(u8"\U000F03EB");
    auto removeTeamAction = new QAction(d->teamsContextMenu);
    removeTeamAction->setIconText(u8"\U000F01B4");
    auto exitTeamAction = new QAction(d->teamsContextMenu);
    exitTeamAction->setIconText(u8"\U000F01B4");
    d->teamsContextMenu->setActions({
        editTeamAction,
        removeTeamAction,
        exitTeamAction,
    });

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
    connect(d->renewProSubscriptionButton, &Button::clicked, this,
            &AccountNavigator::renewProPressed);
    connect(d->tryCloudButton, &Button::clicked, this, &AccountNavigator::tryCloudForFreePressed);
    connect(d->renewCloudSubscriptionButton, &Button::clicked, this,
            &AccountNavigator::renewCloudPressed);
    connect(d->buyCreditsButton, &Button::clicked, this, &AccountNavigator::buyCreditsPressed);
    connect(d->logoutButton, &Button::clicked, this, &AccountNavigator::logoutPressed);
    //
    connect(editTeamAction, &QAction::triggered, this, [this] {
        emit editTeamPressed(d->teamsOwner->currentIndex().data(kTeamIdRole).toInt());
    });
    connect(removeTeamAction, &QAction::triggered, this, [this] {
        emit removeTeamPressed(d->teamsOwner->currentIndex().data(kTeamIdRole).toInt());
    });
    connect(exitTeamAction, &QAction::triggered, this, [this] {
        emit exitTeamPressed(d->teamsMember->currentIndex().data(kTeamIdRole).toInt());
    });
    connect(d->teamsOwner, &Tree::currentIndexChanged, this,
            [this](const QModelIndex& _currentIndex) {
                if (!_currentIndex.isValid()) {
                    return;
                }

                d->teamsMember->setCurrentIndex({});
                emit teamSelected(_currentIndex.data(kTeamIdRole).toInt());
            });
    connect(d->teamsOwner, &Tree::doubleClicked, this, [this](const QModelIndex& _index) {
        emit editTeamPressed(_index.data(kTeamIdRole).toInt());
    });
    connect(d->teamsOwner, &Tree::customContextMenuRequested, this, [this] {
        d->teamsContextMenu->actions().at(kEditTeamAction)->setVisible(true);
        d->teamsContextMenu->actions().at(kRemoveTeamAction)->setVisible(true);
        d->teamsContextMenu->actions().at(kExitTeamAction)->setVisible(false);
        d->teamsContextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->teamsMember, &Tree::currentIndexChanged, this,
            [this](const QModelIndex& _currentIndex) {
                if (!_currentIndex.isValid()) {
                    return;
                }

                d->teamsOwner->setCurrentIndex({});
                emit teamSelected(_currentIndex.data(kTeamIdRole).toInt());
            });
    connect(d->teamsMember, &Tree::customContextMenuRequested, this, [this] {
        d->teamsContextMenu->actions().at(kEditTeamAction)->setVisible(false);
        d->teamsContextMenu->actions().at(kRemoveTeamAction)->setVisible(false);
        d->teamsContextMenu->actions().at(kExitTeamAction)->setVisible(true);
        d->teamsContextMenu->showContextMenu(QCursor::pos());
    });
    connect(d->addTeamButton, &Button::clicked, this, &AccountNavigator::createTeamPressed);
}

AccountNavigator::~AccountNavigator() = default;

void AccountNavigator::showAccountPage()
{
    setCurrentWidget(d->accountPage);
}

void AccountNavigator::showTeamPage()
{
    setCurrentWidget(d->teamsPage);
}

void AccountNavigator::setConnected(bool _connected)
{
    d->tryProButton->setEnabled(_connected);
    d->renewProSubscriptionButton->setEnabled(_connected);
    d->tryCloudButton->setEnabled(_connected);
    d->renewCloudSubscriptionButton->setEnabled(_connected);
    d->buyCreditsButton->setEnabled(_connected);
    //
    d->teamsOwner->setContextMenuPolicy(_connected ? Qt::CustomContextMenu : Qt::NoContextMenu);
    d->addTeamButton->setEnabled(_connected);
}

void AccountNavigator::setAccountInfo(const Domain::AccountInfo& _account)
{
    //
    // Преднастроим видимость разных элементов
    //
    d->freeTitle->show();
    d->freeSubtitle->show();
    //
    d->proTitleIcon->show();
    d->proTitle->show();
    d->proSubtitle->hide();
    d->tryProButton->hide();
    d->renewProSubscriptionButton->hide();
    //
    d->cloudTitleIcon->show();
    d->cloudTitle->show();
    d->cloudSpaceStats->hide();
    d->cloudSpaceInfo->hide();
    d->cloudSubtitle->hide();
    d->tryCloudButton->hide();
    d->renewCloudSubscriptionButton->hide();
    //
    d->studioTitleIcon->hide();
    d->studioTitle->hide();
    d->studioSubtitle->hide();
    d->studioSpaceStats->hide();
    d->studioSpaceInfo->hide();
    d->studioSubtitle->hide();

    //
    // А потом показываем, в зависимости от активных подписок и доступных опций
    //
    bool hasProSubscription = false;
    bool hasCloudSubscription = false;
    d->cloudStorageSize = _account.cloudStorageSize;
    d->cloudStorageSizeUsed = _account.cloudStorageSizeUsed;
    auto isAccountTeamsCanBeAdded = false;
    for (const auto& subscription : _account.subscriptions) {
        switch (subscription.type) {
        case Domain::SubscriptionType::Free: {
            break;
        }

        case Domain::SubscriptionType::ProMonthly: {
            hasProSubscription = true;

            d->freeTitle->hide();
            d->freeSubtitle->hide();
            //
            d->proSubscriptionEnds = subscription.end;
            d->updateProSubtitleLabel();
            d->proSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::ProLifetime: {
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            //
            d->proSubscriptionEnds = {};
            d->updateProSubtitleLabel();
            d->proSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::CloudMonthly: {
            hasCloudSubscription = true;

            d->freeTitle->hide();
            d->freeSubtitle->hide();
            //
            d->cloudSubscriptionEnds = subscription.end;
            d->updateCloudSubtitleLabel();
            d->cloudSpaceStats->show();
            d->cloudSpaceInfo->show();
            d->cloudSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::CloudLifetime: {
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            d->proTitleIcon->hide();
            d->proTitle->hide();
            //
            d->cloudSubscriptionEnds = {};
            d->updateCloudSubtitleLabel();
            d->cloudSpaceStats->show();
            d->cloudSpaceInfo->show();
            d->cloudSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::Studio: {
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            d->proTitle->hide();
            d->proSubtitle->hide();
            d->proTitleIcon->hide();
            d->proTitle->hide();
            d->cloudTitleIcon->hide();
            d->cloudTitle->hide();
            d->cloudSubtitle->hide();
            d->cloudSpaceStats->hide();
            d->cloudSpaceInfo->hide();
            d->cloudSubtitle->hide();
            //
            d->studioSubscriptionEnds = subscription.end;
            d->updateStudioSubtitleLabel();
            d->studioTitleIcon->show();
            d->studioTitle->show();
            d->studioSubtitle->show();
            d->studioSpaceStats->show();
            d->studioSpaceInfo->show();
            d->studioSubtitle->show();
            isAccountTeamsCanBeAdded = true;
            break;
        }
        }
    }
    d->setAccountTeamsCanBeAdded(isAccountTeamsCanBeAdded);
    bool isProTrialAvailable = false;
    bool isCloudoTrialAvailable = false;
    for (const auto& paymentOption : _account.paymentOptions) {
        switch (paymentOption.subscriptionType) {
        case Domain::SubscriptionType::ProMonthly: {
            if (paymentOption.amount == 0) {
                isProTrialAvailable = true;
                d->tryProButton->show();
            } else {
                d->renewProSubscriptionButton->show();
            }
            break;
        }

        case Domain::SubscriptionType::CloudMonthly: {
            if (paymentOption.amount == 0) {
                isCloudoTrialAvailable = true;
                d->tryCloudButton->show();
            } else {
                d->renewCloudSubscriptionButton->show();
            }
            break;
        }

        default: {
            break;
        }
        }
    }

    //
    // Если доступны бесплатные опции, то уберём кнопки активации платных
    //
    if (isProTrialAvailable) {
        d->renewProSubscriptionButton->hide();
    }
    if (isCloudoTrialAvailable) {
        d->renewCloudSubscriptionButton->hide();
    }

    //
    // Если активна только какая-то одна подписка, то оставим интерфейс только для неё
    //
    if (hasProSubscription && !hasCloudSubscription) {
        d->cloudTitleIcon->hide();
        d->cloudTitle->hide();
        d->tryCloudButton->hide();
        d->renewCloudSubscriptionButton->hide();
    } else if (!hasProSubscription && hasCloudSubscription) {
        d->proTitleIcon->hide();
        d->proTitle->hide();
        d->tryProButton->hide();
        d->renewProSubscriptionButton->hide();
    }

    //
    // Также настроим информацию о доступных кредитах
    //
    d->creditsAvailable = _account.credits;
    d->updateCreditsSubtitleLabel();
}

void AccountNavigator::setAccountTeams(const QVector<Domain::TeamInfo>& _teams)
{
    const auto teamsOwnerLastRow = d->teamsOwner->currentIndex().row();
    const auto teamsMemberLastRow = d->teamsMember->currentIndex().row();

    d->teamsOwnerModel->clear();
    d->teamsMemberModel->clear();

    auto teamModelRow = [](const Domain::TeamInfo& _team) {
        auto teamRow = new QStandardItem(_team.name);
        teamRow->setData(_team.id, kTeamIdRole);
        teamRow->setData(_team.description, Qt::WhatsThisRole);
        if (_team.avatar.isNull()) {
            teamRow->setData(
                ImageHelper::makeAvatar(_team.name, Ui::DesignSystem::font().body1(),
                                        Ui::DesignSystem::treeOneLineItem().iconSize().toSize(),
                                        Qt::white),
                Qt::DecorationRole);
        } else {
            teamRow->setData(
                ImageHelper::makeAvatar(ImageHelper::imageFromBytes(_team.avatar),
                                        Ui::DesignSystem::treeOneLineItem().iconSize().toSize()),
                Qt::DecorationRole);
        }
        teamRow->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return teamRow;
    };

    for (const auto& team : _teams) {
        constexpr int ownerRole = 0;
        if (team.teamRole == ownerRole) {
            d->teamsOwnerModel->appendRow(teamModelRow(team));
        } else {
            d->teamsMemberModel->appendRow(teamModelRow(team));
        }
    }

    if (d->isTeamsCanBeAdded) {
        const auto isTeamsOwnerVisible = d->teamsOwnerModel->rowCount() > 0;
        d->teamsOwner->setVisible(isTeamsOwnerVisible);
        d->teamsOwner->setFixedHeight(d->teamsOwner->viewportSizeHint().height());
        d->teamsOwnerEmptyLabel->setVisible(!isTeamsOwnerVisible);
    }
    const auto isTeamsMemberVisible = d->teamsMemberModel->rowCount() > 0;
    d->teamsMember->setVisible(isTeamsMemberVisible);
    d->teamsMember->setFixedHeight(d->teamsMember->viewportSizeHint().height());
    d->teamsMemberEmptyLabel->setVisible(!isTeamsMemberVisible);

    //
    // Выбираем повледнюю выбранную, либо первую из доступных команд
    //
    if (teamsOwnerLastRow >= 0) {
        d->teamsOwner->setCurrentIndex(d->teamsOwnerModel->index(teamsOwnerLastRow, 0));
    } else if (teamsMemberLastRow >= 0) {
        d->teamsMember->setCurrentIndex(d->teamsMemberModel->index(teamsMemberLastRow, 0));
    } else if (d->teamsOwnerModel->rowCount() > 0) {
        d->teamsOwner->setCurrentIndex(d->teamsOwnerModel->index(0, 0));
    } else if (d->teamsMemberModel->rowCount() > 0) {
        d->teamsMember->setCurrentIndex(d->teamsMemberModel->index(0, 0));
    }
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
    d->renewProSubscriptionButton->setText(tr("Renew"));
    d->cloudTitle->setText(tr("CLOUD version"));
    d->updateCloudSubtitleLabel();
    d->tryCloudButton->setText(tr("Try for free"));
    d->renewCloudSubscriptionButton->setText(tr("Renew"));
    d->studioTitle->setText(tr("STUDIO version"));
    d->updateStudioSubtitleLabel();
    d->creditsTitle->setText(tr("Credits for Ai tools"));
    d->buyCreditsButton->setText(tr("Buy credits"));
    d->logoutButton->setText(tr("Logout"));

    d->teamsOwnerLabel->setText(tr("Owner"));
    d->teamsOwnerEmptyLabel->setText(
        tr("No one team created yet, press \"Add team\" button for adding your first team"));
    d->teamsMemberLabel->setText(tr("Member"));
    d->teamsMemberEmptyLabel->setText(tr("You are not on any team"));
    d->addTeamButton->setText(tr("Add team"));
    d->teamsContextMenu->actions().at(kEditTeamAction)->setText(tr("Edit team"));
    d->teamsContextMenu->actions().at(kRemoveTeamAction)->setText(tr("Remove team"));
    d->teamsContextMenu->actions().at(kExitTeamAction)->setText(tr("Exit team"));
}

void AccountNavigator::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().primary());

    for (auto widget : std::vector<Widget*>{
             d->accountPage,
             d->tree,
             d->teamsPage,
             d->teamsOwner,
             d->teamsMember,
         }) {
        widget->setBackgroundColor(DesignSystem::color().primary());
        widget->setTextColor(DesignSystem::color().onPrimary());
    }

    auto titleIconMargins
        = QMarginsF(Ui::DesignSystem::layout().px(18), Ui::DesignSystem::compactLayout().px(20),
                    Ui::DesignSystem::layout().px12(), Ui::DesignSystem::compactLayout().px4());
    for (auto icon : {
             d->proTitleIcon,
             d->cloudTitleIcon,
             d->studioTitleIcon,
             d->creditsTitleIcon,
         }) {
        icon->setBackgroundColor(Ui::DesignSystem::color().primary());
        icon->setTextColor(Ui::DesignSystem::color().onPrimary());
        icon->setContentsMargins(titleIconMargins.toMargins());
    }
    auto titleMargins = Ui::DesignSystem::label().margins();
    titleMargins.setLeft(0);
    titleMargins.setTop(Ui::DesignSystem::compactLayout().px24());
    titleMargins.setBottom(0);
    for (auto title : {
             d->freeTitle,
             d->proTitle,
             d->cloudTitle,
             d->studioTitle,
             d->creditsTitle,
         }) {
        title->setBackgroundColor(Ui::DesignSystem::color().primary());
        title->setTextColor(Ui::DesignSystem::color().onPrimary());
        title->setContentsMargins(titleMargins.toMargins());
    }

    auto freeTitleMargins = titleMargins;
    freeTitleMargins.setLeft(Ui::DesignSystem::layout().px(18));
    freeTitleMargins.setBottom(Ui::DesignSystem::compactLayout().px12());
    d->freeTitle->setContentsMargins(freeTitleMargins.toMargins());

    auto subtitleMargins = freeTitleMargins;
    subtitleMargins.setTop(Ui::DesignSystem::compactLayout().px2());
    for (auto subtitle : {
             d->freeSubtitle,
             d->proSubtitle,
             d->cloudSpaceInfo,
             d->cloudSubtitle,
             d->studioSpaceInfo,
             d->studioSubtitle,
             d->creditsSubtitle,
         }) {
        subtitle->setBackgroundColor(Ui::DesignSystem::color().primary());
        subtitle->setTextColor(ColorHelper::transparent(Ui::DesignSystem::color().onPrimary(),
                                                        Ui::DesignSystem::inactiveItemOpacity()));
        subtitle->setContentsMargins(subtitleMargins.toMargins());
    }

    auto captionMargins
        = QMarginsF(Ui::DesignSystem::layout().px(18), Ui::DesignSystem::compactLayout().px(20),
                    Ui::DesignSystem::layout().px12(), Ui::DesignSystem::compactLayout().px4());
    subtitleMargins.setTop(Ui::DesignSystem::compactLayout().px2());
    for (auto caption : std::vector<Widget*>{
             d->teamsOwnerLabel,
             d->teamsOwnerEmptyLabel,
             d->teamsMemberLabel,
             d->teamsMemberEmptyLabel,
         }) {
        caption->setBackgroundColor(Ui::DesignSystem::color().primary());
        caption->setTextColor(Ui::DesignSystem::color().onPrimary());
        caption->setContentsMargins(captionMargins.toMargins());
    }

    for (auto button : {
             d->tryProButton,
             d->renewProSubscriptionButton,
             d->tryCloudButton,
             d->renewCloudSubscriptionButton,
             d->buyCreditsButton,
             d->logoutButton,
             d->addTeamButton,
         }) {
        button->setBackgroundColor(Ui::DesignSystem::color().accent());
        button->setTextColor(Ui::DesignSystem::color().accent());
    }

    d->cloudSpaceStats->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->cloudSpaceStats->setContentsMargins(
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::compactLayout().px16(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px4());
    d->studioSpaceStats->setBackgroundColor(Ui::DesignSystem::color().primary());
    d->studioSpaceStats->setContentsMargins(
        Ui::DesignSystem::layout().px16(), Ui::DesignSystem::compactLayout().px16(),
        Ui::DesignSystem::layout().px24(), Ui::DesignSystem::compactLayout().px4());

    d->accountLayout->setVerticalSpacing(Ui::DesignSystem::compactLayout().px4());
    d->accountLayout->setColumnMinimumWidth(0, Ui::DesignSystem::layout().px4());
    d->accountLayout->setColumnMinimumWidth(3, Ui::DesignSystem::layout().px12());
    d->accountLayout->setContentsMargins(0, 0, 0, Ui::DesignSystem::layout().px12());


    d->teamsOwner->setFixedHeight(d->teamsOwner->viewportSizeHint().height());
    d->teamsMember->setFixedHeight(d->teamsMember->viewportSizeHint().height());

    d->addTeamButtonLayout->setContentsMargins(Ui::DesignSystem::layout().px12(), 0,
                                               Ui::DesignSystem::layout().px12(),
                                               Ui::DesignSystem::layout().px12());

    d->teamsContextMenu->setBackgroundColor(Ui::DesignSystem::color().background());
    d->teamsContextMenu->setTextColor(Ui::DesignSystem::color().onBackground());
}

} // namespace Ui

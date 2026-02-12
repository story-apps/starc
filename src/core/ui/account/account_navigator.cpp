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
    void updateCreatorSubtitleLabel();
    void updateStudioSubtitleLabel();
    void updateCampusSubtitleLabel();
    void updateCreditsSubtitleLabel();

    /**
     * @brief Задать возможность добавления команд
     */
    void setAccountTeamsCanBeAdded(bool _can);


    quint64 cloudStorageSize = 0;
    quint64 cloudStorageSizeUsed = 0;
    QDateTime proSubscriptionEnds;
    QDateTime creatorSubscriptionEnds;
    QDateTime studioSubscriptionEnds;
    QDateTime campusSubscriptionEnds;
    int creditsAvailable = 0;
    bool isTeamsCanBeAdded = false;

    //
    // Страница аккаунта
    //

    Widget* accountPage = nullptr;

    Tree* tree = nullptr;

    IconsMidLabel* freeTitleIcon = nullptr;
    ButtonLabel* freeTitle = nullptr;
    Subtitle2Label* freeSubtitle = nullptr;

    IconsMidLabel* proTitleIcon = nullptr;
    ButtonLabel* proTitle = nullptr;
    Subtitle2Label* proSubtitle = nullptr;
    Button* tryProButton = nullptr;
    Button* renewProSubscriptionButton = nullptr;

    IconsMidLabel* creatorTitleIcon = nullptr;
    ButtonLabel* creatorTitle = nullptr;
    ProgressBar* creatorSpaceStats = nullptr;
    Subtitle2Label* creatorSpaceInfo = nullptr;
    Subtitle2Label* creatorSubtitle = nullptr;
    Button* tryCreatorButton = nullptr;
    Button* renewCreatorSubscriptionButton = nullptr;

    IconsMidLabel* studioTitleIcon = nullptr;
    ButtonLabel* studioTitle = nullptr;
    ProgressBar* studioSpaceStats = nullptr;
    Subtitle2Label* studioSpaceInfo = nullptr;
    Subtitle2Label* studioSubtitle = nullptr;

    IconsMidLabel* campusTitleIcon = nullptr;
    ButtonLabel* campusTitle = nullptr;
    ProgressBar* campusSpaceStats = nullptr;
    Subtitle2Label* campusSpaceInfo = nullptr;
    Subtitle2Label* campusSubtitle = nullptr;

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
    , freeTitleIcon(new IconsMidLabel(accountPage))
    , freeTitle(new ButtonLabel(accountPage))
    , freeSubtitle(new Subtitle2Label(accountPage))
    //
    , proTitleIcon(new IconsMidLabel(accountPage))
    , proTitle(new ButtonLabel(accountPage))
    , proSubtitle(new Subtitle2Label(accountPage))
    , tryProButton(new Button(accountPage))
    , renewProSubscriptionButton(new Button(accountPage))
    //
    , creatorTitleIcon(new IconsMidLabel(accountPage))
    , creatorTitle(new ButtonLabel(accountPage))
    , creatorSpaceStats(new ProgressBar(accountPage))
    , creatorSpaceInfo(new Subtitle2Label(accountPage))
    , creatorSubtitle(new Subtitle2Label(accountPage))
    , tryCreatorButton(new Button(accountPage))
    , renewCreatorSubscriptionButton(new Button(accountPage))
    //
    , studioTitleIcon(new IconsMidLabel(accountPage))
    , studioTitle(new ButtonLabel(accountPage))
    , studioSpaceStats(new ProgressBar(accountPage))
    , studioSpaceInfo(new Subtitle2Label(accountPage))
    , studioSubtitle(new Subtitle2Label(accountPage))
    //
    , campusTitleIcon(new IconsMidLabel(accountPage))
    , campusTitle(new ButtonLabel(accountPage))
    , campusSpaceStats(new ProgressBar(accountPage))
    , campusSpaceInfo(new Subtitle2Label(accountPage))
    , campusSubtitle(new Subtitle2Label(accountPage))
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

    freeTitleIcon->setIcon(u8"\U000F0381");
    proTitleIcon->setIcon(u8"\U000F18BC");
    creatorTitleIcon->setIcon(u8"\U000F0674");
    studioTitleIcon->setIcon(u8"\U000F0381");
    campusTitleIcon->setIcon(u8"\U000F0474");
    creditsTitleIcon->setIcon(u8"\U000F133C");

    logoutButton->setIcon(u8"\U000F0343");

    accountLayout->setContentsMargins({});
    accountLayout->setSpacing(0);
    int row = 0;
    accountLayout->addWidget(tree, row++, 0, 1, 4);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(freeTitleIcon);
        layout->addWidget(freeTitle, 1);
        accountLayout->addLayout(layout, row++, 2);
    }
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
        layout->addWidget(creatorTitleIcon);
        layout->addWidget(creatorTitle, 1);
        accountLayout->addLayout(layout, row++, 2);
    }
    accountLayout->addWidget(creatorSpaceStats, row++, 2);
    accountLayout->addWidget(creatorSpaceInfo, row++, 2);
    accountLayout->addWidget(creatorSubtitle, row++, 2);
    accountLayout->addWidget(tryCreatorButton, row++, 2);
    accountLayout->addWidget(renewCreatorSubscriptionButton, row++, 2);
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
        layout->addWidget(campusTitleIcon);
        layout->addWidget(campusTitle, 1);
        accountLayout->addLayout(layout, row++, 2);
    }
    accountLayout->addWidget(campusSpaceStats, row++, 2);
    accountLayout->addWidget(campusSpaceInfo, row++, 2);
    accountLayout->addWidget(campusSubtitle, row++, 2);
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

void AccountNavigator::Implementation::updateCreatorSubtitleLabel()
{
    if (cloudStorageSize > 0) {
        creatorSpaceStats->setProgress(cloudStorageSizeUsed / static_cast<qreal>(cloudStorageSize));
        const qreal divider = 1024. * 1024. * 1024.;
        creatorSpaceInfo->setText(
            tr("Used %1 GB from %2 GB")
                .arg(QString::number(static_cast<qreal>(cloudStorageSizeUsed) / divider, 'f', 2),
                     QString::number(static_cast<qreal>(cloudStorageSize) / divider, 'f', 2)));
    }
    creatorSubtitle->setText(
        creatorSubscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(creatorSubscriptionEnds.toString("dd.MM.yyyy")));
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

void AccountNavigator::Implementation::updateCampusSubtitleLabel()
{
    if (cloudStorageSize > 0) {
        campusSpaceStats->setProgress(cloudStorageSizeUsed / static_cast<qreal>(cloudStorageSize));
        const qreal divider = 1024. * 1024. * 1024.;
        campusSpaceInfo->setText(
            tr("Used %1 GB from %2 GB")
                .arg(QString::number(static_cast<qreal>(cloudStorageSizeUsed) / divider, 'f', 2),
                     QString::number(static_cast<qreal>(cloudStorageSize) / divider, 'f', 2)));
    }
    campusSubtitle->setText(
        campusSubscriptionEnds.isNull()
            ? tr("Lifetime access")
            : tr("Active until %1").arg(campusSubscriptionEnds.toString("dd.MM.yyyy")));
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
    connect(d->tryCreatorButton, &Button::clicked, this,
            &AccountNavigator::tryCreatorForFreePressed);
    connect(d->renewCreatorSubscriptionButton, &Button::clicked, this,
            &AccountNavigator::renewCreatorPressed);
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
    d->tryCreatorButton->setEnabled(_connected);
    d->renewCreatorSubscriptionButton->setEnabled(_connected);
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
    d->freeTitleIcon->show();
    d->freeTitle->show();
    d->freeSubtitle->show();
    //
    d->proTitleIcon->show();
    d->proTitle->show();
    d->proSubtitle->hide();
    d->tryProButton->hide();
    d->renewProSubscriptionButton->hide();
    //
    d->creatorTitleIcon->show();
    d->creatorTitle->show();
    d->creatorSpaceStats->hide();
    d->creatorSpaceInfo->hide();
    d->creatorSubtitle->hide();
    d->tryCreatorButton->hide();
    d->renewCreatorSubscriptionButton->hide();
    //
    d->studioTitleIcon->hide();
    d->studioTitle->hide();
    d->studioSubtitle->hide();
    d->studioSpaceStats->hide();
    d->studioSpaceInfo->hide();
    d->studioSubtitle->hide();
    //
    d->campusTitleIcon->hide();
    d->campusTitle->hide();
    d->campusSubtitle->hide();
    d->campusSpaceStats->hide();
    d->campusSpaceInfo->hide();
    d->campusSubtitle->hide();

    //
    // А потом показываем, в зависимости от активных подписок и доступных опций
    //
    bool hasProSubscription = false;
    bool hasProLifetime = false;
    bool hasCreatorSubscription = false;
    bool hasCreatorLifetime = false;
    bool hasStudio = false;
    bool hasCampus = false;
    d->cloudStorageSize = _account.cloudStorageSize;
    d->cloudStorageSizeUsed = _account.cloudStorageSizeUsed;
    auto isAccountTeamsCanBeAdded = false;
    for (const auto& subscription : _account.subscriptions) {
        switch (subscription.type) {
        default:
        case Domain::SubscriptionType::Free: {
            break;
        }

        case Domain::SubscriptionType::ProMonthly: {
            hasProSubscription = true;

            d->freeTitleIcon->hide();
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            //
            d->proSubscriptionEnds = subscription.end;
            d->updateProSubtitleLabel();
            d->proSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::ProLifetime: {
            hasProLifetime = true;

            d->freeTitleIcon->hide();
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            //
            d->proSubscriptionEnds = {};
            d->updateProSubtitleLabel();
            d->proSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::CreatorMonthly: {
            hasCreatorSubscription = true;

            d->freeTitleIcon->hide();
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            //
            d->creatorSubscriptionEnds = subscription.end;
            d->updateCreatorSubtitleLabel();
            d->creatorSpaceStats->show();
            d->creatorSpaceInfo->show();
            d->creatorSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::CreatorLifetime: {
            hasCreatorLifetime = true;

            d->freeTitleIcon->hide();
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            d->proTitleIcon->hide();
            d->proTitle->hide();
            d->proSubtitle->hide();
            //
            d->creatorSubscriptionEnds = {};
            d->updateCreatorSubtitleLabel();
            d->creatorSpaceStats->show();
            d->creatorSpaceInfo->show();
            d->creatorSubtitle->show();
            break;
        }

        case Domain::SubscriptionType::Studio: {
            hasStudio = true;

            d->freeTitleIcon->hide();
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            d->proTitleIcon->hide();
            d->proTitle->hide();
            d->proSubtitle->hide();
            d->creatorTitleIcon->hide();
            d->creatorTitle->hide();
            d->creatorSubtitle->hide();
            d->creatorSpaceStats->hide();
            d->creatorSpaceInfo->hide();
            d->creatorSubtitle->hide();
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

        case Domain::SubscriptionType::Campus: {
            hasCampus = true;

            d->freeTitleIcon->hide();
            d->freeTitle->hide();
            d->freeSubtitle->hide();
            d->proTitleIcon->hide();
            d->proTitle->hide();
            d->proSubtitle->hide();
            d->creatorTitleIcon->hide();
            d->creatorTitle->hide();
            d->creatorSubtitle->hide();
            d->creatorSpaceStats->hide();
            d->creatorSpaceInfo->hide();
            d->creatorSubtitle->hide();
            //
            d->campusSubscriptionEnds = subscription.end;
            d->updateCampusSubtitleLabel();
            d->campusTitleIcon->show();
            d->campusTitle->show();
            d->campusSubtitle->show();
            d->campusSpaceStats->show();
            d->campusSpaceInfo->show();
            d->campusSubtitle->show();
            isAccountTeamsCanBeAdded = true;
            break;
        }
        }
    }
    d->setAccountTeamsCanBeAdded(isAccountTeamsCanBeAdded);

    //
    // Если у пользователя нет студийной или образовательной подписки
    //
    if (!hasStudio && !hasCampus) {
        //
        // ... настроим заголовки и кнопки в зависимости от доступных опций оплат
        //
        bool isProTrialAvailable = false;
        bool isCreatorTrialAvailable = false;
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

            case Domain::SubscriptionType::CreatorMonthly: {
                if (paymentOption.amount == 0) {
                    isCreatorTrialAvailable = true;
                    d->tryCreatorButton->show();
                } else {
                    d->renewCreatorSubscriptionButton->show();
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
        if (isCreatorTrialAvailable) {
            d->renewCreatorSubscriptionButton->hide();
        }

        //
        // Если активна только какая-то одна подписка, то оставим интерфейс только для неё
        //
        if (hasProSubscription && !hasCreatorSubscription && !hasCreatorLifetime) {
            d->creatorTitleIcon->hide();
            d->creatorTitle->hide();
            d->creatorSubtitle->hide();
            d->tryCreatorButton->hide();
            d->renewCreatorSubscriptionButton->hide();
        } else if (!hasProSubscription && !hasProLifetime && hasCreatorSubscription) {
            d->proTitleIcon->hide();
            d->proTitle->hide();
            d->proSubtitle->hide();
            d->tryProButton->hide();
            d->renewProSubscriptionButton->hide();
        }

        //
        // Если есть подписки навсегда, то просто оставляем заголовки, а кнопку продления скрываем
        //
        if (hasProLifetime) {
            d->tryProButton->hide();
            d->renewProSubscriptionButton->hide();
        }
        if (hasCreatorLifetime) {
            d->tryCreatorButton->hide();
            d->renewCreatorSubscriptionButton->hide();
        }
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
            teamRow->setData(ImageHelper::makeAvatar(
                                 _team.name, DesignSystem::font().body1(),
                                 DesignSystem::treeOneLineItem().iconSize().toSize(), Qt::white),
                             Qt::DecorationRole);
        } else {
            teamRow->setData(
                ImageHelper::makeAvatar(ImageHelper::imageFromBytes(_team.avatar),
                                        DesignSystem::treeOneLineItem().iconSize().toSize()),
                Qt::DecorationRole);
        }
        teamRow->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        return teamRow;
    };

    for (const auto& team : _teams) {
        if (team.isOwner()) {
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
    d->renewProSubscriptionButton->setText(tr("Activate"));
    d->creatorTitle->setText(tr("CREATOR version"));
    d->updateCreatorSubtitleLabel();
    d->tryCreatorButton->setText(tr("Try for free"));
    d->renewCreatorSubscriptionButton->setText(tr("Activate"));
    d->studioTitle->setText(tr("STUDIO version"));
    d->updateStudioSubtitleLabel();
    d->campusTitle->setText(tr("CAMPUS version"));
    d->updateCampusSubtitleLabel();
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
        = QMarginsF(DesignSystem::layout().px(18), DesignSystem::compactLayout().px(20),
                    DesignSystem::layout().px12(), DesignSystem::compactLayout().px4());
    for (auto icon : {
             d->freeTitleIcon,
             d->proTitleIcon,
             d->creatorTitleIcon,
             d->studioTitleIcon,
             d->campusTitleIcon,
             d->creditsTitleIcon,
         }) {
        icon->setBackgroundColor(DesignSystem::color().primary());
        icon->setTextColor(DesignSystem::color().onPrimary());
        icon->setContentsMargins(titleIconMargins.toMargins());
    }
    auto titleMargins = DesignSystem::label().margins();
    titleMargins.setLeft(0);
    titleMargins.setTop(DesignSystem::compactLayout().px24());
    titleMargins.setBottom(0);
    for (auto title : {
             d->freeTitle,
             d->proTitle,
             d->creatorTitle,
             d->studioTitle,
             d->campusTitle,
             d->creditsTitle,
         }) {
        title->setBackgroundColor(DesignSystem::color().primary());
        title->setTextColor(DesignSystem::color().onPrimary());
        title->setContentsMargins(titleMargins.toMargins());
    }

    auto subtitleMargins = titleMargins;
    subtitleMargins.setLeft(DesignSystem::layout().px(18));
    subtitleMargins.setBottom(DesignSystem::compactLayout().px12());
    subtitleMargins.setTop(DesignSystem::compactLayout().px2());
    for (auto subtitle : {
             d->freeSubtitle,
             d->proSubtitle,
             d->creatorSpaceInfo,
             d->creatorSubtitle,
             d->studioSpaceInfo,
             d->studioSubtitle,
             d->campusSpaceInfo,
             d->campusSubtitle,
             d->creditsSubtitle,
         }) {
        subtitle->setBackgroundColor(DesignSystem::color().primary());
        subtitle->setTextColor(ColorHelper::transparent(DesignSystem::color().onPrimary(),
                                                        DesignSystem::inactiveItemOpacity()));
        subtitle->setContentsMargins(subtitleMargins.toMargins());
    }

    auto captionMargins
        = QMarginsF(DesignSystem::layout().px(18), DesignSystem::compactLayout().px(20),
                    DesignSystem::layout().px12(), DesignSystem::compactLayout().px4());
    subtitleMargins.setTop(DesignSystem::compactLayout().px2());
    for (auto caption : std::vector<Widget*>{
             d->teamsOwnerLabel,
             d->teamsOwnerEmptyLabel,
             d->teamsMemberLabel,
             d->teamsMemberEmptyLabel,
         }) {
        caption->setBackgroundColor(DesignSystem::color().primary());
        caption->setTextColor(DesignSystem::color().onPrimary());
        caption->setContentsMargins(captionMargins.toMargins());
    }

    for (auto button : {
             d->tryProButton,
             d->renewProSubscriptionButton,
             d->tryCreatorButton,
             d->renewCreatorSubscriptionButton,
             d->buyCreditsButton,
             d->logoutButton,
             d->addTeamButton,
         }) {
        button->setBackgroundColor(DesignSystem::color().accent());
        button->setTextColor(DesignSystem::color().accent());
    }

    d->creatorSpaceStats->setBackgroundColor(DesignSystem::color().primary());
    d->creatorSpaceStats->setContentsMargins(
        DesignSystem::layout().px16(), DesignSystem::compactLayout().px16(),
        DesignSystem::layout().px24(), DesignSystem::compactLayout().px4());
    d->studioSpaceStats->setBackgroundColor(DesignSystem::color().primary());
    d->studioSpaceStats->setContentsMargins(
        DesignSystem::layout().px16(), DesignSystem::compactLayout().px16(),
        DesignSystem::layout().px24(), DesignSystem::compactLayout().px4());
    d->campusSpaceStats->setBackgroundColor(DesignSystem::color().primary());
    d->campusSpaceStats->setContentsMargins(
        DesignSystem::layout().px16(), DesignSystem::compactLayout().px16(),
        DesignSystem::layout().px24(), DesignSystem::compactLayout().px4());

    d->accountLayout->setVerticalSpacing(DesignSystem::compactLayout().px4());
    d->accountLayout->setColumnMinimumWidth(0, DesignSystem::layout().px4());
    d->accountLayout->setColumnMinimumWidth(3, DesignSystem::layout().px12());
    d->accountLayout->setContentsMargins(0, 0, 0, DesignSystem::layout().px12());


    d->teamsOwner->setFixedHeight(d->teamsOwner->viewportSizeHint().height());
    d->teamsMember->setFixedHeight(d->teamsMember->viewportSizeHint().height());

    d->addTeamButtonLayout->setContentsMargins(DesignSystem::layout().px12(), 0,
                                               DesignSystem::layout().px12(),
                                               DesignSystem::layout().px12());

    d->teamsContextMenu->setBackgroundColor(DesignSystem::color().background());
    d->teamsContextMenu->setTextColor(DesignSystem::color().onBackground());
}

} // namespace Ui

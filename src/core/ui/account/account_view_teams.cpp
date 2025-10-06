#include "account_view_teams.h"

#include "account_view_teams_sidebar.h"

#include <domain/starcloud_api.h>
#include <ui/design_system/design_system.h>
#include <ui/modules/avatar_generator/avatar_generator.h>
#include <ui/widgets/button/button.h>
#include <ui/widgets/card/card.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/label/label.h>
#include <ui/widgets/scroll_bar/scroll_bar.h>
#include <ui/widgets/splitter/splitter.h>
#include <ui/widgets/task_bar/task_bar.h>
#include <ui/widgets/text_field/text_field.h>
#include <ui/widgets/tree/tree.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/ui_helper.h>
#include <utils/validators/email_validator.h>

#include <QAction>
#include <QBoxLayout>
#include <QHeaderView>
#include <QScrollArea>
#include <QStandardItemModel>
#include <QTimeLine>


namespace Ui {

namespace {
enum {
    kRemoveActionIndex = 0,
};
} // namespace

class AccountViewTeams::Implementation
{
public:
    explicit Implementation(AccountViewTeams* _q);

    /**
     * @brief Обновить высоту списка участников команды
     */
    void updateMembersListHeight();

    /**
     * @brief Отобразить уведомление о подключении аккаунта к команде
     */
    void showInvitationNotification(const QString& _email);


    AccountViewTeams* q = nullptr;

    Domain::AccountInfo accountInfo;
    QVector<Domain::TeamInfo> teams;
    int currentTeamId = Domain::kInvalidId;
    QVector<Domain::TeamMemberInfo> currentTeamMembers;

    Widget* emptyPage = nullptr;
    Body1Label* emptyPageTitle = nullptr;

    Widget* teamPage = nullptr;
    QScrollArea* teamPageContent = nullptr;

    Card* addMemberCard = nullptr;
    QVBoxLayout* addMemberCardLayout = nullptr;
    H6Label* addMemberTitle = nullptr;
    Body1Label* addMemberSubtitle = nullptr;
    TextField* email = nullptr;
    TextField* nameForTeam = nullptr;
    Button* addMemberButton = nullptr;

    Card* membersCard = nullptr;
    QVBoxLayout* membersCardLayout = nullptr;
    H6Label* membersTitle = nullptr;
    Tree* members = nullptr;
    QStandardItemModel* membersModel = nullptr;

    ContextMenu* membersContextMenu = nullptr;

    AccountViewTeamsSidebar* sidebar = nullptr;

    Splitter* splitter = nullptr;
};

AccountViewTeams::Implementation::Implementation(AccountViewTeams* _q)
    : q(_q)
    , emptyPage(new Widget(_q))
    , emptyPageTitle(new Body1Label(emptyPage))
    , teamPage(new Widget(_q))
    , teamPageContent(UiHelper::createScrollArea(_q))
    , addMemberCard(new Card(_q))
    , addMemberCardLayout(new QVBoxLayout)
    , addMemberTitle(new H6Label(addMemberCard))
    , addMemberSubtitle(new Body1Label(addMemberCard))
    , email(new TextField(addMemberCard))
    , nameForTeam(new TextField(addMemberCard))
    , addMemberButton(new Button(addMemberCard))
    , membersCard(new Card(_q))
    , membersCardLayout(new QVBoxLayout)
    , membersTitle(new H6Label(membersCard))
    , members(new Tree(membersCard))
    , membersModel(new QStandardItemModel(members))
    , membersContextMenu(new ContextMenu(members))
    , sidebar(new AccountViewTeamsSidebar(_q))
    , splitter(new Splitter(_q))
{
    emptyPageTitle->setAlignment(Qt::AlignCenter);
    auto emptyPageLayout = new QVBoxLayout;
    emptyPageLayout->setContentsMargins({});
    emptyPageLayout->setSpacing(0);
    emptyPageLayout->addStretch();
    emptyPageLayout->addWidget(emptyPageTitle);
    emptyPageLayout->addStretch();
    emptyPage->setLayout(emptyPageLayout);


    email->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    email->setCapitalizeWords(false);
    nameForTeam->setSpellCheckPolicy(SpellCheckPolicy::Manual);
    nameForTeam->setCapitalizeWords(false);
    members->setModel(membersModel);
    members->setContextMenuPolicy(Qt::CustomContextMenu);

    addMemberCardLayout->setContentsMargins({});
    addMemberCardLayout->setSpacing(0);
    addMemberCardLayout->addWidget(addMemberTitle);
    addMemberCardLayout->addWidget(addMemberSubtitle);
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addWidget(email, 2);
        layout->addWidget(nameForTeam, 3, Qt::AlignTop);
        addMemberCardLayout->addLayout(layout);
    }
    {
        auto layout = new QHBoxLayout;
        layout->setContentsMargins({});
        layout->setSpacing(0);
        layout->addStretch();
        layout->addWidget(addMemberButton);
        addMemberCardLayout->addLayout(layout);
    }
    addMemberCard->setContentLayout(addMemberCardLayout);


    membersCardLayout->setContentsMargins({});
    membersCardLayout->setSpacing(0);
    membersCardLayout->addWidget(membersTitle);
    membersCardLayout->addWidget(members, 1, Qt::AlignTop);
    membersCard->setContentLayout(membersCardLayout);
    members->setContextMenuPolicy(Qt::CustomContextMenu);

    auto contentWidgetLayout = qobject_cast<QVBoxLayout*>(teamPageContent->widget()->layout());
    contentWidgetLayout->setContentsMargins({});
    contentWidgetLayout->setSpacing(0);
    contentWidgetLayout->addWidget(addMemberCard);
    contentWidgetLayout->addWidget(membersCard);
    contentWidgetLayout->addStretch();

    auto layout = new QVBoxLayout;
    layout->setContentsMargins({});
    layout->setSpacing(0);
    layout->addWidget(teamPageContent);
    teamPage->setLayout(layout);

    splitter->setWidgets(teamPage, sidebar);
}

void AccountViewTeams::Implementation::updateMembersListHeight()
{
    members->setFixedHeight(members->viewportSizeHint().height());
}

void AccountViewTeams::Implementation::showInvitationNotification(const QString& _email)
{
    TaskBar::addTask(_email);
    TaskBar::setTaskTitle(_email, tr("Invitation sent to the %1").arg(_email));
    auto taskTimeline = new QTimeLine;
    taskTimeline->setDuration(1200);
    taskTimeline->setEasingCurve(QEasingCurve::OutQuad);
    connect(taskTimeline, &QTimeLine::valueChanged, q,
            [_email](qreal _value) { TaskBar::setTaskProgress(_email, 100.0 * _value); });
    connect(taskTimeline, &QTimeLine::finished, taskTimeline, [_email, taskTimeline] {
        TaskBar::finishTask(_email);
        taskTimeline->deleteLater();
    });
    taskTimeline->start();
}


// ****


AccountViewTeams::AccountViewTeams(QWidget* _parent)
    : StackWidget(_parent)
    , d(new Implementation(this))
{
    setFocusPolicy(Qt::StrongFocus);

    setCurrentWidget(d->emptyPage);
    addWidget(d->splitter);

    auto removeMemberAction = new QAction(d->membersContextMenu);
    removeMemberAction->setIconText(u8"\U000F01B4");
    d->membersContextMenu->setActions({ removeMemberAction });

    connect(d->email, &TextField::textChanged, d->email, &TextField::clearError);
    connect(d->email, &TextField::enterPressed, d->addMemberButton, &Button::click);
    connect(d->nameForTeam, &TextField::enterPressed, d->addMemberButton, &Button::click);
    connect(d->addMemberButton, &Button::clicked, this, [this] {
        if (d->currentTeamId == Domain::kInvalidId) {
            return;
        }

        if (!EmailValidator::isValid(d->email->text())) {
            d->email->setError(tr("Email invalid"));
            return;
        }

        const auto email = d->email->text();
        d->email->clear();
        const auto nameForTeam = d->nameForTeam->text();
        d->nameForTeam->clear();

        d->showInvitationNotification(email);

        emit addMemberPressed(d->currentTeamId, email, nameForTeam, {});
    });
    connect(d->members, &Tree::currentIndexChanged, this, [this](const QModelIndex& _index) {
        d->sidebar->setTeamMember(d->currentTeamMembers.at(_index.row()));
    });
    connect(d->members, &Tree::customContextMenuRequested, this, [this] {
        if (d->currentTeamId == Domain::kInvalidId) {
            return;
        }

        //
        // Себя нельзя удалять из команды
        //
        if (d->members->currentIndex().row() == 0) {
            return;
        }

        d->membersContextMenu->showContextMenu(QCursor::pos());
    });
    connect(removeMemberAction, &QAction::triggered, this, [this] {
        const auto email = d->members->currentIndex().data(Qt::UserRole).toString();
        emit removeMemberPressed(d->currentTeamId, email);
    });
    connect(AvatarGenerator::instance(), &AvatarGenerator::avatarUpdated, this,
            [this](const QString& _email, const QPixmap& _avatar) {
                for (int row = 0; row < d->membersModel->rowCount(); ++row) {
                    const auto collabroatorIndex = d->membersModel->index(row, 0);
                    if (collabroatorIndex.data(Qt::UserRole).toString() == _email) {
                        d->membersModel->setData(collabroatorIndex, _avatar, Qt::DecorationRole);
                        break;
                    }
                }
            });
    connect(d->sidebar, &AccountViewTeamsSidebar::teamMemberChanged, this,
            [this](const QString& _email, const QString& _nameForTeam, int _role,
                   bool _hasAccessToAllProjects, bool _allowGrantAccessToProjects) {
                emit changeMemberRequested(d->currentTeamId, _email, _nameForTeam, _role,
                                           _hasAccessToAllProjects, _allowGrantAccessToProjects);
            });
}

AccountViewTeams::~AccountViewTeams() = default;

void AccountViewTeams::setConnected(bool _connected)
{
    d->addMemberCard->setEnabled(_connected);
    d->sidebar->setEnabled(_connected);
}

void AccountViewTeams::setAccountInfo(const Domain::AccountInfo& _accountInfo)
{
    d->accountInfo = _accountInfo;
}

void AccountViewTeams::setTeams(const QVector<Domain::TeamInfo>& _teams)
{
    d->teams = _teams;

    if (d->currentTeamId != Domain::kInvalidId) {
        showTeam(d->currentTeamId);
    }
}

void AccountViewTeams::showEmptyPage()
{
    d->currentTeamId = Domain::kInvalidId;

    setCurrentWidget(d->emptyPage);
}

void AccountViewTeams::showTeam(int _teamId)
{
    const auto isTeamChanged = d->currentTeamId != _teamId || !d->members->currentIndex().isValid();
    const auto memberToSelectRow = isTeamChanged ? 0 : d->members->currentIndex().row();
    if (!isTeamChanged) {
        d->members->blockSignals(true);
    }

    d->currentTeamId = _teamId;
    d->currentTeamMembers.clear();

    if (d->teams.isEmpty()) {
        return;
    }

    QVector<Domain::TeamMemberInfo> members;
    for (const auto& team : std::as_const(d->teams)) {
        if (team.id != d->currentTeamId) {
            continue;
        }

        //
        // Собственно список участников
        //
        members = team.members;
        //
        // Текущий
        //
        members.prepend({
            d->accountInfo.name,
            d->accountInfo.email,
            team.teamRole,
            team.hasAccessToAllProjects,
            team.allowGrantAccessToProjects,
            team.nameForTeam,
            team.color,
        });

        //
        // Настроим возможность добавления участников в команду
        //
        const auto isOwner = team.teamRole == 0;
        d->addMemberCard->setVisible(isOwner);
        //
        // ... управления командой
        //
        d->members->setContextMenuPolicy(isOwner ? Qt::CustomContextMenu : Qt::NoContextMenu);
        //
        // ... и управления параметрами участников
        //
        d->sidebar->setEnabled(isOwner);
        d->sidebar->setVisible(isOwner);
        d->splitter->setSizes(isOwner ? QVector<int>{ 7, 2 } : QVector<int>{ 1, 0 });
        d->splitter->setHidePanelButtonAvailable(true, false);

        break;
    }
    d->currentTeamMembers = members;

    d->membersModel->clear();
    auto nameItem = [](const Domain::TeamMemberInfo& _member) {
        const auto displayName = !_member.nameForTeam.isEmpty()
            ? QString("%1 (%2)").arg(_member.nameForTeam, _member.email)
            : (!_member.email.startsWith(_member.name)
                   ? QString("%1 (%2)").arg(_member.name, _member.email)
                   : _member.email);
        auto item = new QStandardItem(displayName);
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        item->setData(_member.email, Qt::UserRole);
        item->setData(AvatarGenerator::avatar(_member.name, _member.email), Qt::DecorationRole);
        return item;
    };
    auto roleItem = [](const Domain::TeamMemberInfo& _member) {
        auto roleToString = [](int _role) {
            switch (_role) {
            case 0: {
                return tr("Owner");
            }
            case 1: {
                return tr("Member");
            }
            default: {
                return QString();
            }
            }
        };
        auto item = new QStandardItem(roleToString(_member.role));
        item->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
        return item;
    };

    for (const auto& member : std::as_const(members)) {
        const QList<QStandardItem*> memberItems = {
            nameItem(member),
            roleItem(member),
        };
        d->membersModel->appendRow(memberItems);
    }

    d->members->setMinimumHeight((d->membersModel->rowCount() + 0.5)
                                 * DesignSystem::treeOneLineItem().height());
    d->members->headerView()->setSectionResizeMode(1, QHeaderView::ResizeToContents);

    d->updateMembersListHeight();

    d->members->setCurrentIndex(d->membersModel->index(memberToSelectRow, 0));
    if (!isTeamChanged) {
        d->members->blockSignals(false);
    }

    setCurrentWidget(d->splitter);
}

void AccountViewTeams::updateTranslations()
{
    d->emptyPageTitle->setText(tr("Here will be a list of your teammates."));
    d->addMemberTitle->setText(tr("Invite the team"));
    d->addMemberSubtitle->setText(tr("Add people who you’d like to join the team"));
    d->email->setLabel(tr("Email"));
    d->nameForTeam->setLabel(tr("Name used for team"));
    d->addMemberButton->setText(tr("Add"));
    d->membersTitle->setText(tr("Team members"));
    d->membersContextMenu->actions().at(kRemoveActionIndex)->setText(tr("Remove"));
}

void AccountViewTeams::designSystemChangeEvent(DesignSystemChangeEvent* _event)
{
    StackWidget::designSystemChangeEvent(_event);

    setBackgroundColor(DesignSystem::color().surface());
    d->emptyPage->setBackgroundColor(DesignSystem::color().surface());
    d->teamPage->setBackgroundColor(DesignSystem::color().surface());

    d->teamPageContent->widget()->layout()->setContentsMargins(
        QMarginsF(DesignSystem::layout().px24()
                      + (isLeftToRight() ? 0.0 : DesignSystem::scrollBar().minimumSize()),
                  DesignSystem::compactLayout().topContentMargin(),
                  DesignSystem::layout().px24()
                      + (isRightToLeft() ? 0.0 : DesignSystem::scrollBar().minimumSize()),
                  DesignSystem::compactLayout().px24())
            .toMargins());

    d->addMemberCard->setBackgroundColor(DesignSystem::color().background());
    d->membersCard->setBackgroundColor(DesignSystem::color().background());

    auto labelMargins = DesignSystem::label().margins().toMargins();
    for (auto label : std::vector<Widget*>{
             d->emptyPageTitle,
             d->addMemberTitle,
             d->addMemberSubtitle,
             d->membersTitle,
         }) {
        label->setBackgroundColor(DesignSystem::color().background());
        label->setTextColor(DesignSystem::color().onBackground());
        label->setContentsMargins(labelMargins);
    }
    d->emptyPageTitle->setBackgroundColor(DesignSystem::color().surface());
    labelMargins.setTop(DesignSystem::compactLayout().px(32));
    d->membersTitle->setContentsMargins(labelMargins);
    labelMargins.setTop(0);
    labelMargins.setBottom(DesignSystem::compactLayout().px16());
    d->addMemberSubtitle->setContentsMargins(labelMargins);

    for (auto textField : std::vector<TextField*>{
             d->email,
             d->nameForTeam,
         }) {
        textField->setBackgroundColor(DesignSystem::color().onBackground());
        textField->setTextColor(DesignSystem::color().onBackground());
    }
    d->nameForTeam->setCustomMargins({ isLeftToRight() ? 0 : DesignSystem::layout().px24(), 0,
                                       isLeftToRight() ? DesignSystem::layout().px24() : 0, 0 });

    d->addMemberButton->setBackgroundColor(DesignSystem::color().accent());
    d->addMemberButton->setTextColor(DesignSystem::color().accent());
    d->addMemberButton->setContentsMargins(
        isLeftToRight() ? 0 : DesignSystem::layout().px16(), DesignSystem::compactLayout().px16(),
        isLeftToRight() ? DesignSystem::layout().px16() : 0, DesignSystem::layout().px16());

    d->members->setBackgroundColor(DesignSystem::color().background());
    d->members->setTextColor(DesignSystem::color().onBackground());

    d->membersContextMenu->setBackgroundColor(DesignSystem::color().background());
    d->membersContextMenu->setTextColor(DesignSystem::color().onBackground());

    d->splitter->setBackgroundColor(DesignSystem::color().background());

    d->updateMembersListHeight();
}

} // namespace Ui

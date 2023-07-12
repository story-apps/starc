#include "projects_manager.h"

#include "projects_model.h"
#include "projects_model_project_item.h"
#include "projects_model_team_item.h"

#include <data_layer/database.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <ui/design_system/design_system.h>
#include <ui/projects/create_project_dialog.h>
#include <ui/projects/projects_navigator.h>
#include <ui/projects/projects_tool_bar.h>
#include <ui/projects/projects_view.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/dialog/dialog.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/helpers/platform_helper.h>
#include <utils/logging.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QCryptographicHash>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMenu>
#include <QPointer>
#include <QStandardPaths>
#include <QTimer>
#include <QWidget>


namespace ManagementLayer {

class ProjectsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    QString projectPosterPath(const QString& _projectPath);

    QString newProjectPath(const QString& _projectName, const QString& _projectPath);
    QString newProjectPath(const QString& _projectName);


    bool isConnected = false;
    bool isUserAuthorized = false;
    bool canCreateCloudProject = false;
    QVector<Domain::TeamInfo> teamsForProjects;

    BusinessLayer::ProjectsModel* projects = nullptr;
    BusinessLayer::ProjectsModelProjectItem* currentProject = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::ProjectsToolBar* toolBar = nullptr;
    Ui::ProjectsNavigator* navigator = nullptr;
    Ui::ProjectsView* view = nullptr;

    QPointer<Ui::CreateProjectDialog> createProjectDialog;

    bool isCloudProjectNameChanged = false;
    bool isCloudProjectLoglineChnaged = false;
    bool isCloudProjectCoverChnaged = false;
    Debouncer cloudProjectChangeDebouncer;
};

ProjectsManager::Implementation::Implementation(QWidget* _parent)
    : projects(new BusinessLayer::ProjectsModel(_parent))
    , topLevelWidget(_parent)
    , toolBar(new Ui::ProjectsToolBar(_parent))
    , navigator(new Ui::ProjectsNavigator(_parent))
    , view(new Ui::ProjectsView(_parent))
    , cloudProjectChangeDebouncer(500)
{
    toolBar->hide();

    navigator->hide();

    view->setProjects(projects);
    view->hide();
}

QString ProjectsManager::Implementation::projectPosterPath(const QString& _projectPath)
{
    const QString posterDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/thumbnails/projects/";
    QDir::root().mkpath(posterDir);

    const QString posterName
        = QCryptographicHash::hash(_projectPath.toUtf8(), QCryptographicHash::Md5).toHex();
    return posterDir + posterName;
}

QString ProjectsManager::Implementation::newProjectPath(const QString& _projectName,
                                                        const QString& _projectPath)
{
    const auto projectPathPrefix
        = _projectPath + "/" + PlatformHelper::systemSavebleFileName(_projectName);
    auto projectPath = projectPathPrefix + BusinessLayer::ProjectsModelProjectItem::extension();
    //
    // Ситуация, что файл с таким названием уже существует крайне редка, хотя и
    // гипотетически возможна
    //
    if (QFileInfo::exists(projectPath)) {
        //
        // ... в таком случае добавляем метку с датой и временем создания файла, чтобы имена
        //     не пересекались
        //
        projectPath = projectPathPrefix + "_"
            + QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss")
            + BusinessLayer::ProjectsModelProjectItem::extension();
    }

    return projectPath;
}

QString ProjectsManager::Implementation::newProjectPath(const QString& _projectName)
{
    return newProjectPath(_projectName,
                          settingsValue(DataStorageLayer::kProjectSaveFolderKey).toString());
}


// **


ProjectsManager::ProjectsManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent)
    , d(new Implementation(_parentWidget))
{
    connect(d->toolBar, &Ui::ProjectsToolBar::menuPressed, this, &ProjectsManager::menuRequested);

    connect(d->navigator, &Ui::ProjectsNavigator::createProjectPressed, this,
            &ProjectsManager::createProjectRequested);
    connect(d->view, &Ui::ProjectsView::createProjectPressed, this,
            &ProjectsManager::createProjectRequested);

    connect(d->navigator, &Ui::ProjectsNavigator::openProjectPressed, this,
            &ProjectsManager::openProjectRequested);
    connect(d->view, &Ui::ProjectsView::openProjectPressed, this,
            &ProjectsManager::openProjectRequested);
    connect(d->view, &Ui::ProjectsView::openProjectRequested, this,
            [this](BusinessLayer::ProjectsModelProjectItem* _project) {
                if (_project->isLocal()) {
                    emit openLocalProjectRequested(_project->path());
                } else {
                    emit openCloudProjectRequested(_project->id(), _project->path());
                }
            });
    connect(
        d->view, &Ui::ProjectsView::projectContextMenuRequested, this,
        [this](BusinessLayer::ProjectsModelProjectItem* _project) {
            QVector<QAction*> actions;
            //
            // Действия над локальным проектом
            //
            if (_project->isLocal()) {
                auto moveToCloudAction = new QAction;
                moveToCloudAction->setIconText(u8"\U000F0167");
                moveToCloudAction->setText(tr("Move to the cloud"));
                connect(moveToCloudAction, &QAction::triggered, this, [this, _project] {
                    //
                    // Если пользователь не авторизован, предлагаем авторизоваться
                    //
                    if (!d->isUserAuthorized) {
                        auto dialog = new Dialog(d->view->topLevelWidget());
                        dialog->setContentMaximumWidth(Ui::DesignSystem::dialog().maximumWidth());
                        dialog->showDialog(
                            {}, tr("To move a project to the cloud, you should be authorized."),
                            { { 0, tr("Maybe later"), Dialog::RejectButton },
                              { 1, tr("Sign in"), Dialog::AcceptButton } });
                        QObject::connect(dialog, &Dialog::finished, this,
                                         [this, dialog](const Dialog::ButtonInfo& _presedButton) {
                                             dialog->hideDialog();
                                             if (_presedButton.type == Dialog::AcceptButton) {
                                                 emit signInRequested();
                                             }
                                         });
                        QObject::connect(dialog, &Dialog::disappeared, dialog,
                                         &Dialog::deleteLater);
                    }
                    //
                    // Если у пользователя нет активной подписки на CLOUD версию,
                    // покажем соответствующее уведомление с предложением обновиться
                    //
                    else if (!d->canCreateCloudProject) {
                        auto dialog = new Dialog(d->view->topLevelWidget());
                        dialog->setContentMaximumWidth(Ui::DesignSystem::dialog().maximumWidth());
                        dialog->showDialog({},
                                           tr("To move a project to the cloud, you need to upgrade "
                                              "to the CLOUD version."),
                                           { { 0, tr("Maybe later"), Dialog::RejectButton },
                                             { 1, tr("Upgrade"), Dialog::AcceptButton } });
                        QObject::connect(dialog, &Dialog::finished, this,
                                         [this, dialog](const Dialog::ButtonInfo& _presedButton) {
                                             dialog->hideDialog();
                                             if (_presedButton.type == Dialog::AcceptButton) {
                                                 emit renewTeamSubscriptionRequested();
                                             }
                                         });
                        QObject::connect(dialog, &Dialog::disappeared, dialog,
                                         &Dialog::deleteLater);
                    }
                    //
                    // Если проект может быть создан в облаке, то делаем это
                    //
                    else {
                        emit createCloudProjectRequested(_project->name(), _project->path(),
                                                         Domain::kInvalidId);
                    }
                });
                actions.append(moveToCloudAction);
                //
                auto showInFolderAction = new QAction;
                showInFolderAction->setSeparator(true);
                showInFolderAction->setIconText(u8"\U000F178A");
                showInFolderAction->setText(tr("Show in folder"));
                connect(showInFolderAction, &QAction::triggered, this,
                        [_project] { PlatformHelper::showInGraphicalShell(_project->path()); });
                actions.append(showInFolderAction);
                //
                auto hideFromRecentAction = new QAction;
                hideFromRecentAction->setIconText(u8"\U000F0209");
                hideFromRecentAction->setText(tr("Hide from recent list"));
                connect(hideFromRecentAction, &QAction::triggered, this, [this, _project] {
                    auto dialog = new Dialog(d->view->topLevelWidget());
                    constexpr int cancelButtonId = 0;
                    constexpr int hideButtonId = 1;
                    dialog->showDialog(
                        {}, tr("Do you really want to hide this project from the recent list?"),
                        { { cancelButtonId, tr("No"), Dialog::RejectButton },
                          { hideButtonId, tr("Yes, hide"), Dialog::AcceptButton } });
                    connect(dialog, &Dialog::finished, this,
                            [this, _project, cancelButtonId,
                             dialog](const Dialog::ButtonInfo& _buttonInfo) {
                                dialog->hideDialog();

                                //
                                // Пользователь передумал скрывать
                                //
                                if (_buttonInfo.id == cancelButtonId) {
                                    return;
                                }

                                //
                                // Если таки хочет, то скрываем проект
                                //
                                if (currentProject() == _project) {
                                    emit closeCurrentProjectRequested();
                                }
                                d->projects->removeItem(_project);
                            });
                    connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
                });
                actions.append(hideFromRecentAction);
                //
                auto removeProjectAction = new QAction;
                removeProjectAction->setIconText(u8"\U000F01B4");
                removeProjectAction->setText(tr("Remove project"));
                connect(removeProjectAction, &QAction::triggered, this, [this, _project] {
                    auto dialog = new Dialog(d->view->topLevelWidget());
                    constexpr int cancelButtonId = 0;
                    constexpr int hideButtonId = 1;
                    dialog->showDialog(
                        {}, tr("Do you really want to remove this project from the computer?"),
                        { { cancelButtonId, tr("No"), Dialog::RejectButton },
                          { hideButtonId, tr("Yes, remove"), Dialog::AcceptButton } });
                    connect(dialog, &Dialog::finished, this,
                            [this, _project, cancelButtonId,
                             dialog](const Dialog::ButtonInfo& _buttonInfo) {
                                dialog->hideDialog();

                                //
                                // Пользователь передумал удалять
                                //
                                if (_buttonInfo.id == cancelButtonId) {
                                    return;
                                }

                                //
                                // Если таки хочет, то удаляем проект
                                //
                                if (currentProject() == _project) {
                                    emit closeCurrentProjectRequested();
                                }
                                QFile::remove(_project->path());
                                d->projects->removeItem(_project);
                            });
                    connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
                });
                actions.append(removeProjectAction);
            }
            //
            // Действия над облачным проектом
            //
            else {
                //
                // Если пользователь может изменть проект
                //
                if (_project->editingMode() == DocumentEditingMode::Edit) {
                    auto saveLocallyAction = new QAction;
                    saveLocallyAction->setIconText(u8"\U000F0162");
                    saveLocallyAction->setText(tr("Save to local file"));
                    connect(saveLocallyAction, &QAction::triggered, this, [this, _project] {
                        emit createLocalProjectRequested(_project->name(),
                                                         d->newProjectPath(_project->name()),
                                                         _project->path());
                    });
                    actions.append(saveLocallyAction);
                }

                //
                // Если пользователь владеет проектом
                //
                if (_project->isOwner()) {
                    auto removeProjectAction = new QAction;
                    removeProjectAction->setSeparator(!actions.isEmpty());
                    removeProjectAction->setIconText(u8"\U000F01B4");
                    removeProjectAction->setText(tr("Remove project"));
                    connect(removeProjectAction, &QAction::triggered, this, [this, _project] {
                        auto dialog = new Dialog(d->view->topLevelWidget());
                        constexpr int cancelButtonId = 0;
                        constexpr int removeButtonId = 1;
                        dialog->showDialog(
                            {}, tr("Do you really want to remove this project?"),
                            { { cancelButtonId, tr("No"), Dialog::RejectButton },
                              { removeButtonId, tr("Yes, remove"), Dialog::AcceptButton } });
                        connect(dialog, &Dialog::finished, this,
                                [this, _project, cancelButtonId,
                                 dialog](const Dialog::ButtonInfo& _buttonInfo) {
                                    dialog->hideDialog();

                                    //
                                    // Пользователь передумал удалять
                                    //
                                    if (_buttonInfo.id == cancelButtonId) {
                                        return;
                                    }
                                    //
                                    // Если таки хочет, то уведомляем, чтобы удалить проект на
                                    // сервере
                                    //
                                    emit removeCloudProjectRequested(_project->id());
                                });
                        connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
                    });
                    actions.append(removeProjectAction);
                }
                //
                // Если пользователь не владеет проектом
                //
                else {
                    auto unsubscribeAction = new QAction;
                    unsubscribeAction->setSeparator(!actions.isEmpty());
                    unsubscribeAction->setIconText(u8"\U000F01B4");
                    unsubscribeAction->setText(tr("Unsubscribe"));
                    connect(unsubscribeAction, &QAction::triggered, this, [this, _project] {
                        auto dialog = new Dialog(d->view->topLevelWidget());
                        constexpr int cancelButtonId = 0;
                        constexpr int unsubscribeButtonId = 1;
                        dialog->showDialog(
                            {}, tr("Do you really want to unsubscribe from this project?"),
                            { { cancelButtonId, tr("No"), Dialog::RejectButton },
                              { unsubscribeButtonId, tr("Yes, unsubscribe"),
                                Dialog::AcceptButton } });
                        connect(dialog, &Dialog::finished, this,
                                [this, _project, cancelButtonId,
                                 dialog](const Dialog::ButtonInfo& _buttonInfo) {
                                    dialog->hideDialog();

                                    //
                                    // Пользователь передумал отписываться
                                    //
                                    if (_buttonInfo.id == cancelButtonId) {
                                        return;
                                    }
                                    //
                                    // Если таки хочет, то уведомляем, чтобы отписаться от
                                    // проекта на сервере
                                    //
                                    emit unsubscribeFromCloudProjectRequested(_project->id());
                                });
                        connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
                    });
                    actions.append(unsubscribeAction);
                }
            }

            auto menu = new ContextMenu(d->view);
            menu->setActions(actions);
            menu->setBackgroundColor(Ui::DesignSystem::color().background());
            menu->setTextColor(Ui::DesignSystem::color().onBackground());
            connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

            menu->showContextMenu(QCursor::pos());
        });
    connect(&d->cloudProjectChangeDebouncer, &Debouncer::gotWork, this, [this] {
        if (!d->currentProject->isRemote()) {
            return;
        }

        const auto name = d->isCloudProjectNameChanged ? d->currentProject->name() : QString();
        const auto logline
            = d->isCloudProjectLoglineChnaged ? d->currentProject->logline() : QString();
        const auto cover = d->isCloudProjectCoverChnaged
            ? ImageHelper::bytesFromImage(d->currentProject->poster())
            : QByteArray();

        d->isCloudProjectCoverChnaged = false;
        d->isCloudProjectLoglineChnaged = false;
        d->isCloudProjectCoverChnaged = false;

        emit updateCloudProjectRequested(d->currentProject->id(), name, logline, cover);
    });

    connect(d->navigator, &Ui::ProjectsNavigator::helpPressed, this, [] {
        QString helpUrl;
        switch (QLocale().language()) {
        case QLocale::Russian:
        case QLocale::Belarusian:
        case QLocale::Ukrainian: {
            helpUrl = "https://starc.app/ru/help/";
            break;
        }

        default: {
            helpUrl = "https://starc.app/help/";
            break;
        }
        }

        QDesktopServices::openUrl(helpUrl);
    });
}

ProjectsManager::~ProjectsManager() = default;

QWidget* ProjectsManager::toolBar() const
{
    return d->toolBar;
}

QWidget* ProjectsManager::navigator() const
{
    return d->navigator;
}

QWidget* ProjectsManager::view() const
{
    return d->view;
}

void ProjectsManager::setHasUnreadNotifications(bool _hasUnreadNotifications)
{
    d->toolBar->setBadgeVisible(d->toolBar->actions().constFirst(), _hasUnreadNotifications);
}

void ProjectsManager::loadProjects()
{
    const auto projectsData = settingsValue(DataStorageLayer::kApplicationProjectsKey);
    const auto projectsJson
        = QJsonDocument::fromJson(QByteArray::fromHex(projectsData.toByteArray()));

    auto readProject = [](const QJsonObject& _json) -> BusinessLayer::ProjectsModelItem* {
        //
        // Если локальный файл проекта удалили, то не добавляем его в список недавних проектов
        //
        const auto isRemote = _json.contains("id");
        const auto projectPath = _json["path"].toString();
        if (!isRemote && !QFileInfo::exists(projectPath)) {
            return nullptr;
        }

        auto project = new BusinessLayer::ProjectsModelProjectItem;
        if (isRemote) {
            project->setId(_json["id"].toInt());
            project->setTeamId(_json["team_id"].toInt());
            project->setOwner(_json["is_owner"].toBool());
            project->setEditingMode(static_cast<DocumentEditingMode>(_json["role"].toInt()));

            QHash<QUuid, DocumentEditingMode> permissions;
            const auto permissionsJson = _json["permissions"].toArray();
            for (const auto& documentAccessJson : permissionsJson) {
                const auto documentAccess = documentAccessJson.toObject();
                permissions[documentAccess["uuid"].toString()]
                    = static_cast<DocumentEditingMode>(documentAccess["role"].toInt());
            }
            project->setEditingPermissions(permissions);
        }
        project->setProjectType(static_cast<BusinessLayer::ProjectType>(_json["type"].toInt()));
        project->setUuid(_json["uuid"].toString());
        project->setName(_json["name"].toString());
        project->setLogline(_json["logline"].toString());
        project->setPath(_json["path"].toString());
        project->setPosterPath(_json["poster_path"].toString());
        project->setLastEditTime(
            QDateTime::fromString(_json["last_edit_time"].toString(), Qt::ISODateWithMs));
        project->setCanAskAboutSwitch(_json["can_ask"].toBool());
        return project;
    };

    QVector<BusinessLayer::ProjectsModelItem*> teamsAndProjects;
    for (const auto& itemJsonValue : projectsJson.array()) {
        const auto itemJson = itemJsonValue.toObject();

        //
        // Команда
        //
        if (itemJson.contains("is_team") && itemJson["is_team"].toBool() == true) {
            auto team = new BusinessLayer::ProjectsModelTeamItem;
            team->setId(itemJson["id"].toInt());
            team->setName(itemJson["name"].toString());
            team->setDescription(itemJson["description"].toString());
            team->setOpened(itemJson["is_opened"].toBool());
            for (const auto& projectJsonValue : itemJson["projects"].toArray()) {
                const auto projectJson = projectJsonValue.toObject();
                auto project = readProject(projectJson);
                if (project == nullptr) {
                    continue;
                }
                team->appendItem(project);
            }
            teamsAndProjects.append(team);
            continue;
        }

        //
        // Проект
        //
        auto project = readProject(itemJson);
        if (project == nullptr) {
            continue;
        }
        teamsAndProjects.append(project);
    }
    d->projects->appendItems(teamsAndProjects);
}

void ProjectsManager::saveProjects()
{
    auto projectItemJson = [](BusinessLayer::ProjectsModelItem* _item) {
        if (_item->type() != BusinessLayer::ProjectsModelItemType::Project) {
            return QJsonObject();
        }

        const auto projectItem = static_cast<BusinessLayer::ProjectsModelProjectItem*>(_item);
        QJsonObject projectJson;
        projectJson["is_team"] = false;
        if (projectItem->isRemote()) {
            projectJson["id"] = projectItem->id();
            projectJson["team_id"] = projectItem->teamId();
            projectJson["is_owner"] = projectItem->isOwner();
            projectJson["role"] = static_cast<int>(projectItem->editingMode());

            QJsonArray permissionsJson;
            const auto permissions = projectItem->editingPermissions();
            for (auto iter = permissions.begin(); iter != permissions.end(); ++iter) {
                QJsonObject documentAccess;
                documentAccess["uuid"] = iter.key().toString();
                documentAccess["role"] = static_cast<int>(iter.value());
                permissionsJson.append(documentAccess);
            }
            projectJson["permissions"] = permissionsJson;
        }
        projectJson["type"] = static_cast<int>(projectItem->projectType());
        projectJson["uuid"] = projectItem->uuid().toString();
        projectJson["name"] = projectItem->name();
        projectJson["logline"] = projectItem->logline();
        projectJson["path"] = projectItem->path();
        projectJson["poster_path"] = projectItem->posterPath();
        projectJson["last_edit_time"] = projectItem->lastEditTime().toString(Qt::ISODateWithMs);
        projectJson["can_ask"] = projectItem->canAskAboutSwitch();
        return projectJson;
    };

    QJsonArray projectsJson;
    for (int row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::ProjectsModelItemType::Project) {
            projectsJson.append(projectItemJson(item));
            continue;
        }

        if (item->type() != BusinessLayer::ProjectsModelItemType::Team) {
            continue;
        }

        const auto teamItem = static_cast<BusinessLayer::ProjectsModelTeamItem*>(item);
        QJsonObject teamJson;
        teamJson["is_team"] = true;
        teamJson["id"] = teamItem->id();
        teamJson["name"] = teamItem->name();
        teamJson["description"] = teamItem->description();
        teamJson["is_opened"] = teamItem->isOpened();
        QJsonArray teamProjectsJson;
        for (int childRow = 0; childRow < d->projects->rowCount(itemIndex); ++childRow) {
            const auto childItemIndex = d->projects->index(childRow, 0, itemIndex);
            const auto childItem = d->projects->itemForIndex(childItemIndex);
            Q_ASSERT(childItem->type() == BusinessLayer::ProjectsModelItemType::Project);
            teamProjectsJson.append(projectItemJson(childItem));
        }
        teamJson["projects"] = teamProjectsJson;
        projectsJson.append(teamJson);
    }
    setSettingsValue(DataStorageLayer::kApplicationProjectsKey,
                     QJsonDocument(projectsJson).toJson(QJsonDocument::Compact).toHex());
}

void ProjectsManager::saveChanges()
{
    if (d->currentProject == nullptr) {
        return;
    }

    d->currentProject->setLastEditTime(QDateTime::currentDateTime());
    d->projects->updateItem(d->currentProject);
}

void ProjectsManager::setConnected(bool _connected)
{
    d->isConnected = _connected;

    if (!d->createProjectDialog.isNull()) {
        d->createProjectDialog->configureCloudProjectCreationAbility(
            d->isConnected, d->isUserAuthorized, d->canCreateCloudProject, d->teamsForProjects);
    }
}

void ProjectsManager::setProjectsInCloudCanBeCreated(bool _authorized,
                                                     Domain::SubscriptionType _subscritionType)
{
    d->isUserAuthorized = _authorized;
    d->canCreateCloudProject = _subscritionType == Domain::SubscriptionType::CloudMonthly
        || _subscritionType == Domain::SubscriptionType::CloudLifetime
        || _subscritionType == Domain::SubscriptionType::Studio;

    if (!d->createProjectDialog.isNull()) {
        d->createProjectDialog->configureCloudProjectCreationAbility(
            d->isConnected, d->isUserAuthorized, d->canCreateCloudProject, d->teamsForProjects);
    }
}

void ProjectsManager::setTeams(const QVector<Domain::TeamInfo>& _teams)
{
    //
    // Формируем список команд, в которых пользователь может размещать проекты
    //
    d->teamsForProjects.clear();
    for (const auto& team : _teams) {
        if (team.teamRole == 0) {
            d->teamsForProjects.append(team);
        }
    }
    //
    // ... и конфигурируем диалог создания проектов
    //
    if (!d->createProjectDialog.isNull()) {
        d->createProjectDialog->configureCloudProjectCreationAbility(
            d->isConnected, d->isUserAuthorized, d->canCreateCloudProject, d->teamsForProjects);
    }

    //
    // Убираем из закешированного списка команды, которых более нет для текущего аккаунта
    //
    for (int row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() != BusinessLayer::ProjectsModelItemType::Team) {
            continue;
        }

        auto projectTeam = static_cast<BusinessLayer::ProjectsModelTeamItem*>(item);
        bool hideTeam = true;
        for (const auto& team : _teams) {
            if (projectTeam->id() == team.id) {
                hideTeam = false;
                break;
            }
        }
        if (hideTeam) {
            d->projects->removeItem(projectTeam);
            --row;
        }
    }

    //
    // Добавляем в список проектов команды, которых там ещё нет
    //
    for (const auto& team : _teams) {
        addOrUpdateTeam(team);
    }
}

void ProjectsManager::addOrUpdateTeam(const Domain::TeamInfo& _teamInfo)
{
    bool hasTeam = false;
    //
    //  Проходим все верхнеуровневые элементы
    //
    for (int row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() != BusinessLayer::ProjectsModelItemType::Team) {
            continue;
        }

        auto projectTeam = static_cast<BusinessLayer::ProjectsModelTeamItem*>(item);
        if (projectTeam->id() == _teamInfo.id) {
            projectTeam->setTeamInfo(_teamInfo);
            d->projects->updateItem(projectTeam);
            hasTeam = true;
            break;
        }
    }

    //
    // ... если команды не нашлось, то нужно создать
    //
    if (!hasTeam) {
        auto projectTeam = new BusinessLayer::ProjectsModelTeamItem;
        projectTeam->setTeamInfo(_teamInfo);
        d->projects->prependItem(projectTeam);
    }
}

void ProjectsManager::hideTeam(int _id)
{
    for (int row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() != BusinessLayer::ProjectsModelItemType::Team) {
            continue;
        }

        auto projectTeam = static_cast<BusinessLayer::ProjectsModelTeamItem*>(item);
        if (projectTeam->id() == _id) {
            d->projects->removeItem(projectTeam);
            return;
        }
    }
}

void ProjectsManager::createProject()
{
    Log::debug("Create new project");

    if (!d->createProjectDialog.isNull()) {
        Log::warning("Create new project dialog is already visible, avoid double dialog appearing");
        return;
    }

    //
    // Создаём и настраиваем диалог
    //
    d->createProjectDialog = new Ui::CreateProjectDialog(d->topLevelWidget);
    d->createProjectDialog->setProjectType(
        settingsValue(DataStorageLayer::kProjectTypeKey).toInt());
    d->createProjectDialog->setProjectFolder(
        settingsValue(DataStorageLayer::kProjectSaveFolderKey).toString());
    d->createProjectDialog->setImportFolder(
        settingsValue(DataStorageLayer::kProjectImportFolderKey).toString());
    d->createProjectDialog->configureCloudProjectCreationAbility(
        d->isConnected, d->isUserAuthorized, d->canCreateCloudProject, d->teamsForProjects);

    //
    // Настраиваем соединения диалога
    //
    connect(d->createProjectDialog, &Ui::CreateProjectDialog::loginPressed, this,
            &ProjectsManager::signInRequested);
    connect(d->createProjectDialog, &Ui::CreateProjectDialog::renewSubscriptionPressed, this,
            &ProjectsManager::renewTeamSubscriptionRequested);
    connect(d->createProjectDialog, &Ui::CreateProjectDialog::createProjectPressed, this, [this] {
        setSettingsValue(DataStorageLayer::kProjectTypeKey, d->createProjectDialog->projectType());
        setSettingsValue(DataStorageLayer::kProjectSaveFolderKey,
                         d->createProjectDialog->projectFolder());
        setSettingsValue(DataStorageLayer::kProjectImportFolderKey,
                         d->createProjectDialog->importFilePath());

        if (d->createProjectDialog->isLocal()) {
            //
            // ... проверим, можно ли создавать проекты в заданной папке
            //
            const auto projectPath = d->newProjectPath(d->createProjectDialog->projectName(),
                                                       d->createProjectDialog->projectFolder());
            QFile file(projectPath);
            const bool canWrite = file.open(QIODevice::WriteOnly);
            file.close();
            file.remove();
            //
            // ... если нет, то покажем эту проблему прямо в диалоге
            //
            if (!canWrite) {
                d->createProjectDialog->showProjectFolderError();
                return;
            }
            //
            // ... а если может, то создаём проект
            //
            emit createLocalProjectRequested(d->createProjectDialog->projectName(), projectPath,
                                             d->createProjectDialog->importFilePath());
        } else {
            emit createCloudProjectRequested(d->createProjectDialog->projectName(),
                                             d->createProjectDialog->importFilePath(),
                                             d->createProjectDialog->teamId());
        }
        d->createProjectDialog->hideDialog();
    });
    connect(d->createProjectDialog, &Ui::CreateProjectDialog::disappeared, d->createProjectDialog,
            &Ui::CreateProjectDialog::deleteLater);

    //
    // Отображаем диалог
    //
    d->createProjectDialog->showDialog();
}

void ProjectsManager::openProject()
{
    Log::debug("Open project");

    //
    // Предоставим пользователю возможность выбрать файл, который он хочет открыть
    //
    const auto projectOpenFolder
        = settingsValue(DataStorageLayer::kProjectOpenFolderKey).toString();
    const auto projectPath
        = QFileDialog::getOpenFileName(d->topLevelWidget, tr("Choose the file to open"),
                                       projectOpenFolder, DialogHelper::filtersForOpenProject());
    if (projectPath.isEmpty()) {
        return;
    }

    //
    // Если файл был выбран
    //
    // ... обновим папку, откуда в следующий раз он предположительно опять будет открывать проекты
    //
    setSettingsValue(DataStorageLayer::kProjectOpenFolderKey, projectPath);
    //
    // ... и сигнализируем о том, что файл выбран для открытия
    //
    emit openLocalProjectRequested(projectPath);
}

void ProjectsManager::setCloudProjects(const QVector<Domain::ProjectInfo>& _projects)
{
    auto removeProjectIfNeeded
        = [this, _projects](BusinessLayer::ProjectsModelProjectItem* _project) {
              if (_project->isLocal()) {
                  return false;
              }

              bool hideProject = true;
              for (const auto& project : _projects) {
                  if (_project->id() == project.id) {
                      hideProject = false;
                      break;
                  }
              }

              if (hideProject) {
                  d->projects->removeItem(_project);
                  return true;
              }

              return false;
          };

    //
    // Убираем из закешированного списка проекты, которых более нет для текущего аккаунта
    //
    for (int row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::ProjectsModelItemType::Team) {
            for (auto childRow = 0; childRow < d->projects->rowCount(itemIndex); ++childRow) {
                const auto childItemIndex = d->projects->index(childRow, 0, itemIndex);
                const auto childItem = d->projects->itemForIndex(childItemIndex);
                auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(childItem);
                const auto isProjectRemoved = removeProjectIfNeeded(project);
                if (isProjectRemoved) {
                    --childRow;
                }
            }
            continue;
        }

        auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(item);
        const auto isProjectRemoved = removeProjectIfNeeded(project);
        if (isProjectRemoved) {
            --row;
        }
    }

    //
    // Загружаем остальные
    //
    for (const auto& project : _projects) {
        addOrUpdateCloudProject(project);
    }
}

void ProjectsManager::addOrUpdateCloudProject(const Domain::ProjectInfo& _projectInfo)
{
    //
    // Определим, находится ли устанавливаемый проект уже в списке, или это новый
    //
    BusinessLayer::ProjectsModelTeamItem* projectTeam = nullptr;
    BusinessLayer::ProjectsModelProjectItem* cloudProject = nullptr;

    //
    // Если проект в команде
    //
    if (_projectInfo.teamId != Domain::kInvalidId) {
        //
        // ... ищем созданную команду
        //
        for (int row = 0; row < d->projects->rowCount(); ++row) {
            const auto itemIndex = d->projects->index(row);
            const auto item = d->projects->itemForIndex(itemIndex);
            if (item->type() != BusinessLayer::ProjectsModelItemType::Team) {
                continue;
            }

            auto team = static_cast<BusinessLayer::ProjectsModelTeamItem*>(item);
            if (team->id() == _projectInfo.teamId) {
                projectTeam = team;
                break;
            }
        }

        //
        // ... если команды не нашлось, то нужно создать
        //
        if (projectTeam == nullptr) {
            projectTeam = new BusinessLayer::ProjectsModelTeamItem;
            projectTeam->setId(_projectInfo.teamId);
            d->projects->prependItem(projectTeam);
        }
    }


    //
    // Проверяем находится ли проект в списке недавно используемых
    //
    for (int row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::ProjectsModelItemType::Team) {
            for (auto childRow = 0; childRow < d->projects->rowCount(itemIndex); ++childRow) {
                const auto childItemIndex = d->projects->index(childRow, 0, itemIndex);
                const auto childItem = d->projects->itemForIndex(childItemIndex);
                auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(childItem);
                if (project->id() == _projectInfo.id) {
                    cloudProject = project;
                    break;
                }
            }

            if (cloudProject != nullptr) {
                break;
            }

            continue;
        }

        auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(item);
        if (project->id() == _projectInfo.id) {
            cloudProject = project;
            break;
        }
    }
    //
    // ... если не нашли, то создадим новый
    //
    if (cloudProject == nullptr) {
        cloudProject = new BusinessLayer::ProjectsModelProjectItem;
    }

    //
    // Сформируем путь к файлу
    //
    const auto projectDir
        = QString("%1/projects/%2")
              .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation),
                   DataStorageLayer::StorageFacade::settingsStorage()->accountEmail());
    QDir::root().mkpath(projectDir);
    const auto projectPath = QDir::toNativeSeparators(
        cloudProject->path().isEmpty()
            ? QString("%1/%2.%3")
                  .arg(projectDir, QString::number(_projectInfo.id),
                       BusinessLayer::ProjectsModelProjectItem::extension())
            : cloudProject->path());

    //
    // Если текущий пользователь является владельцем проекта
    //
    if (_projectInfo.accountRole == 0) {
        cloudProject->setOwner(true);
        cloudProject->setEditingMode(DocumentEditingMode::Edit);
        cloudProject->clearEditingPermissions();
    }
    //
    // В противном случае
    //
    else {
        cloudProject->setOwner(false);
        cloudProject->setEditingMode(
            static_cast<DocumentEditingMode>(_projectInfo.accountRole - 1));
        QHash<QUuid, DocumentEditingMode> permissions;
        for (auto iter = _projectInfo.accountPermissions.begin();
             iter != _projectInfo.accountPermissions.end(); ++iter) {
            permissions[iter.key()] = static_cast<DocumentEditingMode>(iter.value() - 1);
        }
        cloudProject->setEditingPermissions(permissions);
    }
    //
    // Обновим параметры проекта
    //
    cloudProject->setName(_projectInfo.name);
    cloudProject->setLogline(_projectInfo.logline);
    cloudProject->setLastEditTime(_projectInfo.lastEditTime);
    if (!_projectInfo.poster.isNull()) {
        if (_projectInfo.poster.isEmpty()) {
            cloudProject->setPosterPath({});
        } else {
            const auto posterPath = d->projectPosterPath(projectPath);
            QFile posterFile(posterPath);
            if (posterFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                posterFile.write(_projectInfo.poster);
                posterFile.close();
            }
            cloudProject->setPosterPath(posterPath);
        }
    }
    cloudProject->setCollaborators(_projectInfo.collaborators);

    //
    // Если проект не нашёлся в списке недавних, добавляем в начало
    //
    if (cloudProject->projectType() == BusinessLayer::ProjectType::Invalid) {
        //
        // Определим параметры проекта
        //
        cloudProject->setId(_projectInfo.id);
        cloudProject->setTeamId(_projectInfo.teamId);
        cloudProject->setProjectType(BusinessLayer::ProjectType::Cloud);
        cloudProject->setPath(projectPath);
        cloudProject->setRealPath(projectPath);

        //
        // Добавляем проект в список
        //
        d->projects->prependItem(cloudProject, projectTeam);
    }
    //
    // А если нашёлся, то обновим его параметры
    //
    else {
        d->projects->updateItem(cloudProject);
    }

    //
    // При необходимости также обновим текущий проект
    //
    if (d->currentProject != nullptr && d->currentProject->id() == cloudProject->id()) {
        d->currentProject = cloudProject;
    }
}

void ProjectsManager::setCurrentProject(const QString& _path)
{
    setCurrentProject(_path, _path);
}

void ProjectsManager::setCurrentProject(const QString& _path, const QString& _realPath)
{
    //
    // Приведём путь к нативному виду
    //
    const QString projectPath = QDir::toNativeSeparators(_path);
    const QString projectRealPath = QDir::toNativeSeparators(_realPath);

    //
    // Делаем проект текущим и загружаем из него БД
    // или создаём, если ранее его не существовало
    //
    DatabaseLayer::Database::setCurrentFile(projectRealPath);

    //
    // Определим, находится ли устанавливаемый проект уже в списке, или это новый
    //
    BusinessLayer::ProjectsModelProjectItem* newCurrentProject = nullptr;

    //
    // Проверяем находится ли проект в списке недавно используемых
    //
    for (int row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::ProjectsModelItemType::Team) {
            for (int childRow = 0; childRow < d->projects->rowCount(itemIndex); ++childRow) {
                const auto childItemIndex = d->projects->index(childRow, 0, itemIndex);
                const auto childItem = d->projects->itemForIndex(childItemIndex);
                auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(childItem);
                if (project->path() == projectPath) {
                    newCurrentProject = project;
                    break;
                }
            }

            if (newCurrentProject != nullptr) {
                break;
            }
            continue;
        }

        auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(item);
        if (project->path() == projectPath) {
            newCurrentProject = project;
            break;
        }
    }
    if (newCurrentProject == nullptr) {
        newCurrentProject = new BusinessLayer::ProjectsModelProjectItem;
    }

    //
    // Всегда обновляем реальный путь, т.к. для теневых проектов он будет изменён при повторном
    // открытии
    //
    newCurrentProject->setRealPath(projectRealPath);

    //
    // Если проект не нашёлся в списке недавних, добавляем в начало
    //
    if (newCurrentProject->projectType() == BusinessLayer::ProjectType::Invalid) {
        QFileInfo fileInfo(projectPath);

        //
        // Определим параметры проекта
        //
        newCurrentProject->setProjectType(projectPath == projectRealPath
                                              ? BusinessLayer::ProjectType::Local
                                              : BusinessLayer::ProjectType::LocalShadow);
        newCurrentProject->setName(fileInfo.completeBaseName());
        newCurrentProject->setPath(projectPath);
        newCurrentProject->setLastEditTime(fileInfo.lastModified());

        //
        // Добавляем проект в список
        //
        d->projects->prependItem(newCurrentProject);
    }

    //
    // Запоминаем проект, как текущий
    //
    d->currentProject = newCurrentProject;
}

void ProjectsManager::setCurrentProject(int _projectId)
{
    QString newCurrentProjectPath;
    for (auto row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::ProjectsModelItemType::Team) {
            for (int childRow = 0; childRow < d->projects->rowCount(itemIndex); ++childRow) {
                const auto childItemIndex = d->projects->index(childRow, 0, itemIndex);
                const auto childItem = d->projects->itemForIndex(childItemIndex);
                auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(childItem);
                if (project->isRemote() && project->id() == _projectId) {
                    newCurrentProjectPath = project->path();
                    break;
                }
            }

            if (!newCurrentProjectPath.isEmpty()) {
                break;
            }

            continue;
        }

        auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(item);
        if (project->isRemote() && project->id() == _projectId) {
            newCurrentProjectPath = project->path();
            break;
        }
    }
    Q_ASSERT(!newCurrentProjectPath.isEmpty());
    if (newCurrentProjectPath.isEmpty()) {
        Log::critical("Can't find cloud project with identifier %1", QString::number(_projectId));
        return;
    }

    //
    // ... и установим его текущим
    //
    setCurrentProject(newCurrentProjectPath);
}

void ProjectsManager::setCurrentProjectUuid(const QUuid& _uuid)
{
    d->currentProject->setUuid(_uuid);
    d->projects->updateItem(d->currentProject);
}

void ProjectsManager::setCurrentProjectName(const QString& _name)
{
    d->currentProject->setName(_name);
    d->projects->updateItem(d->currentProject);

    if (d->currentProject->isRemote()) {
        d->isCloudProjectNameChanged = true;
        d->cloudProjectChangeDebouncer.orderWork();
    }
}

void ProjectsManager::setCurrentProjectLogline(const QString& _logline)
{
    d->currentProject->setLogline(_logline);
    d->projects->updateItem(d->currentProject);

    if (d->currentProject->isRemote()) {
        d->isCloudProjectLoglineChnaged = true;
        d->cloudProjectChangeDebouncer.orderWork();
    }
}

void ProjectsManager::setCurrentProjectCover(const QPixmap& _cover)
{
    if (_cover.isNull()) {
        d->currentProject->setPosterPath({});
        d->projects->updateItem(d->currentProject);
    } else {
        const auto posterPath = d->projectPosterPath(d->currentProject->path());
        _cover.save(posterPath, "PNG");

        d->currentProject->setPosterPath(posterPath);
        d->projects->updateItem(d->currentProject);
    }

    if (d->currentProject->isRemote()) {
        d->isCloudProjectCoverChnaged = true;
        d->cloudProjectChangeDebouncer.orderWork();
    }
}

void ProjectsManager::setCurrentProjectNeverAskAboutSwitch()
{
    d->currentProject->setCanAskAboutSwitch(false);
    d->projects->updateItem(d->currentProject);
}

void ProjectsManager::setCurrentProjectCanBeSynced(bool _can)
{
    d->currentProject->setCanBeSynced(_can);
    d->projects->updateItem(d->currentProject);
}

void ProjectsManager::closeCurrentProject()
{
    //
    // Закрываем сам файл с базой данных
    //
    DatabaseLayer::Database::closeCurrentFile();

    //
    // Очищаем хранилища
    //
    DataStorageLayer::StorageFacade::clearStorages();

    //
    // Для теневого проекта, удаляем временный файл
    //
    if (d->currentProject->projectType() == BusinessLayer::ProjectType::LocalShadow) {
        QFile::remove(d->currentProject->realPath());
    }

    //
    // Очищаем текущий проект
    //
    d->currentProject = {};
}

BusinessLayer::ProjectsModelProjectItem* ProjectsManager::project(const QString& _path) const
{
    for (auto row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::ProjectsModelItemType::Team) {
            for (int childRow = 0; childRow < d->projects->rowCount(itemIndex); ++childRow) {
                const auto childItemIndex = d->projects->index(childRow, 0, itemIndex);
                const auto childItem = d->projects->itemForIndex(childItemIndex);
                auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(childItem);
                if (project->path() == _path) {
                    return project;
                }
            }

            continue;
        }

        auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(item);
        if (project->path() == _path) {
            return project;
            break;
        }
    }

    return {};
}

BusinessLayer::ProjectsModelProjectItem* ProjectsManager::project(int _id) const
{
    if (_id == Domain::kInvalidId) {
        return {};
    }

    for (auto row = 0; row < d->projects->rowCount(); ++row) {
        const auto itemIndex = d->projects->index(row);
        const auto item = d->projects->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::ProjectsModelItemType::Team) {
            for (int childRow = 0; childRow < d->projects->rowCount(itemIndex); ++childRow) {
                const auto childItemIndex = d->projects->index(childRow, 0, itemIndex);
                const auto childItem = d->projects->itemForIndex(childItemIndex);
                auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(childItem);
                if (project->id() == _id) {
                    return project;
                }
            }

            continue;
        }

        auto project = static_cast<BusinessLayer::ProjectsModelProjectItem*>(item);
        if (project->id() == _id) {
            return project;
            break;
        }
    }

    return {};
}

void ProjectsManager::hideProject(const QString& _path)
{
    auto project = this->project(_path);
    if (project == nullptr) {
        return;
    }

    d->projects->removeItem(project);
}

void ProjectsManager::hideProject(int _id)
{
    auto project = this->project(_id);
    if (project == nullptr) {
        return;
    }

    d->projects->removeItem(project);
}

void ProjectsManager::removeProject(int _id)
{
    auto project = this->project(_id);
    if (project == nullptr) {
        return;
    }

    QFile::remove(project->path());
    d->projects->removeItem(project);
}

BusinessLayer::ProjectsModelProjectItem* ProjectsManager::currentProject() const
{
    return d->currentProject;
}

} // namespace ManagementLayer

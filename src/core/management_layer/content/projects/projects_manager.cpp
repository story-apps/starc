#include "projects_manager.h"

#include "project.h"

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
#include <utils/helpers/file_helper.h>
#include <utils/helpers/image_helper.h>
#include <utils/logging.h>
#include <utils/tools/debouncer.h>

#include <QAction>
#include <QCryptographicHash>
#include <QDateTime>
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


    bool isConnected = false;
    bool isUserAuthorized = false;
    bool canCreateCloudProject = false;

    ProjectsModel* projects = nullptr;
    Project currentProject;

    QWidget* topLevelWidget = nullptr;

    Ui::ProjectsToolBar* toolBar = nullptr;
    Ui::ProjectsNavigator* navigator = nullptr;
    Ui::ProjectsView* view = nullptr;

    QPointer<Ui::CreateProjectDialog> createProjectDialog;

    bool isCloudProjectNameChnaged = false;
    bool isCloudProjectLoglineChnaged = false;
    bool isCloudProjectCoverChnaged = false;
    Debouncer cloudProjectChangeDebouncer;
};

ProjectsManager::Implementation::Implementation(QWidget* _parent)
    : projects(new ProjectsModel(_parent))
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
            [this](const Project& _project) {
                if (_project.isLocal()) {
                    emit openLocalProjectRequested(_project.path());
                } else {
                    emit openCloudProjectRequested(_project.id(), _project.path());
                }
            });
    connect(d->view, &Ui::ProjectsView::projectContextMenuRequested, this,
            [this](const Project& _project) {
                QVector<QAction*> actions;
                //
                // Действия над локальным проектом
                //
                if (_project.isLocal()) {
                    auto showInFolderAction = new QAction;
                    showInFolderAction->setIconText(u8"\U000F178B");
                    showInFolderAction->setText(tr("Show in folder"));
                    connect(showInFolderAction, &QAction::triggered, this,
                            [_project] { FileHelper::showInGraphicalShell(_project.path()); });
                    actions.append(showInFolderAction);
                    //
                    auto hideFromRecentAction = new QAction;
                    hideFromRecentAction->setIconText(u8"\U000F06D1");
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
                                    d->projects->remove(_project);
                                });
                        connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
                    });
                    actions.append(hideFromRecentAction);
                }
                //
                // Действия над облачным проектом
                //
                else {
                    //
                    // TODO: Контекстное меню в зависимости от того, владеет ли пользоваетль файлом
                    //
                    auto removeProjectAction = new QAction;
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
                                    // Если таки хочет, то скрываем уведомляем, чтобы удалить проект
                                    // на сервере
                                    //
                                    emit removeCloudProjectRequested(_project.id());
                                });
                        connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
                    });
                    actions.append(removeProjectAction);
                }

                auto menu = new ContextMenu(d->view);
                menu->setActions(actions);
                menu->setBackgroundColor(Ui::DesignSystem::color().background());
                menu->setTextColor(Ui::DesignSystem::color().onBackground());
                connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

                menu->showContextMenu(QCursor::pos());
            });
    connect(&d->cloudProjectChangeDebouncer, &Debouncer::gotWork, this, [this] {
        const auto name = d->isCloudProjectNameChnaged ? d->currentProject.name() : QString();
        const auto logline
            = d->isCloudProjectLoglineChnaged ? d->currentProject.logline() : QString();
        const auto cover = d->isCloudProjectCoverChnaged
            ? ImageHelper::bytesFromImage(d->currentProject.poster())
            : QByteArray();

        d->isCloudProjectCoverChnaged = false;
        d->isCloudProjectLoglineChnaged = false;
        d->isCloudProjectCoverChnaged = false;

        emit updateCloudProjectRequested(d->currentProject.id(), name, logline, cover);
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

    QVector<Project> projects;
    for (const auto projectJsonValue : projectsJson.array()) {
        const auto projectJson = projectJsonValue.toObject();

        //
        // Если локальный файл проекта удалили, то не добавляем его в список недавних проектов
        //
        const auto isRemote = projectJson.contains("id");
        const auto projectPath = projectJson["path"].toString();
        if (!isRemote && !QFileInfo::exists(projectPath)) {
            continue;
        }

        Project project;
        if (isRemote) {
            project.setId(projectJson["id"].toInt());
        }
        project.setType(static_cast<ProjectType>(projectJson["type"].toInt()));
        project.setName(projectJson["name"].toString());
        project.setLogline(projectJson["logline"].toString());
        project.setPath(projectJson["path"].toString());
        project.setPosterPath(projectJson["poster_path"].toString());
        project.setLastEditTime(
            QDateTime::fromString(projectJson["last_edit_time"].toString(), Qt::ISODateWithMs));
        project.setCanAskAboutSwitch(projectJson["can_ask"].toBool());
        projects.append(project);
    }
    d->projects->append(projects);
}

void ProjectsManager::saveProjects()
{
    QJsonArray projectsJson;
    for (int projectIndex = 0; projectIndex < d->projects->rowCount(); ++projectIndex) {
        const auto& project = d->projects->projectAt(projectIndex);
        QJsonObject projectJson;
        if (project.isRemote()) {
            projectJson["id"] = project.id();
        }
        projectJson["type"] = static_cast<int>(project.type());
        projectJson["name"] = project.name();
        projectJson["logline"] = project.logline();
        projectJson["path"] = project.path();
        projectJson["poster_path"] = project.posterPath();
        projectJson["last_edit_time"] = project.lastEditTime().toString(Qt::ISODateWithMs);
        projectJson["can_ask"] = project.canAskAboutSwitch();
        projectsJson.append(projectJson);
    }
    setSettingsValue(DataStorageLayer::kApplicationProjectsKey,
                     QJsonDocument(projectsJson).toJson(QJsonDocument::Compact).toHex());
}

void ProjectsManager::saveChanges()
{
    if (!d->currentProject.isValid()) {
        return;
    }

    d->currentProject.setLastEditTime(QDateTime::currentDateTime());
    d->projects->updateProject(d->currentProject);
}

void ProjectsManager::setConnected(bool _connected)
{
    d->isConnected = _connected;

    if (!d->createProjectDialog.isNull()) {
        d->createProjectDialog->configureCloudProjectCreationAbility(
            d->isConnected, d->isUserAuthorized, d->canCreateCloudProject);
    }
}

void ProjectsManager::setProjectsInCloudCanBeCreated(bool _authorized,
                                                     Domain::SubscriptionType _subscritionType)
{
    d->isUserAuthorized = _authorized;
    d->canCreateCloudProject = _subscritionType == Domain::SubscriptionType::TeamMonthly
        || _subscritionType == Domain::SubscriptionType::TeamLifetime;

    if (!d->createProjectDialog.isNull()) {
        d->createProjectDialog->configureCloudProjectCreationAbility(
            d->isConnected, d->isUserAuthorized, d->canCreateCloudProject);
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
        d->isConnected, d->isUserAuthorized, d->canCreateCloudProject);

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
            const auto projectPathPrefix = d->createProjectDialog->projectFolder() + "/"
                + FileHelper::systemSavebleFileName(d->createProjectDialog->projectName());
            auto projectPath = projectPathPrefix + Project::extension();
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
                    + Project::extension();
            }
            emit createLocalProjectRequested(d->createProjectDialog->projectName(), projectPath,
                                             d->createProjectDialog->importFilePath());
        } else {
            emit createCloudProjectRequested(d->createProjectDialog->projectName(),
                                             d->createProjectDialog->importFilePath());
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
    //
    // Убираем из закешированного списка проекты, которых более нет для текущего аккаунта
    //
    for (int projectRow = 0; projectRow < d->projects->rowCount(); ++projectRow) {
        const auto& project = d->projects->projectAt(projectRow);
        if (project.isLocal()) {
            continue;
        }

        bool hideProject = true;
        for (int index = 0; index < _projects.size(); ++index) {
            if (project.id() == _projects.at(index).id) {
                hideProject = false;
                break;
            }
        }

        if (hideProject) {
            d->projects->remove(project);
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
    Project cloudProject;

    //
    // Проверяем находится ли проект в списке недавно используемых
    //
    for (int projectRow = 0; projectRow < d->projects->rowCount(); ++projectRow) {
        const auto& project = d->projects->projectAt(projectRow);
        if (project.id() == _projectInfo.id) {
            cloudProject = project;
            break;
        }
    }

    //
    // Сформируем путь к файлу
    //
    const auto projectDir
        = QString("%1/projects/%2")
              .arg(QStandardPaths::writableLocation(QStandardPaths::DataLocation),
                   DataStorageLayer::StorageFacade::settingsStorage()->accountEmail());
    QDir::root().mkpath(projectDir);
    const auto projectPath = cloudProject.path().isEmpty()
        ? QString("%1/%2.%3")
              .arg(projectDir, QString::number(_projectInfo.id), Project::extension())
        : cloudProject.path();

    //
    // Обновим параметры проекта
    //
    cloudProject.setName(_projectInfo.name);
    cloudProject.setLogline(_projectInfo.logline);
    cloudProject.setLastEditTime(_projectInfo.lastEditTime);
    if (!_projectInfo.poster.isNull()) {
        if (_projectInfo.poster.isEmpty()) {
            cloudProject.setPosterPath({});
        } else {
            const auto posterPath = d->projectPosterPath(projectPath);
            const auto poster = ImageHelper::imageFromBytes(_projectInfo.poster);
            poster.save(posterPath, "PNG");
            cloudProject.setPosterPath(posterPath);
        }
    }
    cloudProject.setCollaborators(_projectInfo.collaborators);

    //
    // Если проект не нашёлся в списке недавних, добавляем в начало
    //
    if (cloudProject.type() == ProjectType::Invalid) {
        //
        // Определим параметры проекта
        //
        cloudProject.setId(_projectInfo.id);
        cloudProject.setType(ProjectType::Cloud);
        //
        // Если текущий пользователь является владельцем проекта
        //
        if (_projectInfo.accountRole == 0) {
            cloudProject.setOwner(true);
            cloudProject.setEditingMode(DocumentEditingMode::Edit);
        }
        //
        // В противном случае
        //
        else {
            cloudProject.setOwner(false);
            cloudProject.setEditingMode(
                static_cast<DocumentEditingMode>(_projectInfo.accountRole - 1));
        }
        cloudProject.setPath(projectPath);
        cloudProject.setRealPath(projectPath);

        //
        // Добавляем проект в список
        //
        d->projects->prepend(cloudProject);
    }
    //
    // А если нашёлся, то обновим его параметры
    //
    else {
        d->projects->updateProject(cloudProject);
    }

    //
    // При необходимости также обновим текущий проект
    //
    if (d->currentProject.id() == cloudProject.id()) {
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
    Project newCurrentProject;

    //
    // Проверяем находится ли проект в списке недавно используемых
    //
    for (int projectRow = 0; projectRow < d->projects->rowCount(); ++projectRow) {
        const auto& project = d->projects->projectAt(projectRow);
        if (project.path() == projectPath) {
            newCurrentProject = project;
            break;
        }
    }

    //
    // Всегда обновляем реальный путь, т.к. для теневых проектов он будет изменён при повторном
    // открытии
    //
    newCurrentProject.setRealPath(projectRealPath);

    //
    // Если проект не нашёлся в списке недавних, добавляем в начало
    //
    if (newCurrentProject.type() == ProjectType::Invalid) {
        QFileInfo fileInfo(projectPath);

        //
        // Определим параметры проекта
        //
        newCurrentProject.setType(projectPath == projectRealPath ? ProjectType::Local
                                                                 : ProjectType::LocalShadow);
        newCurrentProject.setName(fileInfo.completeBaseName());
        newCurrentProject.setPath(projectPath);
        newCurrentProject.setLastEditTime(fileInfo.lastModified());

        //
        // Добавляем проект в список
        //
        d->projects->prepend(newCurrentProject);
    }

    //
    // Запоминаем проект, как текущий
    //
    d->currentProject = newCurrentProject;
}

void ProjectsManager::setCurrentProject(int _projectId)
{
    QString newCurrentProjectPath;
    for (auto index = 0; index < d->projects->rowCount(); ++index) {
        const auto& project = d->projects->projectAt(index);
        if (project.isRemote() && project.id() == _projectId) {
            newCurrentProjectPath = project.path();
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

void ProjectsManager::setCurrentProjectName(const QString& _name)
{
    d->currentProject.setName(_name);
    d->projects->updateProject(d->currentProject);

    if (d->currentProject.isRemote()) {
        d->isCloudProjectNameChnaged = true;
        d->cloudProjectChangeDebouncer.orderWork();
    }
}

void ProjectsManager::setCurrentProjectLogline(const QString& _logline)
{
    d->currentProject.setLogline(_logline);
    d->projects->updateProject(d->currentProject);

    if (d->currentProject.isRemote()) {
        d->isCloudProjectLoglineChnaged = true;
        d->cloudProjectChangeDebouncer.orderWork();
    }
}

void ProjectsManager::setCurrentProjectCover(const QPixmap& _cover)
{
    if (_cover.isNull()) {
        d->currentProject.setPosterPath({});
        d->projects->updateProject(d->currentProject);
    } else {
        const auto posterPath = d->projectPosterPath(d->currentProject.path());
        _cover.save(posterPath, "PNG");

        d->currentProject.setPosterPath(posterPath);
        d->projects->updateProject(d->currentProject);
    }

    if (d->currentProject.isRemote()) {
        d->isCloudProjectCoverChnaged = true;
        d->cloudProjectChangeDebouncer.orderWork();
    }
}

void ProjectsManager::setCurrentProjectNeverAskAboutSwitch()
{
    d->currentProject.setCanAskAboutSwitch(false);
    d->projects->updateProject(d->currentProject);
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
    if (d->currentProject.type() == ProjectType::LocalShadow) {
        QFile::remove(d->currentProject.realPath());
    }

    //
    // Очищаем текущий проект
    //
    d->currentProject = {};
}

void ProjectsManager::hideProject(const QString& _path)
{
    for (int projectRow = 0; projectRow < d->projects->rowCount(); ++projectRow) {
        const auto& project = d->projects->projectAt(projectRow);
        if (project.path() == _path) {
            d->projects->remove(project);
            break;
        }
    }
}

void ProjectsManager::hideProject(int _id)
{
    if (_id == -1) {
        return;
    }

    for (int projectRow = 0; projectRow < d->projects->rowCount(); ++projectRow) {
        const auto& project = d->projects->projectAt(projectRow);
        if (project.id() == _id) {
            d->projects->remove(project);
            break;
        }
    }
}

void ProjectsManager::removeProject(int _id)
{
    if (_id == -1) {
        return;
    }

    for (int projectRow = 0; projectRow < d->projects->rowCount(); ++projectRow) {
        const auto& project = d->projects->projectAt(projectRow);
        if (project.id() == _id) {
            QFile::remove(project.path());
            d->projects->remove(project);
            break;
        }
    }
}

const Project& ProjectsManager::currentProject() const
{
    return d->currentProject;
}

} // namespace ManagementLayer

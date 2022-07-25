#include "projects_manager.h"

#include "project.h"

#include <data_layer/database.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>
#include <ui/design_system/design_system.h>
#include <ui/projects/create_project_dialog.h>
#include <ui/projects/projects_navigator.h>
#include <ui/projects/projects_tool_bar.h>
#include <ui/projects/projects_view.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/dialog/dialog.h>
#include <utils/helpers/dialog_helper.h>
#include <utils/helpers/file_helper.h>
#include <utils/logging.h>

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
#include <QStandardPaths>
#include <QTimer>
#include <QWidget>


namespace ManagementLayer {

class ProjectsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    bool isUserAuthorized = false;
    bool canCreateCloudProject = false;

    ProjectsModel* projects = nullptr;
    Project currentProject;

    QWidget* topLevelWidget = nullptr;

    Ui::ProjectsToolBar* toolBar = nullptr;
    Ui::ProjectsNavigator* navigator = nullptr;
    Ui::ProjectsView* view = nullptr;
};

ProjectsManager::Implementation::Implementation(QWidget* _parent)
    : projects(new ProjectsModel(_parent))
    , topLevelWidget(_parent)
    , toolBar(new Ui::ProjectsToolBar(_parent))
    , navigator(new Ui::ProjectsNavigator(_parent))
    , view(new Ui::ProjectsView(_parent))
{
    toolBar->hide();

    navigator->hide();

    view->setProjects(projects);
    view->hide();
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
            [this](const Project& _project) { emit openChoosedProjectRequested(_project.path()); });
    connect(d->view, &Ui::ProjectsView::projectContextMenuRequested, this,
            [this](const Project& _project) {
                auto showInFolderAction = new QAction;
                showInFolderAction->setIconText(u8"\U000F178B");
                showInFolderAction->setText(tr("Show in folder"));
                connect(showInFolderAction, &QAction::triggered, this,
                        [_project] { FileHelper::showInGraphicalShell(_project.path()); });
                //
                auto hideFromRecentAction = new QAction;
                hideFromRecentAction->setIconText(u8"\U000F06D1");
                hideFromRecentAction->setText(tr("Hide from recent list"));
                connect(hideFromRecentAction, &QAction::triggered, this, [this, _project] {
                    auto dialog = new Dialog(d->view->topLevelWidget());
                    constexpr int cancelButtonId = 0;
                    constexpr int hideButtonId = 1;
                    dialog->showDialog(
                        {}, tr("Do you really want to hide a project from the recent list?"),
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

                auto menu = new ContextMenu(d->view);
                menu->setActions({ showInFolderAction, hideFromRecentAction });
                menu->setBackgroundColor(Ui::DesignSystem::color().background());
                menu->setTextColor(Ui::DesignSystem::color().onBackground());
                connect(menu, &ContextMenu::disappeared, menu, &ContextMenu::deleteLater);

                menu->showContextMenu(QCursor::pos());
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
    //
    // TODO: Оставить только голый json
    //
    auto projectsJson
#if (QT_VERSION < QT_VERSION_CHECK(6, 0, 0))
        = QJsonDocument::fromBinaryData(QByteArray::fromHex(projectsData.toByteArray()));
    if (projectsJson.isEmpty())
        projectsJson
#endif
            = QJsonDocument::fromJson(QByteArray::fromHex(projectsData.toByteArray()));

    QVector<Project> projects;
    for (const auto projectJsonValue : projectsJson.array()) {
        const auto projectJson = projectJsonValue.toObject();

        //
        // Если файл проекта удалили, то не пропускаем его
        //
        const auto projectPath = projectJson["path"].toString();
        if (!QFileInfo::exists(projectPath)) {
            continue;
        }

        Project project;
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

void ProjectsManager::setProjectsInCloudCanBeCreated(bool _authorized, bool _ableToCreate)
{
    d->isUserAuthorized = _authorized;
    d->canCreateCloudProject = _ableToCreate;
}

void ProjectsManager::createProject()
{
    Log::debug("Create new project");

    //
    // Создаём и настраиваем диалог
    //
    auto dialog = new Ui::CreateProjectDialog(d->topLevelWidget);
    dialog->setProjectType(settingsValue(DataStorageLayer::kProjectTypeKey).toInt());
    dialog->setProjectFolder(settingsValue(DataStorageLayer::kProjectSaveFolderKey).toString());
    dialog->setImportFolder(settingsValue(DataStorageLayer::kProjectImportFolderKey).toString());
    dialog->configureCloudProjectCreationAbility(d->isUserAuthorized, d->canCreateCloudProject);

    //
    // Настраиваем соединения диалога
    //
    connect(dialog, &Ui::CreateProjectDialog::createProjectPressed, this, [this, dialog] {
        setSettingsValue(DataStorageLayer::kProjectTypeKey, dialog->projectType());
        setSettingsValue(DataStorageLayer::kProjectSaveFolderKey, dialog->projectFolder());
        setSettingsValue(DataStorageLayer::kProjectImportFolderKey, dialog->importFilePath());

        if (dialog->isLocal()) {
            const auto projectPathPrefix = dialog->projectFolder() + "/"
                + FileHelper::systemSavebleFileName(dialog->projectName());
            auto projectPath = projectPathPrefix + Project::extension();
            //
            // Ситуация, что файл с таким названием уже существует крайне редка, хотя и
            // гипотетически возможна
            //
            if (QFileInfo::exists(projectPath)) {
                //
                // ... в таком случае добавляем метку с датой и временем создания файла, чтобы имена
                // не пересекались
                //
                projectPath = projectPathPrefix + "_"
                    + QDateTime::currentDateTime().toString("yyyy_MM_dd_hh_mm_ss")
                    + Project::extension();
            }
            emit createLocalProjectRequested(dialog->projectName(), projectPath,
                                             dialog->importFilePath());
        } else {
            emit createCloudProjectRequested(dialog->projectName(), dialog->importFilePath());
        }
        dialog->hideDialog();
    });
    connect(dialog, &Ui::CreateProjectDialog::disappeared, dialog,
            &Ui::CreateProjectDialog::deleteLater);

    //
    // Отображаем диалог
    //
    dialog->showDialog();
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
    emit openChoosedProjectRequested(projectPath);
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
        newCurrentProject.setName(fileInfo.completeBaseName());
        newCurrentProject.setType(projectPath == projectRealPath ? ProjectType::Local
                                                                 : ProjectType::LocalShadow);
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

void ProjectsManager::setCurrentProjectName(const QString& _name)
{
    d->currentProject.setName(_name);
    d->projects->updateProject(d->currentProject);
}

void ProjectsManager::setCurrentProjectLogline(const QString& _logline)
{
    d->currentProject.setLogline(_logline);
    d->projects->updateProject(d->currentProject);
}

void ProjectsManager::setCurrentProjectCover(const QPixmap& _cover)
{
    if (_cover.isNull()) {
        d->currentProject.setPosterPath({});
        d->projects->updateProject(d->currentProject);
        return;
    }

    const QString posterDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
        + "/thumbnails/projects/";
    QDir::root().mkpath(posterDir);

    const QString posterName
        = QCryptographicHash::hash(d->currentProject.path().toUtf8(), QCryptographicHash::Md5)
              .toHex();
    const QString posterPath = posterDir + posterName;
    _cover.save(posterPath, "PNG");

    d->currentProject.setPosterPath(posterPath);
    d->projects->updateProject(d->currentProject);
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

const Project& ProjectsManager::currentProject() const
{
    return d->currentProject;
}

} // namespace ManagementLayer

#include "projects_manager.h"

#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/project.h>

#include <ui/projects/create_project_dialog.h>
#include <ui/projects/projects_navigator.h>
#include <ui/projects/projects_tool_bar.h>
#include <ui/projects/projects_view.h>

#include <QDateTime>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <QWidget>


namespace ManagementLayer
{

QString Project::extension()
{
    return ".starc";
}

Project::Project(const QString& _name, const QString& _path, const QString& _lastEditDatetime) :
    m_name(_name),
    m_path(_path),
    m_lastEditDatetime(_lastEditDatetime)
{
}

bool Project::isValid() const
{
    return !m_name.isEmpty() && !m_path.isEmpty();
}

QString Project::name() const
{
    return m_name;
}

void Project::setName(const QString& _name)
{
    if (m_name != _name) {
        m_name = _name;
    }
}

QString Project::path() const
{
    return m_path;
}

void Project::setPath(const QString& _path)
{
    if (m_path != _path) {
        m_path = _path;
    }
}

QString Project::lastEditDatetime() const
{
    return m_lastEditDatetime;
}

void Project::setLastEditDatetime(const QString& _datetime)
{
    if (m_lastEditDatetime != _datetime) {
        m_lastEditDatetime = _datetime;
    }
}


bool operator==(const Project& _lhs, const Project& _rhs)
{
    return _lhs.path() == _rhs.path();
}


// ****


class ProjectsManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    Domain::ProjectsModel* projects = nullptr;

    QWidget* topLevelWidget = nullptr;

    Ui::ProjectsToolBar* toolBar = nullptr;
    Ui::ProjectsNavigator* navigator = nullptr;
    Ui::ProjectsView* view = nullptr;

    bool isUserAuthorized = false;
    bool canCreateCloudProject = false;
};

ProjectsManager::Implementation::Implementation(QWidget* _parent)
    : projects(new Domain::ProjectsModel(_parent)),
      topLevelWidget(_parent),
      toolBar(new Ui::ProjectsToolBar(_parent)),
      navigator(new Ui::ProjectsNavigator(_parent)),
      view(new Ui::ProjectsView(_parent))
{
    toolBar->hide();

    navigator->hide();

    view->setProjects(projects);
    view->hide();
}


// **


ProjectsManager::ProjectsManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->toolBar, &Ui::ProjectsToolBar::menuPressed, this, &ProjectsManager::menuRequested);
    connect(d->navigator, &Ui::ProjectsNavigator::createProjectPressed, this, &ProjectsManager::createProjectRequested);

    connect(d->view, &Ui::ProjectsView::createProjectPressed, this, &ProjectsManager::createProjectRequested);
    connect(d->view, &Ui::ProjectsView::hideProjectRequested, this, [this] (const Domain::Project& _project) {
        d->projects->remove(_project);
    });
    connect(d->view, &Ui::ProjectsView::removeProjectRequested, this, [this] (const Domain::Project& _project) {
        d->projects->remove(_project);
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

void ProjectsManager::loadProjects()
{
    const auto projectsData
        = DataStorageLayer::StorageFacade::settingsStorage()->value(
              DataStorageLayer::kApplicationProjectsKey,
              DataStorageLayer::SettingsStorage::SettingsPlace::Application);
    const auto projectsJson = QJsonDocument::fromBinaryData(QByteArray::fromHex(projectsData.toByteArray()));
    QVector<Domain::Project> projects;
    for (const auto projectJsonValue : projectsJson.array()) {
        const auto projectJson = projectJsonValue.toObject();
        Domain::Project project;
        project.setType(static_cast<Domain::ProjectType>(projectJson["type"].toInt()));
        project.setName(projectJson["name"].toString());
        project.setLogline(projectJson["logline"].toString());
        project.setPath(projectJson["path"].toString());
        project.setPosterPath(projectJson["poster_path"].toString());
        project.setLastEditTime(QDateTime::fromString(projectJson["last_edit_time"].toString(), Qt::ISODateWithMs));
        projects.append(project);
    }
    d->projects->append(projects);

//    for (int i=0;i<10;++i) {
//        Domain::Project p;
//        p.setName("test" + QString::number(i));
//        p.setType(i%2 ? Domain::ProjectType::Remote : Domain::ProjectType::Local);
//        d->projects->prepend(p);
//    }
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
        projectsJson.append(projectJson);
    }
    DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                DataStorageLayer::kApplicationProjectsKey,
                QJsonDocument(projectsJson).toBinaryData().toHex(),
                DataStorageLayer::SettingsStorage::SettingsPlace::Application);
}

void ProjectsManager::setProjectsInCloudCanBeCreated(bool _authorized, bool _ableToCreate)
{
    d->isUserAuthorized = _authorized;
    d->canCreateCloudProject = _ableToCreate;
}

void ProjectsManager::createProject()
{
    Ui::CreateProjectDialog* dialog = new Ui::CreateProjectDialog(d->topLevelWidget);
    dialog->configureCloudProjectCreationAbility(d->isUserAuthorized, d->canCreateCloudProject);
    dialog->setProjectFolder(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    DataStorageLayer::kProjectSaveFolderKey,
                    DataStorageLayer::SettingsStorage::SettingsPlace::Application)
                .toString());
    dialog->setImportFolder(
                DataStorageLayer::StorageFacade::settingsStorage()->value(
                    DataStorageLayer::kProjectImportFolderKey,
                    DataStorageLayer::SettingsStorage::SettingsPlace::Application)
                .toString());
    dialog->showDialog();
    connect(dialog, &Ui::CreateProjectDialog::createProjectPressed, this, [this, dialog] {
        DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                    DataStorageLayer::kProjectSaveFolderKey,
                    dialog->projectFolder(),
                    DataStorageLayer::SettingsStorage::SettingsPlace::Application);
        DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                    DataStorageLayer::kProjectImportFolderKey,
                    dialog->importFilePath(),
                    DataStorageLayer::SettingsStorage::SettingsPlace::Application);

        if (dialog->isLocal()) {
            const QString projectPath = dialog->projectFolder() + "/" + dialog->projectName() + Project::extension();
            emit createLocalProjectRequested(projectPath, dialog->importFilePath());
        } else {
            emit createCloudProjectRequested(dialog->projectName(), dialog->importFilePath());
        }
    });
    connect(dialog, &Ui::CreateProjectDialog::disappeared, dialog, &Ui::CreateProjectDialog::deleteLater);
}

} // namespace ManagementLayer

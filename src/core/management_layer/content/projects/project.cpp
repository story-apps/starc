#include "project.h"

#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>

#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>


namespace ManagementLayer {

class Project::Implementation
{
public:
    ProjectType type = ProjectType::Invalid;
    QString path;
    QString realPath;

    mutable QPixmap poster;
    QString posterPath;
    QUuid uuid;
    QString name;
    QString logline;
    QDateTime lastEditTime;
    bool canAskAboutSwitch = true;
    bool canBeSynced = true;
    int id = -1;
    bool isOwner = true;
    DocumentEditingMode editingMode = DocumentEditingMode::Edit;
    QHash<QUuid, DocumentEditingMode> editingPermissions;
    QVector<Domain::ProjectCollaboratorInfo> collaborators;
};


// **

QString Project::extension()
{
    return ".starc";
}


Project::Project()
    : d(new Implementation)
{
}

Project::Project(const Project& _other)
    : d(new Implementation(*_other.d))
{
}

Project::~Project() = default;

const Project& Project::operator=(const Project& _other)
{
    d.reset(new Implementation(*_other.d));
    return *this;
}

bool Project::isValid() const
{
    return d->type != ProjectType::Invalid;
}

bool Project::isLocal() const
{
    return d->type == ProjectType::Local || d->type == ProjectType::LocalShadow;
}

bool Project::isRemote() const
{
    return d->type == ProjectType::Cloud;
}

ProjectType Project::type() const
{
    return d->type;
}

void Project::setType(ProjectType _type)
{
    d->type = _type;
}

QString Project::path() const
{
    return d->path;
}

void Project::setPath(const QString& _path)
{
    d->path = _path;

    if (d->type == ProjectType::Local || d->type == ProjectType::LocalShadow) {
        d->editingMode = QFileInfo(d->path).isWritable() ? DocumentEditingMode::Edit
                                                         : DocumentEditingMode::Read;
    }
}

QString Project::realPath() const
{
    return d->realPath;
}

void Project::setRealPath(const QString& _path)
{
    d->realPath = _path;
}

const QPixmap& Project::poster() const
{
    if (d->poster.isNull()) {
        d->poster.load(d->posterPath);
        if (d->poster.isNull()) {
            static const QPixmap kDefaultPoster(":/images/movie-poster");
            d->poster = kDefaultPoster;
        }
    }

    return d->poster;
}

QString Project::posterPath() const
{
    return d->posterPath;
}

void Project::setPosterPath(const QString& _path)
{
    if (d->posterPath == _path) {
        return;
    }

    d->posterPath = _path;

    //
    // Обнуляем постер, чтобы он потом извлёкся по заданному пути
    //
    d->poster = {};
}

QUuid Project::uuid() const
{
    return d->uuid;
}

void Project::setUuid(const QUuid& _uuid)
{
    d->uuid = _uuid;
}

QString Project::name() const
{
    return d->name;
}

void Project::setName(const QString& _name)
{
    d->name = _name;
}

QString Project::logline() const
{
    return d->logline;
}

void Project::setLogline(const QString& _logline)
{
    d->logline = _logline;
}

QString Project::displayLastEditTime() const
{
    switch (d->lastEditTime.daysTo(QDateTime::currentDateTime())) {
    case 0: {
        return QApplication::translate("Domain::Project", "today at")
            + d->lastEditTime.toString(" hh:mm");
    }

    case 1: {
        return QApplication::translate("Domain::Project", "yesterday at")
            + d->lastEditTime.toString(" hh:mm");
    }

    default: {
        return d->lastEditTime.toString("dd.MM.yyyy hh:mm");
    }
    }
}

QDateTime Project::lastEditTime() const
{
    return d->lastEditTime;
}

void Project::setLastEditTime(const QDateTime& _time)
{
    d->lastEditTime = _time;
}

bool Project::canAskAboutSwitch() const
{
    return d->canAskAboutSwitch;
}

void Project::setCanAskAboutSwitch(bool _can)
{
    d->canAskAboutSwitch = _can;
}

bool Project::canBeSynced() const
{
    return d->canBeSynced;
}

void Project::setCanBeSynced(bool _can)
{
    d->canBeSynced = _can;
}

int Project::id() const
{
    return d->id;
}

void Project::setId(int _id)
{
    d->id = _id;
}

bool Project::isOwner() const
{
    return d->isOwner;
}

void Project::setOwner(bool _isOwner)
{
    d->isOwner = _isOwner;
}

DocumentEditingMode Project::editingMode() const
{
    return d->editingMode;
}

void Project::setEditingMode(DocumentEditingMode _mode)
{
    d->editingMode = _mode;
}

bool Project::isReadOnly() const
{
    return d->editingMode == DocumentEditingMode::Read;
}

QHash<QUuid, DocumentEditingMode> Project::editingPermissions() const
{
    return d->editingPermissions;
}

void Project::setEditingPermissions(const QHash<QUuid, DocumentEditingMode>& _permissions)
{
    d->editingPermissions = _permissions;
}

void Project::clearEditingPermissions()
{
    d->editingPermissions.clear();
}

QVector<Domain::ProjectCollaboratorInfo> Project::collaborators() const
{
    return d->collaborators;
}

void Project::setCollaborators(const QVector<Domain::ProjectCollaboratorInfo>& _collaborators)
{
    d->collaborators = _collaborators;
}

QVariant Project::data(int _role) const
{
    switch (_role) {
    case ProjectDataRole::Type: {
        return static_cast<int>(type());
    }

    case ProjectDataRole::Path: {
        return path();
    }

    case ProjectDataRole::PosterPath: {
        return posterPath();
    }

    case ProjectDataRole::Name: {
        return name();
    }

    case ProjectDataRole::Logline: {
        return logline();
    }

    case ProjectDataRole::LastEditTime: {
        return lastEditTime();
    }

    default: {
        return {};
    }
    }
}

bool operator==(const Project& _lhs, const Project& _rhs)
{
    if (!_lhs.uuid().isNull() && !_rhs.uuid().isNull()) {
        return _lhs.uuid() == _rhs.uuid();
    }

    return _lhs.type() == _rhs.type() && _lhs.path() == _rhs.path() && _lhs.name() == _rhs.name()
        && _lhs.logline() == _rhs.logline() && _lhs.lastEditTime() == _rhs.lastEditTime();
}


// ****


class ProjectsModel::Implementation
{
public:
    QVector<Project*> projects;
};


// **


ProjectsModel::ProjectsModel(QObject* _parent)
    : QAbstractListModel(_parent)
    , d(new Implementation)
{
}

ProjectsModel::~ProjectsModel()
{
    qDeleteAll(d->projects);
    d->projects.clear();
}

Project* ProjectsModel::projectAt(int _row) const
{
    Q_ASSERT(_row >= 0 && _row < d->projects.size());
    return d->projects.at(_row);
}

Project* ProjectsModel::projectForIndex(const QModelIndex& _index) const
{
    return projectAt(_index.row());
}

void ProjectsModel::append(const Project& _project)
{
    beginInsertRows({}, d->projects.size(), d->projects.size());
    d->projects.append(new Project(_project));
    endInsertRows();
}

void ProjectsModel::append(const QVector<Project>& _projects)
{
    if (_projects.isEmpty()) {
        return;
    }

    beginInsertRows({}, d->projects.size(), d->projects.size() + _projects.size() - 1);
    for (const auto& project : _projects) {
        d->projects.append(new Project(project));
    }
    endInsertRows();
}

void ProjectsModel::prepend(const Project& _project)
{
    beginInsertRows({}, 0, 0);
    d->projects.prepend(new Project(_project));
    endInsertRows();
}

void ProjectsModel::remove(const Project& _project)
{
    if (d->projects.isEmpty()) {
        return;
    }

    for (int projectIndex = 0; projectIndex < d->projects.size(); ++projectIndex) {
        if (*d->projects[projectIndex] == _project) {
            beginRemoveRows({}, projectIndex, projectIndex);
            auto project = d->projects.takeAt(projectIndex);
            delete project;
            project = nullptr;
            endRemoveRows();
            break;
        }
    }
}

void ProjectsModel::moveProject(Project* _item, Project* _afterSiblingItem, Project* _parentItem)
{
    Q_UNUSED(_parentItem)

    //
    // Попытка переметить тот же самый проект
    //
    if (_item == _afterSiblingItem) {
        return;
    }

    const int kInvalidIndex = -1;

    //
    // Перемещаемого проекта нет в списке
    //
    const int movedProjectIndex = d->projects.indexOf(_item);
    if (movedProjectIndex == kInvalidIndex) {
        return;
    }

    //
    // Проект перемещается в самое начало
    //
    if (_afterSiblingItem == nullptr) {
        //
        // Проект и так уже самый первый
        //
        if (movedProjectIndex == 0) {
            return;
        }

        beginMoveRows({}, movedProjectIndex, movedProjectIndex, {}, 0);
        d->projects.move(movedProjectIndex, 0);
        endMoveRows();
        return;
    }

    //
    // Проект перемещается внутри списка
    //
    const int insertAfterProjectIndex = d->projects.indexOf(_afterSiblingItem);

    //
    // Проекта, после которого нужно вставить, нет в в списке
    //
    if (insertAfterProjectIndex == kInvalidIndex) {
        return;
    }

    //
    // Проекты и так уже идут друг за другом
    //
    if ((movedProjectIndex - 1) == insertAfterProjectIndex) {
        return;
    }

    //
    // Перемещаем
    //
    beginMoveRows({}, movedProjectIndex, movedProjectIndex, {}, insertAfterProjectIndex + 1);
    d->projects.move(movedProjectIndex,
                     movedProjectIndex > insertAfterProjectIndex ? insertAfterProjectIndex + 1
                                                                 : insertAfterProjectIndex);
    endMoveRows();
}

void ProjectsModel::updateProject(const Project& _project)
{
    for (int projectIndex = 0; projectIndex < d->projects.size(); ++projectIndex) {
        if (projectAt(projectIndex)->path() == _project.path()) {
            *d->projects[projectIndex] = _project;
            emit dataChanged(index(projectIndex), index(projectIndex));
            break;
        }
    }
}

bool ProjectsModel::isEmpty() const
{
    return d->projects.isEmpty();
}

QModelIndex ProjectsModel::index(int _row, int _column, const QModelIndex& _parent) const
{
    if (_parent.isValid()) {
        return {};
    }

    if (_row < 0 || _row > rowCount(_parent)) {
        return {};
    }

    return createIndex(_row, _column, d->projects[_row]);
}

int ProjectsModel::rowCount(const QModelIndex& _parent) const
{
    if (_parent.isValid()) {
        return 0;
    }

    return d->projects.size();
}

QVariant ProjectsModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    const int projectIndex = _index.row();
    if (projectIndex < 0 || projectIndex >= d->projects.size()) {
        return {};
    }

    const auto project = d->projects.at(projectIndex);
    return project->data(_role);
}

bool ProjectsModel::moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
                             const QModelIndex& _destinationParent, int _destinationRow)
{
    Q_ASSERT(_count == 1);
    Project* item = projectForIndex(index(_sourceRow, 0, _sourceParent));
    Project* afterSiblingItem = nullptr;
    if (_destinationRow != 0) {
        afterSiblingItem = projectForIndex(index(_destinationRow - 1, 0, _destinationParent));
    }
    Project* parentItem = nullptr; // projectForIndex(_destinationParent);
    moveProject(item, afterSiblingItem, parentItem);
    return true;
}

} // namespace ManagementLayer

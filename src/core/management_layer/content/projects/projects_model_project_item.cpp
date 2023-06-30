#include "projects_model_project_item.h"

#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>

#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QPixmap>


namespace BusinessLayer {

class ProjectsModelProjectItem::Implementation
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
    int id = Domain::kInvalidId;
    int teamId = Domain::kInvalidId;
    bool isOwner = true;
    ManagementLayer::DocumentEditingMode editingMode = ManagementLayer::DocumentEditingMode::Edit;
    QHash<QUuid, ManagementLayer::DocumentEditingMode> editingPermissions;
    QVector<Domain::ProjectCollaboratorInfo> collaborators;
};


// **

QString ProjectsModelProjectItem::extension()
{
    return ".starc";
}


ProjectsModelProjectItem::ProjectsModelProjectItem()
    : ProjectsModelItem()
    , d(new Implementation)
{
}

ProjectsModelProjectItem::ProjectsModelProjectItem(const ProjectsModelProjectItem& _other)
    : ProjectsModelItem()
    , d(new Implementation(*_other.d))
{
}

ProjectsModelProjectItem::~ProjectsModelProjectItem() = default;

const ProjectsModelProjectItem& ProjectsModelProjectItem::operator=(
    const ProjectsModelProjectItem& _other)
{
    d.reset(new Implementation(*_other.d));
    return *this;
}

ProjectsModelItemType ProjectsModelProjectItem::type() const
{
    return ProjectsModelItemType::Project;
}

bool ProjectsModelProjectItem::isLocal() const
{
    return d->type == ProjectType::Local || d->type == ProjectType::LocalShadow;
}

bool ProjectsModelProjectItem::isRemote() const
{
    return d->type == ProjectType::Cloud;
}

ProjectType ProjectsModelProjectItem::projectType() const
{
    return d->type;
}

void ProjectsModelProjectItem::setProjectType(ProjectType _type)
{
    d->type = _type;
}

QString ProjectsModelProjectItem::path() const
{
    return d->path;
}

void ProjectsModelProjectItem::setPath(const QString& _path)
{
    d->path = _path;

    if (d->type == ProjectType::Local || d->type == ProjectType::LocalShadow) {
        d->editingMode = QFileInfo(d->path).isWritable()
            ? ManagementLayer::DocumentEditingMode::Edit
            : ManagementLayer::DocumentEditingMode::Read;
    }
}

QString ProjectsModelProjectItem::realPath() const
{
    return d->realPath;
}

void ProjectsModelProjectItem::setRealPath(const QString& _path)
{
    d->realPath = _path;
}

const QPixmap& ProjectsModelProjectItem::poster() const
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

QString ProjectsModelProjectItem::posterPath() const
{
    return d->posterPath;
}

void ProjectsModelProjectItem::setPosterPath(const QString& _path)
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

QUuid ProjectsModelProjectItem::uuid() const
{
    return d->uuid;
}

void ProjectsModelProjectItem::setUuid(const QUuid& _uuid)
{
    d->uuid = _uuid;
}

QString ProjectsModelProjectItem::name() const
{
    return d->name;
}

void ProjectsModelProjectItem::setName(const QString& _name)
{
    d->name = _name;
}

QString ProjectsModelProjectItem::logline() const
{
    return d->logline;
}

void ProjectsModelProjectItem::setLogline(const QString& _logline)
{
    d->logline = _logline;
}

QString ProjectsModelProjectItem::displayLastEditTime() const
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

QDateTime ProjectsModelProjectItem::lastEditTime() const
{
    return d->lastEditTime;
}

void ProjectsModelProjectItem::setLastEditTime(const QDateTime& _time)
{
    d->lastEditTime = _time;
}

bool ProjectsModelProjectItem::canAskAboutSwitch() const
{
    return d->canAskAboutSwitch;
}

void ProjectsModelProjectItem::setCanAskAboutSwitch(bool _can)
{
    d->canAskAboutSwitch = _can;
}

bool ProjectsModelProjectItem::canBeSynced() const
{
    return d->canBeSynced;
}

void ProjectsModelProjectItem::setCanBeSynced(bool _can)
{
    d->canBeSynced = _can;
}

int ProjectsModelProjectItem::id() const
{
    return d->id;
}

void ProjectsModelProjectItem::setId(int _id)
{
    d->id = _id;
}

int ProjectsModelProjectItem::teamId() const
{
    return d->teamId;
}

void ProjectsModelProjectItem::setTeamId(int _id)
{
    d->teamId = _id;
}

bool ProjectsModelProjectItem::isOwner() const
{
    return d->isOwner;
}

void ProjectsModelProjectItem::setOwner(bool _isOwner)
{
    d->isOwner = _isOwner;
}

ManagementLayer::DocumentEditingMode ProjectsModelProjectItem::editingMode() const
{
    return d->editingMode;
}

void ProjectsModelProjectItem::setEditingMode(ManagementLayer::DocumentEditingMode _mode)
{
    d->editingMode = _mode;
}

bool ProjectsModelProjectItem::isReadOnly() const
{
    return d->editingMode == ManagementLayer::DocumentEditingMode::Read;
}

QHash<QUuid, ManagementLayer::DocumentEditingMode> ProjectsModelProjectItem::editingPermissions()
    const
{
    return d->editingPermissions;
}

void ProjectsModelProjectItem::setEditingPermissions(
    const QHash<QUuid, ManagementLayer::DocumentEditingMode>& _permissions)
{
    d->editingPermissions = _permissions;
}

void ProjectsModelProjectItem::clearEditingPermissions()
{
    d->editingPermissions.clear();
}

QVector<Domain::ProjectCollaboratorInfo> ProjectsModelProjectItem::collaborators() const
{
    return d->collaborators;
}

void ProjectsModelProjectItem::setCollaborators(
    const QVector<Domain::ProjectCollaboratorInfo>& _collaborators)
{
    d->collaborators = _collaborators;
}

bool operator==(const ProjectsModelProjectItem& _lhs, const ProjectsModelProjectItem& _rhs)
{
    if (!_lhs.uuid().isNull() && !_rhs.uuid().isNull()) {
        return _lhs.uuid() == _rhs.uuid();
    }

    return _lhs.projectType() == _rhs.projectType() && _lhs.path() == _rhs.path()
        && _lhs.name() == _rhs.name() && _lhs.logline() == _rhs.logline()
        && _lhs.lastEditTime() == _rhs.lastEditTime();
}

} // namespace BusinessLayer

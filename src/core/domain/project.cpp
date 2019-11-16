#include "project.h"

#include <QApplication>
#include <QDateTime>
#include <QPixmap>


namespace Domain
{

class Project::Implementation
{
public:
    ProjectType type = ProjectType::Invalid;
    QString path = "/home/lucas/screenplays/tlj.starc";

    mutable QPixmap poster;
    QString posterPath;
    QString name = "Star Wars";
    QString logline = "Rey develops her newly discovered abilities with the guidance of Luke Skywalker, who is unsettled by the strength of her powers. Meanwhile, the Resistance prepares for battle with the First Order. ";
    QDateTime lastEditTime = QDateTime::fromString("2019-10-01 22:48:56", "yyyy-MM-dd hh:mm:ss");
};


// **


Project::Project()
    : d(new Implementation)
{
}

Project::Project(const Project& _other)
    : d(new Implementation(*_other.d))
{
}

const Project& Project::operator=(const Project& _other)
{
    d.reset(new Implementation(*_other.d));
    return *this;
}

Project::~Project() = default;

ProjectType Project::type() const
{
    return d->type;
}

void Project::setType(ProjectType _type)
{
    d->type = _type;
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

QString Project::displayPath() const
{
    //
    // TODO: Remote project path
    //
    return d->path;
}

QString Project::path() const
{
    return d->path;
}

void Project::setPath(const QString& _path)
{
    d->path = _path;
}

const QPixmap& Project::poster() const
{
    if (d->poster.isNull()) {
        if (!d->poster.load(d->posterPath)) {
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

QString Project::displayLastEditTime() const
{
    switch (d->lastEditTime.daysTo(QDateTime::currentDateTime())) {
        case 0: {
            return QApplication::translate("Domain::Project", "today at") + d->lastEditTime.toString(" hh:mm");
        }

        case 1: {
            return QApplication::translate("Domain::Project", "tomorrow at") + d->lastEditTime.toString(" hh:mm");
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
    return _lhs.type() == _rhs.type()
            && _lhs.path() == _rhs.path()
            && _lhs.posterPath() == _rhs.posterPath()
            && _lhs.name() == _rhs.name()
            && _lhs.logline() == _rhs.logline()
            && _lhs.lastEditTime() == _rhs.lastEditTime();
}


// ****


class ProjectsModel::Implementation
{
public:
    QVector<Project> projects;
};


// **


ProjectsModel::ProjectsModel(QObject* _parent)
    : QAbstractListModel(_parent),
      d(new Implementation)
{
}

Project ProjectsModel::projectAt(int _row) const
{
    Q_ASSERT(_row >= 0 && _row < d->projects.size());
    return d->projects.at(_row);
}

ProjectsModel::~ProjectsModel() = default;

void ProjectsModel::append(const Project &_project)
{
    beginInsertRows({}, d->projects.size(), d->projects.size());
    d->projects.append(_project);
    endInsertRows();
}

void ProjectsModel::append(const QVector<Project>& _projects)
{
    beginInsertRows({}, d->projects.size(), d->projects.size() + _projects.size() - 1);
    d->projects.append(_projects);
    endInsertRows();
}

void ProjectsModel::prepend(const Project& _project)
{
    beginInsertRows({}, 0, 0);
    d->projects.prepend(_project);
    endInsertRows();
}

void ProjectsModel::remove(const Project &_project)
{
    const int kInvalidIndex = -1;
    const int projectIndex = d->projects.indexOf(_project);
    if (projectIndex == kInvalidIndex) {
        return;
    }

    beginRemoveRows({}, projectIndex, projectIndex);
    d->projects.removeAt(projectIndex);
    endRemoveRows();
}

bool ProjectsModel::moveProject(const Project& _moved, const Project& _insertAfter)
{
    //
    // Попытка переметить тот же самый проект
    //
    if (_moved == _insertAfter) {
        return false;
    }

    const int kInvalidIndex = -1;

    //
    // Перемещаемого проекта нет в списке
    //
    const int movedProjectIndex = d->projects.indexOf(_moved);
    if (movedProjectIndex == kInvalidIndex) {
        return false;
    }

    //
    // Проект перемещается в самое начало
    //
    if (_insertAfter == Project()) {
        //
        // Проект и так уже самый первый
        //
        if (movedProjectIndex == 0) {
            return false;
        }

        beginMoveRows({}, movedProjectIndex, movedProjectIndex, {}, 0);
        d->projects.move(movedProjectIndex, 0);
        endMoveRows();
        return true;
    }

    //
    // Проект перемещается внутри списка
    //
    const int insertAfterProjectIndex = d->projects.indexOf(_insertAfter);

    //
    // Проекта, после которого нужно вставить, нет в в списке
    //
    if (insertAfterProjectIndex == kInvalidIndex) {
        return false;
    }

    //
    // Проекты и так уже идут друг за другом
    //
    if ((movedProjectIndex - 1) == insertAfterProjectIndex) {
        return false;
    }

    //
    // Перемещаем
    //
    beginMoveRows({}, movedProjectIndex, movedProjectIndex, {}, insertAfterProjectIndex + 1);
    d->projects.move(movedProjectIndex, movedProjectIndex > insertAfterProjectIndex
                                        ? insertAfterProjectIndex + 1
                                        : insertAfterProjectIndex);
    endMoveRows();
    return true;
}

bool ProjectsModel::isEmpty() const
{
    return d->projects.isEmpty();
}

int ProjectsModel::rowCount(const QModelIndex& _parent) const
{
    Q_UNUSED(_parent)
    return d->projects.size();
}

QVariant ProjectsModel::data(const QModelIndex& _index, int _role) const
{
    if (!_index.isValid()) {
        return {};
    }

    const int projectIndex = _index.row();
    if (projectIndex < 0
        || projectIndex >= d->projects.size()) {
        return {};
    }

    const Project& project = d->projects.at(projectIndex);
    return project.data(_role);
}

} // namespace Domain

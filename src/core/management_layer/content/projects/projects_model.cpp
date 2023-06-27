#include "projects_model.h"

#include "project.h"


namespace BusinessLayer {

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

    if (_row < 0 || _row >= rowCount(_parent)) {
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

} // namespace BusinessLayer

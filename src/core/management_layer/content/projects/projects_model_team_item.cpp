#include "projects_model_team_item.h"

#include <domain/starcloud_api.h>

#include <QPixmap>
#include <QVariant>


namespace BusinessLayer {

class ProjectsModelTeamItem::Implementation
{
public:
    Domain::TeamInfo teamInfo;
    bool isOpened = true;
};


// ****


ProjectsModelTeamItem::ProjectsModelTeamItem()
    : d(new Implementation)
{
}

ProjectsModelTeamItem::~ProjectsModelTeamItem() = default;

ProjectsModelItemType ProjectsModelTeamItem::type() const
{
    return ProjectsModelItemType::Team;
}

void ProjectsModelTeamItem::setTeamInfo(const Domain::TeamInfo& _teamInfo)
{
    d->teamInfo = _teamInfo;
    setChanged(true);
}

int ProjectsModelTeamItem::id() const
{
    return d->teamInfo.id;
}

void ProjectsModelTeamItem::setId(int _id)
{
    if (d->teamInfo.id == _id) {
        return;
    }

    d->teamInfo.id = _id;
    setChanged(true);
}

QString ProjectsModelTeamItem::name() const
{
    return d->teamInfo.name;
}

void ProjectsModelTeamItem::setName(const QString& _name)
{
    if (d->teamInfo.name == _name) {
        return;
    }

    d->teamInfo.name = _name;
    setChanged(true);
}

QString ProjectsModelTeamItem::description() const
{
    return d->teamInfo.description;
}

void ProjectsModelTeamItem::setDescription(const QString& _description)
{
    if (d->teamInfo.description == _description) {
        return;
    }

    d->teamInfo.description = _description;
    setChanged(true);
}

QByteArray ProjectsModelTeamItem::avatar() const
{
    return d->teamInfo.avatar;
}

void ProjectsModelTeamItem::setAvatar(const QByteArray& _avatar)
{
    d->teamInfo.avatar = _avatar;
    setChanged(true);
}

bool ProjectsModelTeamItem::allowGrantAccessToProject() const
{
    return d->teamInfo.allowGrantAccessToProjects;
}

QVector<Domain::TeamMemberInfo> ProjectsModelTeamItem::members() const
{
    return d->teamInfo.members;
}

bool ProjectsModelTeamItem::isOpened() const
{
    return d->isOpened;
}

void ProjectsModelTeamItem::setOpened(bool _opened)
{
    d->isOpened = _opened;
}

} // namespace BusinessLayer

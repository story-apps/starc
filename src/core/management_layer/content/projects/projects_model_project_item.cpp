#include "projects_model_project_item.h"

#include <business_layer/compliance/compliance_checker.h>
#include <domain/starcloud_api.h>
#include <interfaces/management_layer/i_document_manager.h>
#include <utils/helpers/image_helper.h>

#include <QApplication>
#include <QDateTime>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
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
    bool isReviewEnabled = false;
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
    if (!d->poster.isNull() && d->posterPath == _path) {
        //
        // Если постер уже был инициилизирован, но путь не изменился, проверяем реальное сходство
        // двух изображений, чтобы покрыть кейс, когда постер обновился на сервере и теперь его
        // нужно обновить локально, но при этом путь остался тем же самым
        //
        if (ImageHelper::isImagesEqual(d->poster, QPixmap(d->posterPath))) {
            return;
        }
    }
    d->posterPath = _path;

    //
    // Обнуляем постер, чтобы он потом извлёкся по заданному пути
    //
    d->poster = {};

    setChanged(true);
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
    setChanged(true);
}

QString ProjectsModelProjectItem::logline() const
{
    return d->logline;
}

void ProjectsModelProjectItem::setLogline(const QString& _logline)
{
    d->logline = _logline;
    setChanged(true);
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

bool ProjectsModelProjectItem::isReviewEnabled() const
{
    return d->isReviewEnabled;
}

void ProjectsModelProjectItem::setReviewEnabled(bool _enabled)
{
    d->isReviewEnabled = _enabled;
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

QVector<ComplianceRule> ProjectsModelProjectItem::complianceRules() const
{
    //
    // TODO: Пока доступно только для конкретных проектов, вынести в параметры проекта
    //
    switch (d->id) {
    case 3661: {
        const QStringList primaryLocations = {
            "ШКОЛА. КАБИНЕТ ВШ",        "ШКОЛА. КАБИНЕТ ВШ. КОРИДОР",
            "ШКОЛА. КАБИНЕТ ДИРЕКТОРА", "ШКОЛА. КАБИНЕТ ДИРЕКТОРА. КОРИДОР",
            "ШКОЛА. КАБИНЕТ ЗАВУЧА",    "ШКОЛА. КАБИНЕТ ЗАВУЧА. КОРИДОР",
            "ШКОЛА. УЧИТЕЛЬСКАЯ",       "ШКОЛА. УЧИТЕЛЬСКАЯ. КОРИДОР",
        };
        return {
            { BusinessLayer::ComplianceRuleType::TotalDuration, {}, true, 23 * 60, 25 * 60, {} },
            { BusinessLayer::ComplianceRuleType::ScenesCount, {}, true, 16, 24, {} },
            { BusinessLayer::ComplianceRuleType::SceneDuration, {}, true, 40, 80, {} },
            { BusinessLayer::ComplianceRuleType::CharacterShouldSpeakInEveryScene,
              {},
              true,
              {},
              {},
              {
                  "Димон",
                  "Завуч",
              } },
            { BusinessLayer::ComplianceRuleType::SceneMaxCharactersCount, {}, false, 0, 5, {} },
            { BusinessLayer::ComplianceRuleType::PrimaryLocationsPercent,
              {},
              false,
              70,
              0,
              primaryLocations },
            { BusinessLayer::ComplianceRuleType::SecondaryLocationsCount,
              {},
              false,
              0,
              3,
              primaryLocations },
            { BusinessLayer::ComplianceRuleType::SecondaryLocationsSceneCount,
              {},
              false,
              3,
              0,
              primaryLocations },
            { BusinessLayer::ComplianceRuleType::SecondaryLocationsNightScenePercent,
              {},
              false,
              0,
              10,
              primaryLocations },
        };
        break;
    }

    case 4568:
    case 4702: {
        const QStringList primaryLocations = {
            "ТРЕШКА. *",
            "КВАРТИРА ДИМЫ. КОМНАТА ДИМЫ",
            "ЗАВОД. КАБИНЕТ РОМАНА",
            "ЗАВОД. КАБИНЕТ КАДРОВИЧКИ",
        };
        return {
            { BusinessLayer::ComplianceRuleType::TotalDuration, {}, true, 23 * 60, 25 * 60, {} },
            { BusinessLayer::ComplianceRuleType::ScenesCount, {}, true, 16, 24, {} },
            { BusinessLayer::ComplianceRuleType::SceneDuration, {}, true, 40, 80, {} },
            { BusinessLayer::ComplianceRuleType::CharacterShouldSpeakInEveryScene,
              {},
              true,
              {},
              {},
              {
                  "Любовь",
                  "Гриша",
                  "Максим",
              } },
            { BusinessLayer::ComplianceRuleType::SceneMaxCharactersCount, {}, false, 0, 5, {} },
            { BusinessLayer::ComplianceRuleType::PrimaryLocationsPercent,
              {},
              false,
              70,
              0,
              primaryLocations },
            { BusinessLayer::ComplianceRuleType::SecondaryLocationsCount,
              {},
              false,
              0,
              3,
              primaryLocations },
            { BusinessLayer::ComplianceRuleType::SecondaryLocationsSceneCount,
              {},
              false,
              3,
              0,
              primaryLocations },
            { BusinessLayer::ComplianceRuleType::SecondaryLocationsNightScenePercent,
              {},
              false,
              0,
              10,
              primaryLocations },
        };
        break;
    }

    case 4566: {
        QJsonArray line1Locations;
        line1Locations.append("ДОМ НИКОЛАЯ. *");
        QJsonArray line1Characters;
        line1Characters.append("НИКОЛАЙ");
        line1Characters.append("АННА");
        line1Characters.append("МУРАТОВ");
        line1Characters.append("КНЯГИНЯ");
        line1Characters.append("АПОЛЛИНАРИЯ");
        line1Characters.append("МУРАВИНСКИЙ");
        line1Characters.append("СИМКА");
        line1Characters.append("ПОТАП");
        QJsonObject line1;
        line1["name"] = "Линия 1";
        line1["long_scene_threshold"] = 2 * 8; // две минуты в восьмушках
        line1["locations"] = line1Locations;
        line1["characters"] = line1Characters;
        //
        QJsonArray line2Locations;
        line2Locations.append("ДОМ АВЕРИНЫХ. *");
        line2Locations.append("ДОМ СПИЦИНЫХ. *");
        line2Locations.append("КОНЮШНЯ");
        line2Locations.append("МАГАЗИН ОДЕЖДЫ");
        QJsonArray line2Characters;
        line2Characters.append("ЕЛЕНА");
        line2Characters.append("ЕКАТЕРИНА");
        line2Characters.append("РОМА");
        line2Characters.append("ЮЛЯ");
        line2Characters.append("АВЕРИН");
        line2Characters.append("СПИЦИН");
        line2Characters.append("СМИРНОВ");
        QJsonObject line2;
        line2["name"] = "Линия 2";
        line2["long_scene_threshold"] = 2 * 8; // две минуты в восьмушках
        line2["locations"] = line2Locations;
        line2["characters"] = line2Characters;
        //
        QJsonArray lines;
        lines.append(line1);
        lines.append(line2);
        //
        const QString linesRequirements = QJsonDocument(lines).toJson(QJsonDocument::Compact);
        return {
            { BusinessLayer::ComplianceRuleType::TotalPages, {}, true, 46, 48, {} },
            { BusinessLayer::ComplianceRuleType::ScenesDistributionByLocationsAndCharacters,
              {},
              false,
              50,
              52,
              { line1["name"].toString(), linesRequirements } },
            { BusinessLayer::ComplianceRuleType::ScenesDistributionByLocationsAndCharacters,
              {},
              false,
              48,
              50,
              { line2["name"].toString(), linesRequirements } },
            { BusinessLayer::ComplianceRuleType::LongScenesByLocationsAndCharacters,
              {},
              false,
              50,
              0,
              { line1["name"].toString(), linesRequirements } },
            { BusinessLayer::ComplianceRuleType::LongScenesByLocationsAndCharacters,
              {},
              false,
              80,
              0,
              { line2["name"].toString(), linesRequirements } },
        };
        break;
    }

    default: {
        break;
    }
    }

    return {};
}

void ProjectsModelProjectItem::setComplianceRules(const QVector<ComplianceRule>& _complianceRules)
{
    Q_UNUSED(_complianceRules)
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

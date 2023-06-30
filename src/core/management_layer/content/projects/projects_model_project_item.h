#pragma once

#include "projects_model_item.h"

class QDateTime;
class QPixmap;
class QUuid;

namespace Domain {
struct ProjectCollaboratorInfo;
}

namespace ManagementLayer {
enum class DocumentEditingMode;
}


namespace BusinessLayer {

/**
 * @brief Тип проекта
 */
enum class ProjectType {
    Invalid,
    Local,
    LocalShadow,
    Cloud,
};

/**
 * @brief Файл проекта
 */
class ProjectsModelProjectItem : public ProjectsModelItem
{
public:
    /**
     * @brief Разрешение файла проекта
     */
    static QString extension();

public:
    ProjectsModelProjectItem();
    ProjectsModelProjectItem(const ProjectsModelProjectItem& _other);
    const ProjectsModelProjectItem& operator=(const ProjectsModelProjectItem& _other);
    ~ProjectsModelProjectItem();

    /**
     * @brief Тип элемента модели
     */
    ProjectsModelItemType type() const override;

    /**
     * @brief Тип проекта
     */
    bool isLocal() const;
    bool isRemote() const;
    ProjectType projectType() const;
    void setProjectType(ProjectType _type);

    /**
     * @brief Путь к исходному файлу проекта
     */
    QString path() const;
    void setPath(const QString& _path);

    /**
     * @brief Доподлинный путь к проекту
     */
    QString realPath() const;
    void setRealPath(const QString& _path);

    /**
     * @brief Путь к постеру проекта
     */
    const QPixmap& poster() const;
    QString posterPath() const;
    void setPosterPath(const QString& _path);

    /**
     * @brief Идентификатор проекта
     */
    QUuid uuid() const;
    void setUuid(const QUuid& _uuid);

    /**
     * @brief Название проекта
     */
    QString name() const;
    void setName(const QString& _name);

    /**
     * @brief Логлайн проекта
     */
    QString logline() const;
    void setLogline(const QString& _logline);

    /**
     * @brief Дата и время последнего изменения проекта
     */
    QString displayLastEditTime() const;
    QDateTime lastEditTime() const;
    void setLastEditTime(const QDateTime& _time);

    /**
     * @brief Можно ли показывать диалог с вопросом о смене типа теневого проекта
     */
    bool canAskAboutSwitch() const;
    void setCanAskAboutSwitch(bool _can);

    /**
     * @brief Может ли проект быть синхронизированным
     */
    bool canBeSynced() const;
    void setCanBeSynced(bool _can);

    /**
     * @brief Идентификатор проекта
     */
    int id() const;
    void setId(int _id);

    /**
     * @brief Идентификатор команды проекта
     */
    int teamId() const;
    void setTeamId(int _id);

    /**
     * @brief Является ли текущий пользователь владельцем проекта
     */
    bool isOwner() const;
    void setOwner(bool _isOwner);

    /**
     * @brief Режим работы с проектом
     */
    ManagementLayer::DocumentEditingMode editingMode() const;
    void setEditingMode(ManagementLayer::DocumentEditingMode _mode);
    bool isReadOnly() const;

    /**
     * @brief Разрешения на работу с конкретными документами
     */
    QHash<QUuid, ManagementLayer::DocumentEditingMode> editingPermissions() const;
    void setEditingPermissions(
        const QHash<QUuid, ManagementLayer::DocumentEditingMode>& _permissions);
    void clearEditingPermissions();

    /**
     * @brief Список соавторов
     */
    QVector<Domain::ProjectCollaboratorInfo> collaborators() const;
    void setCollaborators(const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

bool operator==(const ProjectsModelProjectItem& _lhs, const ProjectsModelProjectItem& _rhs);

} // namespace BusinessLayer

#pragma once

#include "projects_model_item.h"

namespace Domain {
struct TeamInfo;
struct TeamMemberInfo;
} // namespace Domain


namespace BusinessLayer {

/**
 * @brief Команда, в которой размещаются проекты
 */
class ProjectsModelTeamItem : public ProjectsModelItem
{
public:
    ProjectsModelTeamItem();
    ~ProjectsModelTeamItem() override;

    /**
     * @brief Тип элемента модели
     */
    ProjectsModelItemType type() const override;

    /**
     * @brief Информация о команде
     */
    void setTeamInfo(const Domain::TeamInfo& _teamInfo);

    /**
     * @brief Идентификатор команды
     */
    int id() const;
    void setId(int _id);

    /**
     * @brief Название команды
     */
    QString name() const;
    void setName(const QString& _name);

    /**
     * @brief Описание команды
     */
    QString description() const;
    void setDescription(const QString& _description);

    /**
     * @brief Аватар команды
     */
    QByteArray avatar() const;
    void setAvatar(const QByteArray& _avatar);

    /**
     * @brief Может ли пользователь предоставлять доступ к документам проекта
     */
    bool allowGrantAccessToProject() const;

    /**
     * @brief Участники команды
     */
    QVector<Domain::TeamMemberInfo> members() const;

    /**
     * @brief Открыта ли команда в представлении
     */
    bool isOpened() const;
    void setOpened(bool _opened);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

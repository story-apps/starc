#pragma once

#include <QObject>

namespace Domain {
enum class SubscriptionType;
struct ProjectInfo;
struct TeamInfo;
} // namespace Domain

namespace BusinessLayer {
class Project;
}


namespace ManagementLayer {

/**
 * @brief Менеджер экрана со списком проектов
 */
class ProjectsManager : public QObject
{
    Q_OBJECT

public:
    ProjectsManager(QObject* _parent, QWidget* _parentWidget);
    ~ProjectsManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Показать/скрыть наличие непрочитанных комментариев
     */
    void setHasUnreadNotifications(bool _hasUnreadNotifications);

    /**
     * @brief Загрузить список проектов
     */
    void loadProjects();

    /**
     * @brief Сохранить список проектов
     */
    void saveProjects();

    /**
     * @brief Обновить дату последнего изменения проекта
     */
    void saveChanges();

    /**
     * @brief Скорректировать интерфейс в зависимости от того есть ли подключение к серверу
     */
    void setConnected(bool _connected);

    /**
     * @brief Можно ли создавать проекты в облаке
     */
    void setProjectsInCloudCanBeCreated(bool _authorized,
                                        Domain::SubscriptionType _subscritionType);

    /**
     * @brief Список команд пользователя
     */
    void setTeams(const QVector<Domain::TeamInfo>& _teams);

    /**
     * @brief Создать проект
     */
    void createProject();

    /**
     * @brief Выбрать проект для открытия
     */
    void openProject();

    /**
     * @brief Задать список облачных проектов
     */
    void setCloudProjects(const QVector<Domain::ProjectInfo>& _projects);

    /**
     * @brief Добавить облачный проект
     */
    void addOrUpdateCloudProject(const Domain::ProjectInfo& _projectInfo);

    /**
     * @brief Установить текущий проект
     */
    void setCurrentProject(const QString& _path);

    /**
     * @brief Установить текущий проект
     * @note Для случаев, когда в программе открывается проект другой программы, @b _path указвыает
     *       путь к исходному файлу, а @b _realPath путь к временному файлу проекта
     */
    void setCurrentProject(const QString& _path, const QString& _realPath);

    /**
     * @brief Установить текущий проект
     */
    void setCurrentProject(int _projectId);

    /**
     * @brief Установить гуид текущего проекта
     */
    void setCurrentProjectUuid(const QUuid& _uuid);

    /**
     * @brief Установить название текущего проекта
     */
    void setCurrentProjectName(const QString& _name);

    /**
     * @brief Установить короткое описание текущего проекта
     */
    void setCurrentProjectLogline(const QString& _logline);

    /**
     * @brief Установить обложку текущего проекта
     */
    void setCurrentProjectCover(const QPixmap& _cover);

    /**
     * @brief Запомнить, что пользователь больше никогда нехочет видеть вопроса о переключении
     * формата проекта
     */
    void setCurrentProjectNeverAskAboutSwitch();

    /**
     * @brief Может ли текущий проект быть синхронизирован
     */
    void setCurrentProjectCanBeSynced(bool _can);

    /**
     * @brief Закрыть текущий проект
     */
    void closeCurrentProject();

    /**
     * @brief Получить проект
     */
    BusinessLayer::Project project(const QString& _path) const;
    BusinessLayer::Project project(int _id) const;

    /**
     * @brief Скрыть проект
     */
    void hideProject(const QString& _path);
    void hideProject(int _id);

    /**
     * @brief Удалить проект
     */
    void removeProject(int _id);

    /**
     * @brief Получить текущий проект
     */
    const BusinessLayer::Project& currentProject() const;

signals:
    /**
     * @brief Запрос на отображение меню
     */
    void menuRequested();

    /**
     * @brief Пользователь хочет авторизоваться
     */
    void signInRequested();

    /**
     * @brief Пользователь хочет оформить ТИМ подписку
     */
    void renewTeamSubscriptionRequested();

    /**
     * @brief Пользователь хочет создать проект
     */
    void createProjectRequested();

    /**
     * @brief Пользователь хочет создать локальный проект
     */
    void createLocalProjectRequested(const QString& _projectName, const QString& _projectFilePath,
                                     const QString& _importFilePath);

    /**
     * @brief Пользователь хочет создать проект в облаке
     */
    void createCloudProjectRequested(const QString& _projectName, const QString& _importFilePath,
                                     int _teamId);

    /**
     * @brief Пользователь хочет открыть проект
     */
    void openProjectRequested();

    /**
     * @brief Пользователь хочет открыть локальный проект по заданному пути
     */
    void openLocalProjectRequested(const QString& _path);

    /**
     * @brief Пользователь хочет открыть облачный проект
     */
    void openCloudProjectRequested(int _id, const QString& _path);

    /**
     * @brief Пользователь обновил параметры облачного проекта
     */
    void updateCloudProjectRequested(int _id, const QString& _name, const QString& _logline,
                                     const QByteArray& _cover);

    /**
     * @brief Пользователь хочет удалить облачный проект
     */
    void removeCloudProjectRequested(int _id);

    /**
     * @brief Пользователь хочет отписаться от облачного проекта
     */
    void unsubscribeFromCloudProjectRequested(int _id);

    /**
     * @brief Запрос на закрытие текущего проекта
     */
    void closeCurrentProjectRequested();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

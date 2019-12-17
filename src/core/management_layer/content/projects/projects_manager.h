#pragma once

#include <QObject>


namespace ManagementLayer
{

/**
 * @brief Менеджер экрана со списком проектов
 */
class ProjectsManager : public QObject
{
    Q_OBJECT

public:
    explicit ProjectsManager(QObject* _parent, QWidget* _parentWidget);
    ~ProjectsManager() override;

    QWidget* toolBar() const;
    QWidget* navigator() const;
    QWidget* view() const;

    /**
     * @brief Загрузить список проектов
     */
    void loadProjects();

    /**
     * @brief Можно ли создавать проекты в облаке
     */
    void setProjectsInCloudCanBeCreated(bool _authorized, bool _ableToCreate);

    /**
     * @brief Создать проект
     */
    void createProject();

    /**
     * @brief Установить текущий проект
     */
    void setCurrentProject(const QString& _path);

    /**
     * @brief Скрыть проект
     */
    void hideProject(const QString& _path);

signals:
    /**
     * @brief Запрос на отображение меню
     */
    void menuRequested();

    /**
     * @brief Пользователь хочет создать проект
     */
    void createProjectRequested();

    /**
     * @brief Пользователь хочет создать локальный проект
     */
    void createLocalProjectRequested(const QString& _projectFilePath, const QString& _importFilePath);

    /**
     * @brief Пользователь хочет создать проект в облаке
     */
    void createCloudProjectRequested(const QString& _projectName, const QString& _importFilePath);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

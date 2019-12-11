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
     * @brief Сохранить список проектов
     */
    void saveProjects();

    /**
     * @brief Можно ли создавать проекты в облаке
     */
    void setProjectsInCloudCanBeCreated(bool _authorized, bool _ableToCreate);

    /**
     * @brief Создать проект
     */
    void createProject();

signals:
    /**
     * @brief Запрос на отображение меню
     */
    void menuRequested();

    /**
     * @brief Пользователь хочет создать проект
     */
    void createProjectRequested();

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace ManagementLayer

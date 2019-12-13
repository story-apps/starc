#pragma once

#include <QObject>


namespace ManagementLayer
{

/**
 * @brief Проект
 */
class Project
{
public:
    /**
     * @brief Разрешение файла проекта
     */
    static QString extension();

public:
    Project();
    Project(const QString& _name, const QString& _path, const QString& _lastEditDatetime);

    /**
     * @brief Валиден ли проект
     */
    bool isValid() const;

    /**
     * @brief Название проекта
     */
    /** @{ */
    QString name() const;
    void setName(const QString& _name);
    /** @} */

    /**
     * @brief Путь к проекту
     */
    /** @{ */
    QString path() const;
    void setPath(const QString& _path);
    /** @} */

    /**
     * @brief Дата и время последнего изменения проекта
     */
    /** @{ */
    QString lastEditDatetime() const;
    void setLastEditDatetime(const QString& _datetime);
    /** @} */

private:
    /**
     * @brief Название проекта
     */
    QString m_name;

    /**
     * @brief Путь к файлу проекта
     */
    QString m_path;

    /**
     * @brief Дата и время последнего изменения проекта
     */
    QString m_lastEditDatetime;
};


/**
 * @brief Сравнить два проекта
 */
bool operator==(const Project& _lhs, const Project& _rhs);

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

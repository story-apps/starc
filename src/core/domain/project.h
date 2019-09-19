#pragma once

#include <QAbstractListModel>


namespace Domain
{

/**
 * @brief Тип проекта
 */
enum class ProjectType {
    Invalid,
    Local,
    Remote
};

enum ProjectDataRole {
    Type,
    Path,
    PosterPath,
    Name,
    Logline,
    LastEditTime
};

/**
 * @brief Файл проекта
 */
class Project
{
public:
    Project();
    Project(const Project& _other);
    const Project& operator=(const Project& _other);
    bool operator==(const Project& _other);
    ~Project();

    /**
     * @brief Тип проекта
     */
    ProjectType type() const;
    void setType(ProjectType _type);

    /**
     * @brief Путь к проекту
     */
    QString displayPath() const;
    QString path() const;
    void setPath(const QString& _path);

    /**
     * @brief Путь к постеру проекта
     */
    QString posterPath() const;
    void setPosterPath(const QString& _path);

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
    QDateTime lastEditTime() const;
    void setLastEditTime(const QDateTime& _time);

    /**
     * @brief Получить данные по роли из модели
     */
    QVariant data(int _role) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

/**
 * @brief Модель списка проектов
 */
class ProjectsModel : public QAbstractListModel
{
public:
    explicit ProjectsModel(QObject* _parent = nullptr);
    ~ProjectsModel() override;

    /**
     * @brief Получить проект по заданному индексу
     */
    Project projectAt(int _row) const;

    /**
     * @brief Добавить новый проект
     * @note Новый проект всегда добавляется в начало списка
     */
    void addProject(const QString& _name);

    /**
     * @brief Переопределяем методы для собственной реализации модели
     */
    int rowCount(const QModelIndex& _parent = {}) const override;
    QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace Domain

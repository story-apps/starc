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

/**
 * @brief Роли для данных проекта в модели
 */
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
    const QPixmap& poster() const;
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
    QString displayLastEditTime() const;
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

bool operator==(const Project& _lhs, const Project& _rhs);

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
     * @brief Добавить новый проект в конец списка
     */
    void append(const Project& _project);

    /**
     * @brief Добавить группу проектов в конец списка
     */
    void append(const QVector<Project>& _projects);

    /**
     * @brief Добавить новый проект в начало списка
     */
    void prepend(const Project& _project);

    /**
     * @brief Удалить проект
     */
    void remove(const Project& _project);

    /**
     * @brief Перенести @p _moved проект после @p _insertAfter
     */
    bool moveProject(const Project& _moved, const Project& _insertAfter);

    /**
     * @brief Пуста ли модель
     */
    bool isEmpty() const;

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

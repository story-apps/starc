#pragma once

#include <QAbstractListModel>


namespace Domain {
struct ProjectCollaboratorInfo;
}

namespace ManagementLayer {

enum class DocumentEditingMode;

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
 * @brief Роли для данных проекта в модели
 */
enum ProjectDataRole {
    Type,
    Path,
    PosterPath,
    Name,
    Logline,
    LastEditTime,
};

/**
 * @brief Файл проекта
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
    Project(const Project& _other);
    const Project& operator=(const Project& _other);
    ~Project();

    /**
     * @brief Валиден ли проект
     */
    bool isValid() const;

    /**
     * @brief Тип проекта
     */
    bool isLocal() const;
    bool isRemote() const;
    ProjectType type() const;
    void setType(ProjectType _type);

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
     * @brief Идентификатор проекта
     */
    int id() const;
    void setId(int _id);

    /**
     * @brief Является ли текущий пользователь владельцем проекта
     */
    bool isOwner() const;
    void setOwner(bool _isOwner);

    /**
     * @brief Режим работы с проектом
     */
    DocumentEditingMode editingMode() const;
    void setEditingMode(DocumentEditingMode _mode);
    bool isReadOnly() const;

    /**
     * @brief Список соавторов
     */
    QVector<Domain::ProjectCollaboratorInfo> collaborators() const;
    void setCollaborators(const QVector<Domain::ProjectCollaboratorInfo>& _collaborators);

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
    const Project& projectAt(int _row) const;

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
     * @brief Уведомить клиентов о том, что проект изменился
     */
    void updateProject(const Project& _project);

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

} // namespace ManagementLayer

#pragma once

#include <QAbstractListModel>


namespace BusinessLayer {

class Project;

/**
 * @brief Модель списка проектов
 */
class ProjectsModel : public QAbstractListModel
{
    Q_OBJECT

public:
    explicit ProjectsModel(QObject* _parent = nullptr);
    ~ProjectsModel() override;

    /**
     * @brief Получить проект по заданному индексу
     */
    Project* projectAt(int _row) const;
    Project* projectForIndex(const QModelIndex& _index) const;

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
    void moveProject(Project* _item, Project* _afterSiblingItem, Project* _parentItem);

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
    QModelIndex index(int _row, int _column = 0, const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const override;
    bool moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
                  const QModelIndex& _destinationParent, int _destinationRow) override;


private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

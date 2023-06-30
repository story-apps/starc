#pragma once

#include <QAbstractItemModel>


namespace BusinessLayer {

class ProjectsModelProjectItem;
class ProjectsModelItem;

/**
 * @brief Модель списка проектов
 */
class ProjectsModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit ProjectsModel(QObject* _parent = nullptr);
    ~ProjectsModel() override;

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(ProjectsModelItem* _item, ProjectsModelItem* _parentItem = nullptr);
    void appendItems(const QVector<ProjectsModelItem*>& _items,
                     ProjectsModelItem* _parentItem = nullptr);

    /**
     * @brief Добавить элемент в начало
     */
    void prependItem(ProjectsModelItem* _item, ProjectsModelItem* _parentItem = nullptr);
    void prependItems(const QVector<ProjectsModelItem*>& _items,
                      ProjectsModelItem* _parentItem = nullptr);

    /**
     * @brief Вставить элемент после заданного
     */
    void insertItem(ProjectsModelItem* _item, ProjectsModelItem* _afterSiblingItem);
    void insertItems(const QVector<ProjectsModelItem*>& _items,
                     ProjectsModelItem* _afterSiblingItem);

    /**
     * @brief Извлечь заданный элемент без удаления
     */
    void takeItem(ProjectsModelItem* _item);
    void takeItems(ProjectsModelItem* _fromItem, ProjectsModelItem* _toItem,
                   ProjectsModelItem* _parentItem);

    /**
     * @brief Удалить заданный элемент
     */
    void removeItem(ProjectsModelItem* _item);
    void removeItems(ProjectsModelItem* _fromItem, ProjectsModelItem* _toItem);

    /**
     * @brief Переместить элемент в заданный родитель после заданного элемента
     */
    void moveItem(ProjectsModelItem* _item, ProjectsModelItem* _afterSiblingItem,
                  ProjectsModelItem* _parentItem = nullptr);

    /**
     * @brief Обновить заданный элемент
     */
    void updateItem(ProjectsModelItem* _item);

    /**
     * @brief Пуста ли модель
     */
    bool isEmpty() const;

    /**
     * @brief Переопределяем методы для собственной реализации модели
     */
    QModelIndex index(int _row, int _column = 0, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount(const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    QVariant data(const QModelIndex& _index, int _role = Qt::DisplayRole) const override;
    //
    bool moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
                  const QModelIndex& _destinationParent, int _destinationRow) override;


    /**
     * @brief Получить элемент модели по индексу
     */
    ProjectsModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить индекс заданного элемента
     */
    QModelIndex indexForItem(ProjectsModelItem* _item) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

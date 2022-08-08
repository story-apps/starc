#pragma once

#include "../abstract_model.h"


namespace Domain {
enum class DocumentObjectType;
}

namespace BusinessLayer {

class StructureModelItem;


/**
 * @brief Список ролей дополнительных данных для навигатора
 */
enum class StructureModelDataRole { IsNavigatorAvailable = Qt::UserRole + 1 };


/**
 * @brief Модель структуры документов проекта
 */
class CORE_LIBRARY_EXPORT StructureModel : public AbstractModel
{
    Q_OBJECT

public:
    explicit StructureModel(QObject* _parent = nullptr);
    ~StructureModel() override;

    /**
     * @brief Является ли проект только что созданным
     */
    bool isNewProject() const;

    /**
     * @brief Задать название проекта
     */
    void setProjectName(const QString& _name);

    /**
     * @brief Добавить документ
     */
    QModelIndex addDocument(Domain::DocumentObjectType _type, const QString& _name = {},
                            const QModelIndex& _parent = {}, const QByteArray& _content = {});

    /**
     * @brief Добавить элемент в начало
     */
    void prependItem(StructureModelItem* _item, StructureModelItem* _parentItem = nullptr);

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(StructureModelItem* _item, StructureModelItem* _parentItem = nullptr,
                    const QByteArray& _content = {});

    /**
     * @brief Вставить элемент после родственика
     */
    void insertItem(StructureModelItem* _item, StructureModelItem* _afterSiblingItem,
                    const QByteArray& _content = {});

    /**
     * @brief Перенести элемент в заданного родителя
     */
    void moveItem(StructureModelItem* _item, StructureModelItem* _parentItem);

    /**
     * @brief Удалить элемент
     */
    void removeItem(StructureModelItem* _item);

    /**
     * @brief Обновить элемент
     */
    void updateItem(StructureModelItem* _item);

    /**
     * @brief Реализация древовидной модели
     */
    /** @{ */
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex& _child) const override;
    int columnCount(const QModelIndex& _parent = {}) const override;
    int rowCount(const QModelIndex& _parent = {}) const override;
    Qt::ItemFlags flags(const QModelIndex& _index) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;
    //! Реализация перетаскивания элементов
    bool canDropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column,
                         const QModelIndex& _parent = {}) const override;
    bool dropMimeData(const QMimeData* _data, Qt::DropAction _action, int _row, int _column,
                      const QModelIndex& _parent = {}) override;
    QMimeData* mimeData(const QModelIndexList& _indexes) const override;
    QStringList mimeTypes() const override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;
    /** @} */

    /**
     * @brief Получить индекс заданного элемента
     */
    QModelIndex indexForItem(StructureModelItem* _item) const;

    /**
     * @brief Получить элемент находящийся в заданном индексе
     */
    StructureModelItem* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить элемент имеющий заданный юид
     */
    StructureModelItem* itemForUuid(const QUuid& _uuid) const;

    /**
     * @brief Получить элемент имеющий заданный тип
     */
    StructureModelItem* itemForType(Domain::DocumentObjectType _type) const;

    /**
     * @brief Получить элемент корзины
     */
    void moveItemToRecycleBin(StructureModelItem* _item);

    /**
     * @brief Задать имя элемента
     */
    void setItemName(const QModelIndex& _index, const QString& _name);
    void setItemName(StructureModelItem* _item, const QString& _name);

    /**
     * @brief Задать цвет элемента
     */
    void setItemColor(StructureModelItem* _item, const QColor& _color);

    /**
     * @brief Задать видимость элемента
     */
    void setItemVisible(StructureModelItem* _item, bool _visible);

    /**
     * @brief Работа со списком версий элемента
     */
    void addItemVersion(StructureModelItem* _item, const QString& _name, const QColor& _color,
                        bool _readOnly, const QByteArray& _content);
    void updateItemVersion(StructureModelItem* _item, int _versionIndex, const QString& _name,
                           const QColor& _color, bool _readOnly);
    void removeItemVersion(StructureModelItem* _item, int _versionIndex);

    /**
     * @brief Задать возможность перехода в навигатор для заданного индекса
     */
    void setNavigatorAvailableFor(const QModelIndex& _index, bool isAvailable);

signals:
    /**
     * @brief Был добавлен документ с заданным идентификатором, типом и содержимым
     */
    void documentAdded(const QUuid& _uuid, const QUuid& _parentUuid,
                       Domain::DocumentObjectType _type, const QString& _name,
                       const QByteArray& _content);

protected:
    /**
     * @brief Реализация модели для работы с документами
     */
    /** @{ */
    void initDocument() override;
    void clearDocument() override;
    QByteArray toXml() const override;
    /** @} */

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

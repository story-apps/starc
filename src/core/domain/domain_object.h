#pragma once

#include "identifier.h"

#include <QAbstractItemModel>


namespace Domain
{

/**
 * @brief Базовый класс для объектов извлекаемых из базы данных
 */
class DomainObject
{
public:
    DomainObject() = default;
    explicit DomainObject(const Identifier& _id);
    virtual ~DomainObject() = default;

public:
    Identifier id() const;
    void setId(const Identifier& _id);

    /**
     * @brief Сохранены ли изменения объекта
     */
    bool isChangesStored() const;

    /**
     * @brief Изменения сохранены
     */
    void markChangesStored();

    /**
     * @brief Изменения не сохранены
     */
    void markChangesNotStored();

private:
    /**
     * @brief Идентификатор объекта
     */
    Identifier m_id;

    /**
     * @brief Флаг изменений объекта
     */
    bool m_isChangesStored = false;
};

// ****

/**
 * @brief Базовый класс для списков объектов извлечённых из базы данных
 */
class DomainObjectsItemModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit DomainObjectsItemModel(QObject* _parent = nullptr);

public:
    /**
     * @brief Базовая реализация методов модели
     */
    /** @{ **/
    QModelIndex index(int _row, int _column, const QModelIndex& _parent = {}) const override;
    QModelIndex parent(const QModelIndex &_index) const override;
    int rowCount(const QModelIndex& _index = {}) const override;
    int columnCount(const QModelIndex& _index) const override;
    QVariant data(const QModelIndex& _index, int _role) const override;
    bool moveRows(const QModelIndex& _sourceParent, int _sourceRow, int _count,
        const QModelIndex& _destinationParent, int _destinationRow) override;
    /** @} **/

    /**
     * @brief Получить доменный объект по индексу
     */
    virtual DomainObject* itemForIndex(const QModelIndex& _index) const;

    /**
     * @brief Получить индекс доменного объекта
     */
    virtual QModelIndex indexForItem(DomainObject* _object) const;

    /**
     * @brief Пуст ли список
     */
    bool isEmpty() const;

    /**
     * @brief Синоним для rowCount
     */
    int size() const;

    /**
     * @brief Содержится ли объект в списке
     */
    bool contains(DomainObject* _object) const;

    /**
     * @brief Добавить объект список
     */
    void append(DomainObject* _object);

    /**
     * @brief Удалить объект из списка
     */
    void remove(DomainObject* _object);

    /**
     * @brief Очистить таблицу и если \p _removeItems равен true, то удалить все элементы
     *        (необходимо для случаев, когда в разных списках могут храниться одни объекты)
     */
    void clear(bool _needRemoveItems = true);

    QVector<DomainObject*>::iterator begin();
    QVector<DomainObject*>::const_iterator begin() const;
    QVector<DomainObject*>::iterator end();
    QVector<DomainObject*>::const_iterator end() const;

protected:
    /**
     * @brief Интерфейс для доступа к списку сохранённых элементов для классов наследников
     */
    const QVector<DomainObject*>& domainObjects() const;

private:
    /**
     * @brief Список сохранённых объектов
     */
    QVector<DomainObject*> m_domainObjects;
};

// ****

/**
 * @brief Интерфейс класса для загрузки/сохранения изображений в БД
 */
class AbstractImageWrapper {
public:
    virtual ~AbstractImageWrapper() = default;

    /**
     * @brief Получить изображение для заданного объекта
     */
    virtual QPixmap image(const DomainObject* _forObject) const = 0;

    /**
     * @brief Установить изображение для заданного объекта
     */
    virtual void setImage(const QPixmap& _image, const DomainObject* _forObject) = 0;
};

} // namespace Domain

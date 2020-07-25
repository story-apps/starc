#pragma once

#include <corelib_global.h>

#include <QScopedPointer>

class QVariant;


namespace BusinessLayer
{

/**
 * @brief Абстрактный класс элемента древовидной модели
 */
class CORE_LIBRARY_EXPORT AbstractModelItem
{
public:
    AbstractModelItem();
    virtual ~AbstractModelItem();

    /**
     * @brief Интерфейс для получения данных элемента по роли
     */
    virtual QVariant data(int _role) const = 0;


    /**
     * @brief Добавить элемент в начало
     */
    void prependItem(AbstractModelItem* _item);

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(AbstractModelItem* _item);

    /**
     * @brief Вставить элемент в указанное место
     */
    void insertItem(int _index, AbstractModelItem* _item);

    /**
     * @brief Удалить элемент
     */
    void removeItem(AbstractModelItem* _item);

    /**
     * @brief Извлечь элемент не удаляя его
     */
    void takeItem(AbstractModelItem* _item);

    /**
     * @brief Имеет ли элемент родительский элемент
     */
    bool hasParent() const;

    /**
     * @brief Родительский элемент
     */
    virtual AbstractModelItem* parent() const;

    /**
     * @brief Имеет ли элемент детей
     */
    bool hasChildren() const;

    /**
     * @brief Количество дочерних элементов
     */
    int childCount() const;

    /**
     * @brief Является ли заданный элемент дочерним текущему
     */
    bool hasChild(AbstractModelItem* _child) const;

    /**
     * @brief Индекс дочернего элемента
     */
    int rowOfChild(AbstractModelItem* _child) const;

    /**
     * @brief Дочерний элемент по индексу
     */
    virtual AbstractModelItem* childAt(int _index) const;

    /**
     * @brief Изменён ли элемент
     */
    bool isChanged() const;
    void setChanged(bool _changed);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

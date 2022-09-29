#pragma once

#include <QScopedPointer>

#include <corelib_global.h>

class QVariant;


namespace BusinessLayer {

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
    void prependItems(const QVector<AbstractModelItem*>& _items);

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(AbstractModelItem* _item);
    void appendItems(const QVector<AbstractModelItem*>& _items);

    /**
     * @brief Вставить элемент в указанное место
     */
    void insertItem(int _index, AbstractModelItem* _item);
    void insertItems(int _index, const QVector<AbstractModelItem*>& _items);

    /**
     * @brief Удалить элемент
     */
    void removeItem(AbstractModelItem* _item);
    void removeItems(int _fromIndex, int _toIndex);

    /**
     * @brief Извлечь элемент не удаляя его
     */
    void takeItem(AbstractModelItem* _item);
    void takeItems(int _fromIndex, int _toIndex);

    /**
     * @brief Имеет ли элемент родительский элемент
     */
    bool hasParent() const;

    /**
     * @brief Родительский элемент
     */
    virtual AbstractModelItem* parent() const;

    /**
     * @brief Установить родительский элемент
     */
    void setParent(AbstractModelItem* _parent);

    /**
     * @brief Является ли элемент ребёнком заданного (рекурсивная проверка)
     */
    bool isChildOf(AbstractModelItem* _parent) const;

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
    bool hasChild(AbstractModelItem* _child, bool _recursively = false) const;

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

protected:
    /**
     * @brief Возможность обработки изменния для дочерних классов
     */
    virtual void handleChange()
    {
        //
        // TODO: добавить возможность указать причину изменения, чтобы наследники могли более гибко
        // реагировать на него, например нет необходимости пересчитывать хронометраж, если не
        // изменился текст или тип параграфа
        //
    }

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

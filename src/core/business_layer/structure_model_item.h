#pragma once

#include <QScopedPointer>

class QColor;
class QUuid;

namespace Domain {
    enum class DocumentObjectType;
}


namespace BusinessLayer
{

/**
 * @brief Элемент структуры проекта
 */
class StructureModelItem
{
public:
    explicit StructureModelItem(const QUuid& _uuid, Domain::DocumentObjectType _type,
         const QString& _name, const QColor& _color);
    explicit StructureModelItem(const StructureModelItem& _other);

    /**
     * @brief Уникальный идентификатор элемента
     */
    QUuid uuid() const;

    /**
     * @brief Иконка элемента
     */
    QString icon() const;

    /**
     * @brief Название элемента
     */
    QString name() const;

    /**
     * @brief Цвет элемента
     */
    QColor color() const;


    //
    // Вспомогательные методы для организации работы модели
    //

    /**
     * @brief Добавить элемент в начало
     */
    void prependItem(StructureModelItem* _item);

    /**
     * @brief Добавить элемент в конец
     */
    void appendItem(StructureModelItem* _item);

    /**
     * @brief Вставить элемент в указанное место
     */
    void insertItem(int _index, StructureModelItem* _item);

    /**
     * @brief Удалить элемент
     */
    void removeItem(StructureModelItem* _item);

    /**
     * @brief Извлечь элемент не удаляя его
     */
    void takeItem(StructureModelItem* _item);

    /**
     * @brief Имеет ли элемент родительский элемент
     */
    bool hasParent() const;

    /**
     * @brief Родительский элемент
     */
    StructureModelItem* parent() const;

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
    bool hasChild(StructureModelItem* _child) const;

    /**
     * @brief Индекс дочернего элемента
     */
    int rowOfChild(StructureModelItem* _child) const;

    /**
     * @brief Дочерний элемент по индексу
     */
    StructureModelItem* childAt(int _index) const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

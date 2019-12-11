#pragma once

#include "item_object.h"


namespace Domain
{

/**
 * @brief Фабрика для создания элементов данных
 */
class ItemsBuilder
{
public:
    /**
     * @brief Создать элемент
     */
    static ItemObject* create(const Identifier& _id, ItemObject* _parent, ItemObjectType _type,
        const QString& _name = {}, const QString& _description = {}, const QColor& _color = {},
        int _sortOrder = 0, bool _isDeleted = false, AbstractImageWrapper* _imageWrapper = nullptr);
};

} // namespace Domain

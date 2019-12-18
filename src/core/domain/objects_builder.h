#pragma once

#include "document_object.h"


namespace Domain
{

/**
 * @brief Фабрика для создания элементов данных
 */
class ObjectsBuilder
{
public:
    /**
     * @brief Создать элемент
     */
    static DocumentObject* create(const Identifier& _id, const QUuid& _uuid, DocumentObjectType _type,
                                  const QByteArray& _content = {});
};

} // namespace Domain

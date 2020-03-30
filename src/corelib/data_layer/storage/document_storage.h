#pragma once

#include <corelib_global.h>

class QByteArray;
class QUuid;

template <typename T>
class QVector;

namespace Domain {
    class DocumentObject;
    enum class DocumentObjectType;
}


namespace DataStorageLayer
{

/**
 * @brief Хранилище документов
 */
class CORE_LIBRARY_EXPORT DocumentStorage
{
public:
    /**
     * @brief Получить документ по uuid'у
     */
    Domain::DocumentObject* document(const QUuid& _uuid);

    /**
     * @brief Получить документ по типу
     * @note Создаёт документ, если такого ещё нет
     */
    Domain::DocumentObject* document(Domain::DocumentObjectType _type);

    /**
     * @brief Получить список докуметов заданного типа
     */
    QVector<Domain::DocumentObject*> documents(Domain::DocumentObjectType _type);

    /**
     * @brief Сохранить документ
     */
    Domain::DocumentObject* storeDocument(const QUuid& _uuid, Domain::DocumentObjectType _type);

    /**
     * @brief Обновить документ
     */
    void updateDocument(Domain::DocumentObject* _document);

    /**
     * @brief Удалить документ
     */
    void removeDocument(Domain::DocumentObject* _document);

    /**
     * @brief Очистить хранилище
     */
    void clear();

private:
    DocumentStorage() = default;
    friend class StorageFacade;
};

} // namespace DataStorageLayer

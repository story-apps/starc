#pragma once

class QByteArray;
class QUuid;

namespace Domain {
    class DocumentObject;
    enum class DocumentObjectType;
}


namespace DataStorageLayer
{

/**
 * @brief Хранилище документов
 */
class DocumentStorage
{
public:
    /**
     * @brief Получить документ структуры проекта
     */
    Domain::DocumentObject* structure();

    /**
     * @brief Получить документ по uuid'у
     */
    Domain::DocumentObject* document(const QUuid& _uuid);

    /**
     * @brief Сохранить документ
     */
    Domain::DocumentObject* storeDocument(const QUuid& _uuid, Domain::DocumentObjectType _type,
        const QByteArray& _content);

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

#pragma once

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
class DocumentsStorage
{
public:
    /**
     * @brief Получить документ по uuid'у
     */
    Domain::DocumentObject* document(const QUuid& _uuid);

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
    DocumentsStorage() = default;
    friend class StorageFacade;
};

} // namespace DataStorageLayer

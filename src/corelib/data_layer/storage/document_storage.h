#pragma once

#include <QScopedPointer>
#include <QtContainerFwd>

#include <corelib_global.h>

class QByteArray;
class QUuid;

namespace Domain {
class DocumentObject;
enum class DocumentObjectType;
} // namespace Domain


namespace DataStorageLayer {

/**
 * @brief Хранилище документов
 */
class CORE_LIBRARY_EXPORT DocumentStorage
{
public:
    ~DocumentStorage();

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
    Domain::DocumentObject* createDocument(const QUuid& _uuid, Domain::DocumentObjectType _type);

    /**
     * @brief Изменить гуид документа
     */
    void updateDocumentUuid(const QUuid& _old, const QUuid& _new);

    /**
     * @brief Обновить документ
     */
    void saveDocument(Domain::DocumentObject* _document);

    /**
     * @brief Удалить документ
     */
    void removeDocument(Domain::DocumentObject* _document);

    /**
     * @brief Очистить хранилище
     */
    void clear();

private:
    DocumentStorage();
    friend class StorageFacade;

    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer

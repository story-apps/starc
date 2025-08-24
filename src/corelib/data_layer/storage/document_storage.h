#pragma once

#include <QObject>
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
class CORE_LIBRARY_EXPORT DocumentStorage : public QObject
{
    Q_OBJECT

public:
    ~DocumentStorage();

    /**
     * @brief Получить документ по uuid'у синхронно
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
     * @brief Получить все документы проекта
     */
    QVector<Domain::DocumentObject*> documents();

    /**
     * @brief Сохранить документ
     */
    Domain::DocumentObject* createDocument(const QUuid& _uuid, Domain::DocumentObjectType _type);

    /**
     * @brief Изменить uuid документа
     */
    void updateDocumentUuid(const QUuid& _old, const QUuid& _new);

    /**
     * @brief Обновить документ синхронно
     */
    bool saveDocument(Domain::DocumentObject* _document);
    bool saveDocument(const QUuid& _documentUuid);

    /**
     * @brief Удалить документ
     */
    bool removeDocument(Domain::DocumentObject* _document);

    /**
     * @brief Получить документы по uuid'у асинхронно
     * @return Uuid запроса
     */
    QUuid loadDocumentsAsync(const QVector<QUuid>& _documentUuids);
    Q_SIGNAL void documentsLoaded(const QUuid& _queryUuid,
                                  QVector<Domain::DocumentObject*> _documents);

    /**
     * @brief Обновить документ асинхронно
     * @return Uuid запроса
     */
    QUuid saveDocumentAsync(Domain::DocumentObject* _document);
    QUuid saveDocumentAsync(const QUuid& _documentUuid);

    /**
     * @brief Удалить документ асинхронно
     * @return Гуид запроса
     */
    QUuid removeDocumentAsync(Domain::DocumentObject* _document);

    /**
     * @brief Очистить хранилище
     */
    void clear();

private:
    DocumentStorage(QObject* _parent = nullptr);
    friend class StorageFacade;

    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer

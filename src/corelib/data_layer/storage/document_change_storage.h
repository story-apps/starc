#pragma once

#include <QScopedPointer>

#include <corelib_global.h>

class QDateTime;
class QUuid;

namespace Domain {
class DocumentChangeObject;
}


namespace DataStorageLayer {

/**
 * @brief Хранилище изменений документов
 */
class CORE_LIBRARY_EXPORT DocumentChangeStorage
{
public:
    ~DocumentChangeStorage();

    /**
     * @brief Сохранить документ
     */
    Domain::DocumentChangeObject* appendDocumentChange(
        const QUuid& _documentUuid, const QUuid& _uuid, const QByteArray& _undoPatch,
        const QByteArray& _redoPatch, const QString& _userEmail, const QString& _userName);

    /**
     * @brief Пуст ли список изменений документов
     */
    bool isEmpty() const;

    /**
     * @brief Обновить параметры изменения
     */
    void updateDocumentChange(Domain::DocumentChangeObject* _change);

    /**
     * @brief Удалить изменение
     */
    void removeDocumentChange(Domain::DocumentChangeObject* _change);

    /**
     * @brief Получить изменение документа с заданным индексом
     */
    Domain::DocumentChangeObject* documentChangeAt(const QUuid& _documentUuid, int _changeIndex);

    /**
     * @brief Список не синхронизированных документов
     */
    QVector<QUuid> unsyncedDocuments();

    /**
     * @brief Изменения документа, которые ещё не были синхронизированы
     */
    QVector<Domain::DocumentChangeObject*> unsyncedDocumentChanges(const QUuid& _documentUuid);

    /**
     * @brief Сохранить несохранённые изменения сценариев
     */
    void store();

    /**
     * @brief Сохранить несохранённые изменения сценариев асинхронно
     */
    void storeAsync();

    /**
     * @brief Удалить все изменения
     */
    void removeAll();

    /**
     * @brief Очистить хранилище
     */
    void clear();

private:
    DocumentChangeStorage();
    friend class StorageFacade;

    class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace DataStorageLayer

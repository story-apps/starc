#include "document_raw_data_storage.h"

#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QUuid>

namespace DataStorageLayer {

class DocumentRawDataStorage::Implementation
{
public:
    explicit Implementation(DocumentRawDataStorage* _q);

    /**
     * @brief Запросить изображение
     */
    void notifyRawDataRequested(const QUuid& _uuid) const;


    DocumentRawDataStorage* q = nullptr;

    /**
     * @brief Список новых документов
     */
    QHash<QUuid, QByteArray> newDocuments;

    /**
     * @brief Список документов на удаление
     */
    QList<QUuid> documentsToRemove;
};

DocumentRawDataStorage::Implementation::Implementation(DocumentRawDataStorage* _q)
    : q(_q)
{
}

void DocumentRawDataStorage::Implementation::notifyRawDataRequested(const QUuid& _uuid) const
{
    QMetaObject::invokeMethod(
        q, [this, _uuid] { emit q->rawDataRequested(_uuid); }, Qt::QueuedConnection);
}


// ****


DocumentRawDataStorage::DocumentRawDataStorage(QObject* _parent)
    : AbstractRawDataWrapper(_parent)
    , d(new Implementation(this))
{
}

DocumentRawDataStorage::~DocumentRawDataStorage() = default;

QByteArray DocumentRawDataStorage::load(const QUuid& _uuid) const
{
    if (_uuid.isNull()) {
        return {};
    }

    if (auto documentIter = d->newDocuments.find(_uuid); documentIter != d->newDocuments.end()) {
        return documentIter.value();
    }

    //
    // Загружаем документ
    //
    auto rawDataDocument = StorageFacade::documentStorage()->document(_uuid);
    //
    // ... подписываемся на его обновления (и загружаем, если не был ещё загружен из облака)
    //
    d->notifyRawDataRequested(_uuid);
    //
    // ... если изображения пока нет в базе, то прерываем выполнение
    //
    if (rawDataDocument == nullptr) {
        return {};
    }

    Q_ASSERT(rawDataDocument->type() == Domain::DocumentObjectType::BinaryData);

    return rawDataDocument->content();
}

QUuid DocumentRawDataStorage::save(const QByteArray& _data)
{
    if (_data.isEmpty()) {
        return {};
    }

    //
    // Сохраним данные во временный буфер
    //
    const QUuid uuid = QUuid::createUuid();
    d->newDocuments.insert(uuid, _data);
    //
    // ... положим в хранилище
    //
    auto document = StorageFacade::documentStorage()->createDocument(
        uuid, Domain::DocumentObjectType::BinaryData);
    document->setContent(_data);
    //
    // ... уведомляем о добавленных данных
    //
    emit rawDataAdded(uuid);

    return uuid;
}

void DocumentRawDataStorage::save(const QUuid& _uuid, const QByteArray& _data)
{
    if (_uuid.isNull()) {
        return;
    }

    //
    // Сохраним изображение во временный буфер
    //
    d->newDocuments.insert(_uuid, _data);
    //
    // ... положим в хранилище, если ещё не был сохранён
    //
    auto document = StorageFacade::documentStorage()->document(_uuid);
    if (document == nullptr) {
        document = StorageFacade::documentStorage()->createDocument(
            _uuid, Domain::DocumentObjectType::BinaryData);
    }
    //
    // ... а если был, проверим, нужно ли его обновлять
    //
    else if (document->content() == _data) {
        return;
    }
    document->setContent(_data);
    //
    // ... уведомляем об обновлении изображения
    //
    emit rawDataUpdated(_uuid, _data);
}

void DocumentRawDataStorage::remove(const QUuid& _uuid)
{
    //
    // Если документ новый, просто удаляем его из списка новых, иначе добавляем в список на
    // удаление из БД
    //
    if (!d->newDocuments.remove(_uuid)) {
        d->documentsToRemove.append(_uuid);
    }
    emit rawDataRemoved(_uuid);
}

void DocumentRawDataStorage::clear()
{
    d->newDocuments.clear();
    d->documentsToRemove.clear();
}

void DocumentRawDataStorage::saveChanges()
{
    for (auto documentIter = d->newDocuments.begin(); documentIter != d->newDocuments.end();
         ++documentIter) {
        const auto isSaved = StorageFacade::documentStorage()->saveDocument(documentIter.key());
        if (!isSaved) {
            return;
        }
    }
    d->newDocuments.clear();

    while (!d->documentsToRemove.isEmpty()) {
        const auto isRemoved = StorageFacade::documentStorage()->removeDocument(
            StorageFacade::documentStorage()->document(d->documentsToRemove.takeFirst()));
        if (!isRemoved) {
            return;
        }
    }
}

void DocumentRawDataStorage::saveChangesAsync()
{
    for (auto documentIter = d->newDocuments.begin(); documentIter != d->newDocuments.end();
         ++documentIter) {
        StorageFacade::documentStorage()->saveDocumentAsync(documentIter.key());
    }
    d->newDocuments.clear();

    while (!d->documentsToRemove.isEmpty()) {
        StorageFacade::documentStorage()->removeDocumentAsync(
            StorageFacade::documentStorage()->document(d->documentsToRemove.takeFirst()));
    }
}

} // namespace DataStorageLayer

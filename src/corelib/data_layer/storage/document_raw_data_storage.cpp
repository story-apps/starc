#include "document_raw_data_storage.h"

#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <domain/document_object.h>

#include <QUuid>

namespace DataStorageLayer {

class DocumentRawDataStorage::Implementation
{
public:
    /**
     * @brief Список новых документов
     */
    QHash<QUuid, QByteArray> newDocumetns;

    /**
     * @brief Список документов на удаление
     */
    QList<QUuid> documentsToRemove;
};


// ****


DocumentRawDataStorage::DocumentRawDataStorage(QObject* _parent)
    : AbstractRawDataWrapper(_parent)
    , d(new Implementation)
{
}

DocumentRawDataStorage::~DocumentRawDataStorage() = default;

QByteArray DocumentRawDataStorage::load(const QUuid& _uuid) const
{
    if (_uuid.isNull()) {
        return {};
    }

    if (auto documentIter = d->newDocumetns.find(_uuid); documentIter != d->newDocumetns.end()) {
        return documentIter.value();
    }

    //
    // Загружаем документ
    //
    auto rawDataDocument = StorageFacade::documentStorage()->document(_uuid);
    Q_ASSERT(rawDataDocument->type() == Domain::DocumentObjectType::BinaryData);

    if (rawDataDocument == nullptr) {
        return {};
    }

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
    d->newDocumetns.insert(uuid, _data);
    //
    // ... положим в хранилище
    //
    auto document = StorageFacade::documentStorage()->createDocument(
        uuid, Domain::DocumentObjectType::BinaryData);
    document->setContent(_data);

    return uuid;
}

void DocumentRawDataStorage::remove(const QUuid& _uuid)
{
    //
    // Если документ новый, просто удаляем его из списка новых, иначе добавляем в список на
    // удаление из БД
    //
    if (!d->newDocumetns.remove(_uuid)) {
        d->documentsToRemove.append(_uuid);
    }
}

void DocumentRawDataStorage::clear()
{
    d->newDocumetns.clear();
    d->documentsToRemove.clear();
}

void DocumentRawDataStorage::saveChanges()
{
    for (auto documentIter = d->newDocumetns.begin(); documentIter != d->newDocumetns.end();
         ++documentIter) {
        StorageFacade::documentStorage()->saveDocument(documentIter.key());
    }
    d->newDocumetns.clear();

    while (!d->documentsToRemove.isEmpty()) {
        StorageFacade::documentStorage()->removeDocument(
            StorageFacade::documentStorage()->document(d->documentsToRemove.takeFirst()));
    }
}

} // namespace DataStorageLayer

#include "document_change_storage.h"

#include <data_layer/database.h>
#include <data_layer/mapper/document_change_mapper.h>
#include <data_layer/mapper/mapper_facade.h>
#include <domain/document_change_object.h>
#include <domain/objects_builder.h>
#include <utils/shugar.h>


namespace DataStorageLayer {

class DocumentChangeStorage::Implementation
{
public:
    QVector<Domain::DocumentChangeObject*> newDocumentChanges;
};


// ****


DocumentChangeStorage::~DocumentChangeStorage() = default;

Domain::DocumentChangeObject* DocumentChangeStorage::appendDocumentChange(
    const QUuid& _documentUuid, const QUuid& _uuid, const QByteArray& _undoPatch,
    const QByteArray& _redoPatch, const QString& _userEmail, const QString& _userName)
{
    const auto isSynced = false;
    auto newDocumentChange = Domain::ObjectsBuilder::createDocumentChange(
        {}, _documentUuid, _uuid, _undoPatch, _redoPatch, QDateTime::currentDateTimeUtc(),
        _userEmail, _userName, isSynced);
    d->newDocumentChanges.append(newDocumentChange);
    return newDocumentChange;
}

bool DocumentChangeStorage::isEmpty() const
{
    return DataMappingLayer::MapperFacade::documentChangeMapper()->isEmpty();
}

void DocumentChangeStorage::updateDocumentChange(Domain::DocumentChangeObject* _change)
{
    DataMappingLayer::MapperFacade::documentChangeMapper()->update(_change);
}

void DocumentChangeStorage::removeDocumentChange(Domain::DocumentChangeObject* _change)
{
    if (const auto changeIndex = d->newDocumentChanges.indexOf(_change); changeIndex != -1) {
        delete d->newDocumentChanges[changeIndex];
        d->newDocumentChanges.removeAt(changeIndex);
    } else {
        DataMappingLayer::MapperFacade::documentChangeMapper()->remove(_change);
    }
}

Domain::DocumentChangeObject* DocumentChangeStorage::documentChangeAt(const QUuid& _documentUuid,
                                                                      int _changeIndex)
{
    auto correctedChangeIndex = _changeIndex;

    //
    // Изменения могут находиться и в списке недавних
    //
    for (const auto change : reversed(d->newDocumentChanges)) {
        if (change->documentUuid() != _documentUuid) {
            continue;
        }

        if (correctedChangeIndex == 0) {
            return change;
        } else {
            --correctedChangeIndex;
        }
    }

    return DataMappingLayer::MapperFacade::documentChangeMapper()->find(_documentUuid,
                                                                        correctedChangeIndex);
}

QVector<QUuid> DocumentChangeStorage::unsyncedDocuments()
{
    QVector<QUuid> unsyncedDocuments;
    for (const auto change : std::as_const(d->newDocumentChanges)) {
        if (!unsyncedDocuments.contains(change->documentUuid())) {
            unsyncedDocuments.append(change->documentUuid());
        }
    }

    unsyncedDocuments.append(
        DataMappingLayer::MapperFacade::documentChangeMapper()->unsyncedDocuments());
    return unsyncedDocuments;
}

QVector<Domain::DocumentChangeObject*> DocumentChangeStorage::unsyncedDocumentChanges(
    const QUuid& _documentUuid)
{
    QVector<Domain::DocumentChangeObject*> changes;
    for (const auto change : std::as_const(d->newDocumentChanges)) {
        if (change->documentUuid() == _documentUuid && !change->isSynced()) {
            changes.append(change);
        }
    }

    changes.append(
        DataMappingLayer::MapperFacade::documentChangeMapper()->findAllUnsynced(_documentUuid));

    //
    // Изменения должны следовать по дате создания, т.к. наложение последующих зависит от предыдущих
    //
    std::sort(
        changes.begin(), changes.end(),
        [](const Domain::DocumentChangeObject* _lhs, const Domain::DocumentChangeObject* _rhs) {
            return _lhs->dateTime() < _rhs->dateTime();
        });

    return changes;
}

void DocumentChangeStorage::store()
{
    DatabaseLayer::Database::transaction();
    while (!d->newDocumentChanges.isEmpty()) {
        const auto change = d->newDocumentChanges.first();
        const auto isInserted
            = DataMappingLayer::MapperFacade::documentChangeMapper()->insert(change);
        if (!isInserted) {
            DatabaseLayer::Database::rollback();
            return;
        }

        d->newDocumentChanges.removeFirst();
    }
    DatabaseLayer::Database::commit();
}

void DocumentChangeStorage::removeAll()
{
    qDeleteAll(d->newDocumentChanges);
    d->newDocumentChanges.clear();
    DataMappingLayer::MapperFacade::documentChangeMapper()->removeAll();
    DatabaseLayer::Database::vacuum();
}

void DocumentChangeStorage::clear()
{
    qDeleteAll(d->newDocumentChanges);
    d->newDocumentChanges.clear();
    DataMappingLayer::MapperFacade::documentChangeMapper()->clear();
}

DocumentChangeStorage::DocumentChangeStorage()
    : d(new Implementation)
{
}

} // namespace DataStorageLayer

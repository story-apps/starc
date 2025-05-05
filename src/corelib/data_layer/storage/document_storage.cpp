#include "document_storage.h"

#include <data_layer/mapper/document_mapper.h>
#include <data_layer/mapper/mapper_facade.h>
#include <domain/document_object.h>
#include <domain/objects_builder.h>


namespace DataStorageLayer {

class DocumentStorage::Implementation
{
public:
    explicit Implementation(DocumentStorage* _q);

    /**
     * @brief Обновить документ
     */
    void saveDocument(Domain::DocumentObject* _document, bool doAsync);
    void saveDocument(const QUuid& _documentUuid, bool doAsync);


    DocumentStorage* q = nullptr;

    /**
     * @brief Созданные, но не сохранённые документы
     */
    QHash<QUuid, Domain::DocumentObject*> notSavedDocuments;
};

DocumentStorage::Implementation::Implementation(DocumentStorage* _q)
    : q(_q)
{
}

void DocumentStorage::Implementation::saveDocument(Domain::DocumentObject* _document, bool doAsync)
{
    if (notSavedDocuments.contains(_document->uuid())) {
        if (doAsync) {
            DataMappingLayer::MapperFacade::documentMapper()->insertAsync(_document);
        } else {
            DataMappingLayer::MapperFacade::documentMapper()->insert(_document);
        }
        notSavedDocuments.remove(_document->uuid());
    } else {
        DataMappingLayer::MapperFacade::documentMapper()->update(_document);
    }
}

void DocumentStorage::Implementation::saveDocument(const QUuid& _documentUuid, bool doAsync)
{
    auto documentToSave = q->document(_documentUuid);
    if (documentToSave == nullptr) {
        return;
    }

    saveDocument(documentToSave, doAsync);
}


// ****


DocumentStorage::~DocumentStorage() = default;

Domain::DocumentObject* DocumentStorage::document(const QUuid& _uuid)
{
    if (d->notSavedDocuments.contains(_uuid)) {
        return d->notSavedDocuments.value(_uuid);
    }

    return DataMappingLayer::MapperFacade::documentMapper()->find(_uuid);
}

Domain::DocumentObject* DocumentStorage::document(Domain::DocumentObjectType _type)
{
    //
    // Сперва ищем документ с заданным типом среди несохранённых
    //
    for (auto document : std::as_const(d->notSavedDocuments)) {
        if (document->type() == _type) {
            return document;
        }
    }

    //
    // Если не нашлось, то среди сохранённых
    //
    auto document = DataMappingLayer::MapperFacade::documentMapper()->findFirst(_type);

    //
    // Если и там не нашлось, то создаём новый документ с указанным типом
    //
    if (document == nullptr) {
        document = createDocument(QUuid::createUuid(), _type);
    }
    return document;
}

QVector<Domain::DocumentObject*> DocumentStorage::documents(Domain::DocumentObjectType _type)
{
    QVector<Domain::DocumentObject*> documents;

    for (auto document : std::as_const(d->notSavedDocuments)) {
        if (document->type() == _type) {
            documents.append(document);
        }
    }

    documents.append(DataMappingLayer::MapperFacade::documentMapper()->findAll(_type));

    return documents;
}

QVector<Domain::DocumentObject*> DocumentStorage::documents()
{
    QVector<Domain::DocumentObject*> documents;

    for (auto document : std::as_const(d->notSavedDocuments)) {
        documents.append(document);
    }

    documents.append(DataMappingLayer::MapperFacade::documentMapper()->findAll());

    return documents;
}

void DocumentStorage::loadDocumentsAsync(const QUuid& _queryUuid,
                                         const QVector<QUuid>& _documentUuids)
{
    DataMappingLayer::MapperFacade::documentMapper()->findAsync(_queryUuid, _documentUuids);
}

Domain::DocumentObject* DocumentStorage::createDocument(const QUuid& _uuid,
                                                        Domain::DocumentObjectType _type)
{
    auto newDocument = Domain::ObjectsBuilder::createDocument({}, _uuid, _type, {}, {});
    d->notSavedDocuments.insert(_uuid, newDocument);
    return newDocument;
}

void DocumentStorage::updateDocumentUuid(const QUuid& _old, const QUuid& _new)
{
    auto document = this->document(_old);
    if (document == nullptr || document->uuid() == _new) {
        return;
    }

    if (d->notSavedDocuments.contains(_old)) {
        d->notSavedDocuments.remove(_old);
        d->notSavedDocuments.insert(_new, document);
    }
    document->setUuid(_new);
}

void DocumentStorage::saveDocument(Domain::DocumentObject* _document)
{
    const bool doAsync = false;
    d->saveDocument(_document, doAsync);
}

void DocumentStorage::saveDocument(const QUuid& _documentUuid)
{
    const bool doAsync = false;
    d->saveDocument(_documentUuid, doAsync);
}

void DocumentStorage::saveDocumentAsync(Domain::DocumentObject* _document)
{
    const bool doAsync = true;
    d->saveDocument(_document, doAsync);
}

void DocumentStorage::saveDocumentAsync(const QUuid& _documentUuid)
{
    const bool doAsync = true;
    d->saveDocument(_documentUuid, doAsync);
}

void DocumentStorage::removeDocument(Domain::DocumentObject* _document)
{
    if (_document == nullptr) {
        return;
    }

    if (d->notSavedDocuments.contains(_document->uuid())) {
        d->notSavedDocuments.remove(_document->uuid());
        delete _document;
        _document = nullptr;
    } else {
        DataMappingLayer::MapperFacade::documentMapper()->remove(_document);
    }
}

void DocumentStorage::clear()
{
    qDeleteAll(d->notSavedDocuments);
    d->notSavedDocuments.clear();
    DataMappingLayer::MapperFacade::documentMapper()->clear();
}

DocumentStorage::DocumentStorage(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
    connect(DataMappingLayer::MapperFacade::documentMapper(),
            &DataMappingLayer::AbstractMapper::objectsFound, this,
            [this](const QUuid& _queryUuid, const QVector<Domain::DomainObject*>& _objects) {
                QVector<Domain::DocumentObject*> documents;
                for (const auto& object : _objects) {
                    const auto document = static_cast<Domain::DocumentObject*>(object);
                    documents.append(document);
                }
                emit documentsLoaded(_queryUuid, documents);
            });
}

} // namespace DataStorageLayer

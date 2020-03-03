#include "document_storage.h"

#include <data_layer/mapper/document_mapper.h>
#include <data_layer/mapper/mapper_facade.h>

#include <domain/document_object.h>
#include <domain/objects_builder.h>


namespace DataStorageLayer
{

Domain::DocumentObject* DocumentStorage::document(const QUuid& _uuid)
{
    return DataMappingLayer::MapperFacade::documentMapper()->find(_uuid);
}

Domain::DocumentObject* DocumentStorage::document(Domain::DocumentObjectType _type)
{
    auto document = DataMappingLayer::MapperFacade::documentMapper()->findFirst(_type);
    if (document == nullptr) {
        document = storeDocument(QUuid::createUuid(), _type);
    }
    return document;
}

QVector<Domain::DocumentObject*> DocumentStorage::documents(Domain::DocumentObjectType _type)
{
    return DataMappingLayer::MapperFacade::documentMapper()->findAll(_type);
}

Domain::DocumentObject* DocumentStorage::storeDocument(const QUuid& _uuid, Domain::DocumentObjectType _type)
{
    auto newDocument = Domain::ObjectsBuilder::createDocument({}, _uuid, _type, {});
    DataMappingLayer::MapperFacade::documentMapper()->insert(newDocument);
    return newDocument;
}

void DocumentStorage::updateDocument(Domain::DocumentObject* _document)
{
    DataMappingLayer::MapperFacade::documentMapper()->update(_document);
}

void DocumentStorage::removeDocument(Domain::DocumentObject* _document)
{
    DataMappingLayer::MapperFacade::documentMapper()->remove(_document);
}

void DocumentStorage::clear()
{
    DataMappingLayer::MapperFacade::documentMapper()->clear();
}

} // namespace DataStorageLayer

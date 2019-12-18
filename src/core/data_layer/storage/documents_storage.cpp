#include "documents_storage.h"

#include <data_layer/mapper/documents_mapper.h>
#include <data_layer/mapper/mapper_facade.h>

#include <domain/document_object.h>
#include <domain/objects_builder.h>


namespace DataStorageLayer
{

Domain::DocumentObject* DocumentsStorage::structure()
{
    auto structure = DataMappingLayer::MapperFacade::documentsMapper()->findStructure();

    //
    // Создаём структуру, если ещё не была создана
    //
    if (structure == nullptr) {
        structure = storeDocument({}, Domain::DocumentObjectType::Structure);
    }

    return structure;
}

Domain::DocumentObject* DocumentsStorage::document(const QUuid& _uuid)
{
    return DataMappingLayer::MapperFacade::documentsMapper()->find(_uuid);
}

Domain::DocumentObject* DocumentsStorage::storeDocument(const QUuid& _uuid, Domain::DocumentObjectType _type)
{
    auto newDocument = Domain::ObjectsBuilder::create({}, _uuid, _type);
    DataMappingLayer::MapperFacade::documentsMapper()->insert(newDocument);
    return newDocument;
}

void DocumentsStorage::updateDocument(Domain::DocumentObject* _document)
{
    DataMappingLayer::MapperFacade::documentsMapper()->update(_document);
}

void DocumentsStorage::removeDocument(Domain::DocumentObject* _document)
{
    DataMappingLayer::MapperFacade::documentsMapper()->remove(_document);
}

void DocumentsStorage::clear()
{
    DataMappingLayer::MapperFacade::documentsMapper()->clear();
}

} // namespace DataStorageLayer

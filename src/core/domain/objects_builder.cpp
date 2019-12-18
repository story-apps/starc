#include "objects_builder.h"

#include "document_object.h"


namespace Domain
{

DocumentObject* ObjectsBuilder::create(const Identifier& _id, const QUuid& _uuid, DocumentObjectType _type,
    const QByteArray& _content)
{
    return new DocumentObject(_id, _uuid, _type, _content);
}

} // namespace Domain

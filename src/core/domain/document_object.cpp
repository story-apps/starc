#include "document_object.h"


namespace Domain
{

const QByteArray DocumentObject::kDefaultStructureContent = "";

const QUuid& DocumentObject::uuid() const
{
    return m_uuid;
}

void DocumentObject::setUuid(const QUuid& _uuid)
{
    if (m_uuid == _uuid) {
        return;
    }

    m_uuid = _uuid;
    markChangesNotStored();
}

DocumentObjectType DocumentObject::type() const
{
    return m_type;
}

void DocumentObject::setType(DocumentObjectType _type)
{
    if (m_type == _type) {
        return;
    }

    m_type = _type;
    markChangesNotStored();
}

const QByteArray& DocumentObject::content() const
{
    return m_content;
}

void DocumentObject::setContent(const QByteArray& _content)
{
    //
    // NOTE: Тут специально нет проверки, т.к. данные могут быть очень большими
    //

    m_content = _content;
    markChangesNotStored();
}

DocumentObject::DocumentObject(const Identifier& _id, const QUuid& _uuid, DocumentObjectType _type,
    const QByteArray& _content)
    : DomainObject(_id),
      m_uuid(_uuid),
      m_type(_type),
      m_content(_content)
{
}

} // namespace Domain

#include "domain_object.h"

namespace Domain {

DomainObject::DomainObject(const Identifier& _id)
    : m_id(_id)
    , m_isChangesStored(_id.isValid())
{
}

Identifier DomainObject::id() const
{
    return m_id;
}

void DomainObject::setId(const Identifier& _id)
{
    m_id = _id;
}

bool DomainObject::isChangesStored() const
{
    return m_isChangesStored;
}

void DomainObject::markChangesStored()
{
    m_isChangesStored = true;
}

void DomainObject::markChangesNotStored()
{
    m_isChangesStored = false;
}

} // namespace Domain

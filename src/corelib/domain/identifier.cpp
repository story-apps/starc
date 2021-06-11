#include "identifier.h"

namespace Domain {

namespace {
const int kInvalidValue = -1;
}

Identifier::Identifier()
    : m_id(kInvalidValue)
    , m_version(kInvalidValue)
    , m_isValid(false)
{
}

Identifier::Identifier(int _id, int _version)
    : m_id(_id)
    , m_version(_version)
    , m_isValid(false)
{
    if (_id != kInvalidValue && _version != kInvalidValue)
        m_isValid = true;
}

Identifier Identifier::next() const
{
    return Identifier(value() + 1);
}

Identifier Identifier::nextVersion() const
{
    return Identifier(value(), version() + 1);
}

bool Identifier::isValid() const
{
    return m_isValid;
}

int Identifier::value() const
{
    return m_id;
}

int Identifier::version() const
{
    return m_version;
}

} // namespace Domain

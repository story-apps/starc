#include "user.h"


User::User() = default;

User::User(const QString& _name)
    : m_name(_name)
{
}

bool User::isValid() const
{
    return !m_name.isEmpty();
}

QString User::name() const
{
    return m_name;
}

bool operator==(const User& _lhs, const User& _rhs)
{
    return _lhs.name() == _rhs.name();
}

bool operator!=(const User& _lhs, const User& _rhs)
{
    return !(_lhs == _rhs);
}

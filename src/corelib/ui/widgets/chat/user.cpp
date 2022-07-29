#include "user.h"

#include <QColor>


class User::Implementation
{
public:
    Implementation(const QString& _name, const QString& _email);

    QString name;
    QString email;
    QColor avatarColor;
};

User::Implementation::Implementation(const QString& _name, const QString& _email)
    : name(_name)
    , email(_email)
{
    ushort hash = 0;
    for (int characterIndex = 0; characterIndex < name.length(); ++characterIndex) {
        hash += name.at(characterIndex).unicode() + ((hash << 5) - hash);
    }
    hash = hash % 360;
    avatarColor = QColor::fromHsl(hash, 255 * 0.4, 255 * 0.5);
}


// ****


User::User()
    : d(new Implementation({}, {}))
{
}

User::User(const QString& _name, const QString& _email)
    : d(new Implementation(_name, _email))
{
}

User::User(const User& _other)
    : d(new Implementation(_other.name(), _other.email()))
{
}

User User::operator=(const User& _other)
{
    d->name = _other.name();
    d->email = _other.email();
    return *this;
}

User::~User() = default;

bool User::isValid() const
{
    return !d->name.isEmpty();
}

QString User::name() const
{
    return d->name;
}

QString User::email() const
{
    return d->email;
}

QColor User::avatarColor() const
{
    return d->avatarColor;
}

bool operator==(const User& _lhs, const User& _rhs)
{
    return _lhs.name() == _rhs.name() && _lhs.email() == _rhs.email();
}

bool operator!=(const User& _lhs, const User& _rhs)
{
    return !(_lhs == _rhs);
}

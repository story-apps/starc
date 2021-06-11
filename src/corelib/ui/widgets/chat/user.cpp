#include "user.h"

#include <QColor>
#include <QString>


class User::Implementation
{
public:
    explicit Implementation(const QString& _name);

    QString name;
    QColor avatarColor;
};

User::Implementation::Implementation(const QString& _name)
    : name(_name)
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
    : d(new Implementation({}))
{
}

User::User(const QString& _name)
    : d(new Implementation(_name))
{
}

User::User(const User& _other)
    : d(new Implementation(_other.name()))
{
}

User User::operator=(const User& _other)
{
    d->name = _other.name();
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

QColor User::avatarColor() const
{
    return d->avatarColor;
}

bool operator==(const User& _lhs, const User& _rhs)
{
    return _lhs.name() == _rhs.name();
}

bool operator!=(const User& _lhs, const User& _rhs)
{
    return !(_lhs == _rhs);
}

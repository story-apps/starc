#pragma once

#include <QScopedPointer>

class QColor;

/**
 * @brief Пользователь
 */
class User
{
public:
    User();
    explicit User(const QString& _name);
    User(const User& _other);
    virtual ~User();
    User operator=(const User& _other);

    bool isValid() const;

    QString name() const;

    QColor avatarColor() const;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

/**
 * @brief Определим оператор сравнения двух пользователей
 */
bool operator==(const User& _lhs, const User& _rhs);
bool operator!=(const User& _lhs, const User& _rhs);

#pragma once

#include <QScopedPointer>
#include <QString>

#include <corelib_global.h>

class QColor;

/**
 * @brief Пользователь
 */
class CORE_LIBRARY_EXPORT User
{
public:
    User();
    User(const QString& _name, const QString& _email);
    User(const User& _other);
    virtual ~User();
    User operator=(const User& _other);

    bool isValid() const;

    QString name() const;
    QString email() const;

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

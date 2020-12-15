#pragma once

#include <QString>


/**
 * @brief Пользователь
 */
class User
{
public:
    User();
    explicit User(const QString& _name);

    bool isValid() const;

    QString name() const;

private:
    QString m_name;
};

/**
 * @brief Определим оператор сравнения двух пользователей
 */
bool operator==(const User& _lhs, const User& _rhs);
bool operator!=(const User& _lhs, const User& _rhs);

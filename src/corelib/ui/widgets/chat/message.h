#pragma once

#include "user.h"

#include <QDateTime>


/**
 * @brief Сообщение чата
 */
class Message
{
public:
    Message();
    Message(const QDateTime& _dateTime, const QString& _text, const User& _author);

    QDateTime dateTime() const;
    QString text() const;
    User author() const;

private:
    QDateTime m_dateTime;
    QString m_text;
    User m_author;
};


bool operator==(const Message& _lhs, const Message& _rhs);
bool operator!=(const Message& _lhs, const Message& _rhs);

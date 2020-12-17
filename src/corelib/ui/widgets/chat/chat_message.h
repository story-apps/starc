#pragma once

#include "user.h"

#include <QDateTime>


/**
 * @brief Сообщение чата
 */
class ChatMessage
{
public:
    ChatMessage();
    ChatMessage(const QDateTime& _dateTime, const QString& _text, const User& _author);

    QDateTime dateTime() const;
    QString text() const;
    User author() const;

private:
    QDateTime m_dateTime;
    QString m_text;
    User m_author;
};


bool operator==(const ChatMessage& _lhs, const ChatMessage& _rhs);
bool operator!=(const ChatMessage& _lhs, const ChatMessage& _rhs);

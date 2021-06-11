#include "chat_message.h"


ChatMessage::ChatMessage() = default;

ChatMessage::ChatMessage(const QDateTime& _dateTime, const QString& _text, const User& _author)
    : m_dateTime(_dateTime)
    , m_text(_text)
    , m_author(_author)
{
}

QDateTime ChatMessage::dateTime() const
{
    return m_dateTime;
}

QString ChatMessage::text() const
{
    return m_text;
}

User ChatMessage::author() const
{
    return m_author;
}

bool operator==(const ChatMessage& _lhs, const ChatMessage& _rhs)
{
    return _lhs.dateTime() == _rhs.dateTime() && _lhs.text() == _rhs.text()
        && _lhs.author() == _rhs.author();
}

bool operator!=(const ChatMessage& _lhs, const ChatMessage& _rhs)
{
    return !(_lhs == _rhs);
}

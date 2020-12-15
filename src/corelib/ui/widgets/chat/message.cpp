#include "message.h"


Message::Message() = default;

Message::Message(const QDateTime& _dateTime, const QString& _text, const User& _author)
    : m_dateTime(_dateTime),
      m_text(_text),
      m_author(_author)
{
}

QDateTime Message::dateTime() const
{
    return m_dateTime;
}

QString Message::text() const
{
    return m_text;
}

User Message::author() const
{
    return m_author;
}

bool operator==(const Message& _lhs, const Message& _rhs)
{
    return _lhs.dateTime() == _rhs.dateTime()
            && _lhs.text() == _rhs.text()
            && _lhs.author() == _rhs.author();
}

bool operator!=(const Message& _lhs, const Message& _rhs)
{
    return !(_lhs == _rhs);
}

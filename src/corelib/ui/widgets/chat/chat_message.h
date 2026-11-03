#pragma once

#include "user.h"

#include <QDateTime>


/**
 * @brief Сообщение чата
 */
class CORE_LIBRARY_EXPORT ChatMessage
{
public:
    ChatMessage();
    ChatMessage(const QDateTime& _dateTime, const QString& _text, const User& _author,
                bool _isSystemMessage);

    QDateTime dateTime() const;
    QString text() const;
    User author() const;
    bool isSistemMessage() const;

private:
    QDateTime m_dateTime;
    QString m_text;
    User m_author;
    bool m_isSistemMessage = false;
};


bool operator==(const ChatMessage& _lhs, const ChatMessage& _rhs);
bool operator!=(const ChatMessage& _lhs, const ChatMessage& _rhs);

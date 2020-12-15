#pragma once

#include <corelib_global.h>

#include <QScrollArea>

class Message;
class User;


/**
 * @brief Представление списка сообщений чата
 */
class CORE_LIBRARY_EXPORT MessagesView : public QScrollArea
{
    Q_OBJECT

public:
    explicit MessagesView(QWidget* _parent = nullptr);
    ~MessagesView() override;

    /**
     * @brief Задать текущего пользователя чата
     */
    void setCurrectUser(const User& _user);

    /**
     * @brief Установить список сообщений
     */
    void setMessages(const QVector<Message>& _messages);

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};





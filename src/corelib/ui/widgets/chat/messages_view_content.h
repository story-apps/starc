#pragma once

#include <ui/widgets/widget/widget.h>

class Message;
class User;


/**
 * @brief Отрисовщик списка сообщений чата
 */
class MessagesViewContent : public Widget
{
    Q_OBJECT

public:
    explicit MessagesViewContent(QWidget* _parent = nullptr);
    ~MessagesViewContent() override;

    /**
     * @brief Задать текущего пользователя чата
     */
    void setCurrectUser(const User& _user);

    /**
     * @brief Установить список сообщений
     */
    void setMessages(const QVector<Message>& _messages);

    /**
     * @brief Получить необходимую высоту в зависимости от ширины
     */
    int heightForWidth(int _width) const override;

protected:
    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

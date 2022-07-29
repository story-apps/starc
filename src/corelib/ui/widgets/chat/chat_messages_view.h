#pragma once

#include <ui/widgets/widget/widget.h>

class ChatMessage;
class User;


/**
 * @brief Отрисовщик списка сообщений чата
 */
class CORE_LIBRARY_EXPORT ChatMessagesView : public Widget
{
    Q_OBJECT

public:
    explicit ChatMessagesView(QWidget* _parent = nullptr);
    ~ChatMessagesView() override;

    /**
     * @brief Задать текущего пользователя чата
     */
    void setCurrentUser(const User& _user);

    /**
     * @brief Установить список сообщений
     */
    void setMessages(const QVector<ChatMessage>& _messages);

    /**
     * @brief Получить необходимую высоту в зависимости от ширины
     */
    int heightForWidth(int _width) const override;

signals:
    /**
     * @brief Пользователь хочет отобразить контекстное меню для сообщения
     */
    void messageContextMenuRequested(int _messageIndex);

protected:
    /**
     * @brief Переопределяем для собственной отрисовки
     */
    void paintEvent(QPaintEvent* _event) override;

private:
    class Implementation;
    QScopedPointer<Implementation> d;
};

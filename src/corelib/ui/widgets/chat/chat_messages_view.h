#pragma once

#include <ui/widgets/widget/widget.h>

class ChatMessage;
class User;
class QUrl;


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
     * @brief Получить текущие сообщения, например, для действий контекстного меню
     */
    QVector<ChatMessage> messages() const;
    QString selectedText() const;

    // Compact bubbles with timestamps for conversational sidebars.
    void setConversationStyle(bool _enabled);
    // Optional in-conversation controls, e.g. a command configuration card.
    // Ownership is transferred to this view; call again after changing its content.
    void setFooterWidget(QWidget* _widget);

    /**
     * @brief Получить необходимую высоту в зависимости от ширины
     */
    int heightForWidth(int _width) const override;

signals:
    /**
     * @brief Пользователь хочет отобразить контекстное меню для сообщения
     */
    void messageContextMenuRequested(int _messageIndex);
    void linkActivated(const QUrl& _url);

protected:
    /**
     * @brief Согласуем размеры текста и внешней области прокрутки
     */
    void resizeEvent(QResizeEvent* _event) override;
    void designSystemChangeEvent(DesignSystemChangeEvent* _event) override;
    void processBackgroundColorChange() override;
    void processTextColorChange() override;

private:
    void rebuildDocument();
    void updateFooterGeometry();
    class Implementation;
    QScopedPointer<Implementation> d;
};

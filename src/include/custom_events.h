#pragma once

#include <QEvent>


/**
 * @brief Типы дополнительных событий
 */
enum class EventType {
    //
    // Событие простоя приложения
    //
    IdleEvent = QEvent::User + 1,
    //
    // Событие изменения дизайн системы
    //
    DesignSystemChangeEvent
};


/**
 * @brief Событие простоя приложения
 */
class IdleEvent : public QEvent
{
public:
    IdleEvent()
        : QEvent(static_cast<QEvent::Type>(EventType::IdleEvent))
    {
    }
};

/**
 * @brief Уведомление об изменении дизайн системы
 */
class DesignSystemChangeEvent : public QEvent
{
public:
    DesignSystemChangeEvent()
        : QEvent(static_cast<QEvent::Type>(EventType::DesignSystemChangeEvent))
    {
    }
};

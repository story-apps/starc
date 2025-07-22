#pragma once

#include <optional>

#include <QEvent>
#include <QString>
#include <QPair>


/**
 * @brief Типы дополнительных событий
 */
enum class EventType {
    //
    // Событие простоя приложения
    //
    IdleEvent = QEvent::User + 1,
    //
    // Событие смены фокуса в приложении
    //
    FocusChangeEvent,
    //
    // Событие изменения дизайн системы
    //
    DesignSystemChangeEvent,
    //
    // Событие смены параметров текстового редактора
    //
    TextEditingOptionsChangeEvent
};


/**
 * @brief Событие простоя приложения
 */
class IdleEvent : public QEvent
{
public:
    IdleEvent(bool isLongIdle)
        : QEvent(static_cast<QEvent::Type>(EventType::IdleEvent))
        , isLongIdle(isLongIdle)
    {
    }

    /**
     * @brief Долгий ли был простой
     */
    const bool isLongIdle;
};

/**
 * @brief Событие смены фокуса
 */
class FocusChangeEvent : public QEvent
{
public:
    FocusChangeEvent()
        : QEvent(static_cast<QEvent::Type>(EventType::FocusChangeEvent))
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

/**
 * @brief Уведомление об изменении параметров проверки орфографии
 */
class TextEditingOptionsChangeEvent : public QEvent
{
public:
    TextEditingOptionsChangeEvent()
        : QEvent(static_cast<QEvent::Type>(EventType::TextEditingOptionsChangeEvent))
    {
    }

    /**
     * @brief Проверка орфографии
     */
    struct Spelling {
        bool enabled = false;
        QString languageCode;
    };
    std::optional<Spelling> spelling;

    /**
     * @brief Нужно ли исправлять ДВойные ЗАглавные буквы
     */
    std::optional<bool> correctDoubleCapitals;

    /**
     * @brief Необходимо ли заменять i на I
     */
    std::optional<bool> capitalizeSingleILetter;

    /**
     * @brief Необходимо ли заменять три точки на многоточие
     */
    std::optional<bool> replaceThreeDots;

    /**
     * @brief Использовать умные кавычки для текущего языка интерфейса
     */
    std::optional<bool> useSmartQuotes;

    /**
     * @brief Заменять два тира на длинное тире
     */
    std::optional<bool> replaceTwoDashes;

    /**
     * @brief Запретить вводить несколько пробелов подряд
     */
    std::optional<bool> avoidMultipleSpaces;

};

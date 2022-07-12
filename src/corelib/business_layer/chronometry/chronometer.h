#pragma once

#include <chrono>
#include <corelib_global.h>

class QTextBlock;


namespace BusinessLayer {

enum class TextParagraphType;

/**
 * @brief Тип счётчика хронометража
 */
enum class CORE_LIBRARY_EXPORT ChronometerType {
    Page,
    Characters,
    Configurable,
};

/**
 * @brief Фасад для вычисления хронометража способом, настроенным пользователем
 */
class CORE_LIBRARY_EXPORT ScreenplayChronometer
{
public:
    /**
     * @brief Определить длительность заданного блока
     */
    static std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                              const QString& _templateId);
};

/**
 * @brief Фасад для вычисления хронометража способом, настроенным пользователем
 */
class CORE_LIBRARY_EXPORT AudioplayChronometer
{
public:
    /**
     * @brief Определить длительность заданного блока
     */
    static std::chrono::milliseconds duration(TextParagraphType _type, const QString& _text,
                                              const QString& _templateId);
};

} // namespace BusinessLayer

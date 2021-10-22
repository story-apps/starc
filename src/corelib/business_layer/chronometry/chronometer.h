#pragma once

#include <chrono>
#include <corelib_global.h>

class QTextBlock;


namespace BusinessLayer {

enum class ScreenplayParagraphType;

/**
 * @brief Тип счётчика хронометража
 */
enum class CORE_LIBRARY_EXPORT ChronometerType { Page, Characters };

/**
 * @brief Фасад для вычисления хронометража способом, настроенным пользователем
 */
class CORE_LIBRARY_EXPORT Chronometer
{
public:
    /**
     * @brief Определить длительность заданного блока
     */
    static std::chrono::milliseconds duration(ScreenplayParagraphType _type, const QString& _text,
                                              const QString& _screenplayTemplateId);
};

} // namespace BusinessLayer

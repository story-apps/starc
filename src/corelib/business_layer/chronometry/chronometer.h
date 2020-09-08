#pragma once

#include <corelib_global.h>

#include <chrono>

class QTextBlock;


namespace BusinessLayer
{

/**
 * @brief Фасад для вычисления хронометража способом, настроенным пользователем
 */
class CORE_LIBRARY_EXPORT Chronometer
{
public:
    /**
     * @brief Определить длительность заданного блока
     */
    static std::chrono::seconds duration(const QTextBlock& _block);
};

} // namespace BusinessLayer

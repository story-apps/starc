#pragma once

#include <corelib_global.h>

#include <chrono>

class QString;


/**
 * @brief Вспомогательный класс для работы со временем
 */
class CORE_LIBRARY_EXPORT TimeHelper
{
public:
    static QString toString(std::chrono::seconds _seconds);
    static QString toString(std::chrono::milliseconds _milliseconds);
};


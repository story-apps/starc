#pragma once

#include <corelib_global.h>

class QWidget;


/**
 * @brief Платформозависимые вещи инкапсулируются тут
 */
class CORE_LIBRARY_EXPORT PlatformHelper
{
public:
    /**
     * @brief Настроить вывод в консоль
     */
    static void initConsoleOutput();

    /**
     * @brief Настроить заголовок окна в зависимости от темы
     */
    static void setTitleBarTheme(QWidget* _window, bool _isLightTheme);
};

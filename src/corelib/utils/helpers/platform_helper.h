#pragma once

class QWidget;


/**
 * @brief Платформозависимые вещи инкапсулируются тут
 */
class PlatformHelper
{
public:
    /**
     * @brief Настроить заголовок окна в зависимости от темы
     */
    static void setTitleBarTheme(QWidget *_window, bool _isLightTheme);
};


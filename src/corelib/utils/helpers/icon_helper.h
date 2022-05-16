#pragma once

#include <corelib_global.h>


/**
 * @brief Вспомогательные функции для работы с иконками
 */
class CORE_LIBRARY_EXPORT IconHelper
{
public:
    /**
     * @brief Получить иконку в зависимости от направления лейаута приложения
     */
    static QString directedIcon(const QString& _icon);
};

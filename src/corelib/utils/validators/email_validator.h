#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Класс - сборник валидоторов под разные нужды
 */
class CORE_LIBRARY_EXPORT EmailValidator
{
public:
    /**
     * @brief Проверить валидность адреса электронной почты
     */
    static bool isValid(const QString& _email);
};

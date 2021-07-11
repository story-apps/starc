#pragma once

#include <QRegExpValidator>
#include <QString>

#include <corelib_global.h>


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

#pragma once

#include <corelib_global.h>

class QString;

/**
 * @brief Вспомогательные функции для работы с файлами
 */
class CORE_LIBRARY_EXPORT FileHelper
{
public:
    /**
     * @brief Получить имя файла, которое можно сохранить в системе
     */
    static QString systemSavebleFileName(const QString& _fileName);
};

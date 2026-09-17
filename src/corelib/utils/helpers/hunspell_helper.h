#pragma once

#include <corelib_global.h>

class QString;


/**
 * @brief Класс со вспомогательными методами для работы с hunspell
 */
class CORE_LIBRARY_EXPORT HunspellHelper
{
public:
    /**
     * @brief Проверить корректность файлов словаря
     * @return Текст ошибки, либо пустоту, если ошибки нет
     */
    static QString validateDictionaryFiles(const QString& _affPath, const QString& _dicPath);
    static QString validateAffFile(const QString& _affPath);
    static QString validateDicFile(const QString& _dicPath);
};

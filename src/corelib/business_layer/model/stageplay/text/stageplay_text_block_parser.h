#pragma once

#include <corelib_global.h>

class QString;


namespace BusinessLayer {

/**
 * @brief Парсер текста блока персонаж
 */
class CORE_LIBRARY_EXPORT StageplayCharacterParser
{
public:
    /**
     * @brief Получить имя персонажа
     */
    static QString name(const QString& _text);
};

} // namespace BusinessLayer

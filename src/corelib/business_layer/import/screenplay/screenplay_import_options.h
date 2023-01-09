#pragma once

#include <QString>

#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Опции импорта
 */
struct CORE_LIBRARY_EXPORT ScreenplayImportOptions {
    /**
     * @brief Путь файла для импорта
     */
    QString filePath;

    /**
     * @brief Нужно ли импортировать персонажей
     */
    bool importCharacters = true;

    /**
     * @brief Нужно ли импортировать локации
     */
    bool importLocations = true;

    /**
     * @brief Нужно ли импортировать сценарий
     */
    bool importScreenplay = true;

    /**
     * @brief Сохранять номера сцен импортируемого сценария
     */
    bool keepSceneNumbers = false;
};

} // namespace BusinessLayer

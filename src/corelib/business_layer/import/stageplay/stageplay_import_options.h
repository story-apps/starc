#pragma once

#include <QString>

#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Опции импорта
 */
struct CORE_LIBRARY_EXPORT StageplayImportOptions {
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
    bool importStageplay = true;

    /**
     * @brief Сохранять номера сцен импортируемого сценария
     */
    bool keepSceneNumbers = true;
};

} // namespace BusinessLayer

#pragma once

#include <business_layer/import/import_options.h>

#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Опции импорта
 */
struct CORE_LIBRARY_EXPORT ScreenplayImportOptions : public ImportOptions {
    /**
     * @brief Нужно ли импортировать документы
     */
    bool importResearch = true;

    /**
     * @brief Сохранять номера сцен импортируемого сценария
     */
    bool keepSceneNumbers = false;
};

} // namespace BusinessLayer

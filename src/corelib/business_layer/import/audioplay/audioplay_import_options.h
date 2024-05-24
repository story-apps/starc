#pragma once

#include <business_layer/import/import_options.h>

#include <corelib_global.h>


namespace BusinessLayer {

/**
 * @brief Опции импорта
 */
struct CORE_LIBRARY_EXPORT AudioplayImportOptions : public ImportOptions {
    /**
     * @brief Сохранять номера сцен импортируемого документа
     */
    bool keepSceneNumbers = true;
};

} // namespace BusinessLayer

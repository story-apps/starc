#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта персонажей
 */
struct CORE_LIBRARY_EXPORT CharactersExportOptions : public DocumentsExportOptions {

    CharactersExportOptions() = default;
    CharactersExportOptions(const DocumentsExportOptions& _other)
    {
        DocumentsExportOptions::copy(&_other, this);
    }

    /**
     * @brief Печатать конкретные параметры персонажей
     */
    bool includeAge = false;
    bool includeGender = false;
};

} // namespace BusinessLayer

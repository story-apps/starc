#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта локации
 */
struct CORE_LIBRARY_EXPORT WorldExportOptions : public DocumentExportOptions {

    WorldExportOptions() = default;
    WorldExportOptions(const DocumentExportOptions& _other)
    {
        DocumentExportOptions::copy(&_other, this);
    }

    /**
     * @brief Печатать дополнительные блоки данных о локации
     */
    bool includeWorldDescriptionInfo = true;
    bool includeNatureInfo = false;
    bool includeCultureInfo = false;
    bool includeSystemInfo = false;
    bool includePoliticsInfo = false;
    bool includeMagicInfo = false;
};

} // namespace BusinessLayer

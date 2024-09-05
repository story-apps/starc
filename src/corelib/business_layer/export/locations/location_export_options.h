#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта локации
 */
struct CORE_LIBRARY_EXPORT LocationExportOptions : public DocumentExportOptions {

    LocationExportOptions() = default;
    LocationExportOptions(const DocumentExportOptions& _other)
    {
        DocumentExportOptions::copy(&_other, this);
    }

    /**
     * @brief Печатать дополнительные блоки данных о локации
     */
    bool includeSenseInfo = true;
    bool includeGeographyInfo = false;
    bool includeBackgroundInfo = false;
};

} // namespace BusinessLayer

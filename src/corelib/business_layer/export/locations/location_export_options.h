#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT LocationExportOptions : public ExportOptions {
    /**
     * @brief Печатать фотографии
     */
    bool includeMainPhoto = true;
    bool includeAdditionalPhotos = false;

    /**
     * @brief Печатать дополнительные блоки данных о локации
     */
    bool includeSenseInfo = true;
    bool includeGeographyInfo = false;
    bool includeBackgroundInfo = false;
};

} // namespace BusinessLayer

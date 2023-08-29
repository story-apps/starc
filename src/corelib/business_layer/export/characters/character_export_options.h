#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT CharacterExportOptions : public ExportOptions {
    /**
     * @brief Печатать фотографии
     */
    bool includeMainPhoto = true;
    bool includeAdditionalPhotos = false;

    /**
     * @brief Печатать дополнительные блоки данных о персонаже
     */
    bool includeStoryInfo = true;
    bool includePersonalInfo = false;
    bool includePhysiqueInfo = false;
    bool includeLifeInfo = false;
    bool includeAttitudeInfo = false;
    bool includeBiographyInfo = false;
};

} // namespace BusinessLayer

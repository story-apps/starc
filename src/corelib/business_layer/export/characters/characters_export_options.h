#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT CharactersExportOptions : public ExportOptions {
    /**
     * @brief Печатать фотографии
     */
    bool includeMainPhoto = true;

    /**
     * @brief Печатать конкретные параметры персонажей
     */
    bool includeStoryRole = true;
    bool includeAge = false;
    bool includeGender = false;
    bool includeOneLineDescription = false;
    bool includeLongDescription = false;
};

} // namespace BusinessLayer

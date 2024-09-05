#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта персонажа
 */
struct CORE_LIBRARY_EXPORT CharacterExportOptions : public DocumentExportOptions {

    CharacterExportOptions() = default;
    CharacterExportOptions(const DocumentExportOptions& _other)
    {
        DocumentExportOptions::copy(&_other, this);
    }

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

#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT LocationsExportOptions : public ExportOptions {
    /**
     * @brief Печатать фотографии
     */
    bool includeMainPhoto = true;

    /**
     * @brief Список локаций для выгрузки
     */
    QVector<QString> locations;

    /**
     * @brief Печатать конкретные параметры локаций
     */
    bool includeStoryRole = true;
    bool includeOneLineDescription = false;
    bool includeLongDescription = false;
};

} // namespace BusinessLayer

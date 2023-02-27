#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT NovelExportOptions : public ExportOptions {
    /**
     * @brief Печатать план
     */
    bool includeOutline = false;

    /**
     * @brief Нужно ли заменять заголовки глав заданной строкой
     */
    QString ornamentalBreak;
};

} // namespace BusinessLayer

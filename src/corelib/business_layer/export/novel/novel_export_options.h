#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта романа
 */
struct CORE_LIBRARY_EXPORT NovelExportOptions : public ExportOptions {

    NovelExportOptions() = default;
    NovelExportOptions(const ExportOptions& _other)
    {
        ExportOptions::copy(&_other, this);
    }

    /**
     * @brief Печатать план
     */
    bool includeOutline = false;

    /**
     * @brief Включать блоки окончания частей и глав
     */
    bool includeFooters = false;

    /**
     * @brief Нужно ли заменять заголовки глав заданной строкой
     */
    QString ornamentalBreak;
};

} // namespace BusinessLayer

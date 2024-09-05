#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта комикса
 */
struct CORE_LIBRARY_EXPORT ComicBookExportOptions : public ExportOptions {

    ComicBookExportOptions() = default;
    ComicBookExportOptions(const ExportOptions& _other)
    {
        ExportOptions::copy(&_other, this);
    }

    /**
     * @brief Использовать слова вместо цифр для заголовков страниц
     */
    bool useWordsInPageHeadings = false;
};

} // namespace BusinessLayer

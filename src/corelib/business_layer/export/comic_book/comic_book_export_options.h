#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT ComicBookExportOptions : public ExportOptions {
    /**
     * @brief Использовать слова вместо цифр для заголовков страниц
     */
    bool useWordsInPageHeadings = false;
};

} // namespace BusinessLayer

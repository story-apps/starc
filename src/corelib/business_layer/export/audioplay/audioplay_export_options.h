#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT AudioplayExportOptions : public ExportOptions {
    /**
     * @brief Нужно ли печатать номера блоков
     */
    bool showBlockNumbers = false;
};

} // namespace BusinessLayer

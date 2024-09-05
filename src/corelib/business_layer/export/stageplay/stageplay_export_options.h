#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта пьесы
 */
struct CORE_LIBRARY_EXPORT StageplayExportOptions : public ExportOptions {

    StageplayExportOptions() = default;
    StageplayExportOptions(const ExportOptions& _other)
    {
        ExportOptions::copy(&_other, this);
    }

    /**
     * @brief Нужно ли печатать номера блоков
     */
    bool showBlockNumbers = false;
};

} // namespace BusinessLayer

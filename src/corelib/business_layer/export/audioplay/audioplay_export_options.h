#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта аудиопьесы
 */
struct CORE_LIBRARY_EXPORT AudioplayExportOptions : public ExportOptions {

    AudioplayExportOptions() = default;
    AudioplayExportOptions(const ExportOptions& _other)
    {
        ExportOptions::copy(&_other, this);
    }

    /**
     * @brief Нужно ли печатать номера блоков
     */
    bool showBlockNumbers = false;
};

} // namespace BusinessLayer

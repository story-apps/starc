#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

/**
 * @brief Опции экспорта сценария
 */
struct CORE_LIBRARY_EXPORT ScreenplayExportOptions : public ExportOptions {

    ScreenplayExportOptions() = default;
    ScreenplayExportOptions(const ExportOptions& _other)
    {
        ExportOptions::copy(&_other, this);
    }

    /**
     * @brief Печатать поэпизодник
     */
    bool includeTreatment = false;

    /**
     * @brief Печатать номера сцен
     */
    bool showScenesNumbers = true;
    bool showScenesNumbersOnLeft = true;
    bool showScenesNumbersOnRight = true;

    /**
     * @brief Печатать номера реплик
     */
    bool showDialoguesNumbers = false;

    /**
     * @brief Список сцен для печати
     * @note Если пустое, значит печатаются все сцены
     */
    QVector<QString> exportScenes;
};

} // namespace BusinessLayer

#pragma once

#include <business_layer/export/export_options.h>

namespace BusinessLayer {

struct CORE_LIBRARY_EXPORT ScreenplayExportOptions : public ExportOptions {
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

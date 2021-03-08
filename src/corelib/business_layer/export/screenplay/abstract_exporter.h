#pragma once

#include <corelib_global.h>


namespace BusinessLayer
{

struct ExportOptions;
class ScreenplayTextModel;

/**
 * @brief Базовый класс для реализации экспортера сценария
 */
class CORE_LIBRARY_EXPORT AbstractExporter
{
public:
    virtual ~AbstractExporter() {}

    /**
     * @brief Экспорт сценария в файл
     */
    virtual void exportTo(ScreenplayTextModel* _model, const ExportOptions& _exportOptions) const = 0;
};

} // namespace BusinessLayer

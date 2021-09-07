#pragma once

#include <corelib_global.h>


namespace BusinessLayer {

struct ScreenplayExportOptions;
class ScreenplayTextDocument;
class ScreenplayTextModel;

/**
 * @brief Базовый класс для реализации экспортера сценария
 */
class CORE_LIBRARY_EXPORT ScreenplayAbstractExporter
{
public:
    virtual ~ScreenplayAbstractExporter()
    {
    }

    /**
     * @brief Экспорт сценария в файл
     */
    virtual void exportTo(ScreenplayTextModel* _model,
                          const ScreenplayExportOptions& _exportOptions) const = 0;

protected:
    /**
     * @brief Подготовить документ к экспорту в соответствии с заданными опциями
     * @note Владение документом передаётся клиенту
     */
    ScreenplayTextDocument* prepareDocument(ScreenplayTextModel* _model,
                                            const ScreenplayExportOptions& _exportOptions) const;
};

} // namespace BusinessLayer

#pragma once

#include <corelib_global.h>


namespace BusinessLayer {

struct ComicBookExportOptions;
class ComicBookTextDocument;
class ComicBookTextModel;

/**
 * @brief Базовый класс для реализации экспортера сценария
 */
class CORE_LIBRARY_EXPORT ComicBookAbstractExporter
{
public:
    virtual ~ComicBookAbstractExporter()
    {
    }

    /**
     * @brief Экспорт сценария в файл
     */
    virtual void exportTo(ComicBookTextModel* _model,
                          const ComicBookExportOptions& _exportOptions) const = 0;

protected:
    /**
     * @brief Подготовить документ к экспорту в соответствии с заданными опциями
     * @note Владение документом передаётся клиенту
     */
    ComicBookTextDocument* prepareDocument(ComicBookTextModel* _model,
                                           const ComicBookExportOptions& _exportOptions) const;
};

} // namespace BusinessLayer

#pragma once

#include "comic_book_abstract_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ComicBookDocxExporter : public ComicBookAbstractExporter
{
public:
    ComicBookDocxExporter() = default;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(ComicBookTextModel* _model,
                  const ComicBookExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

#pragma once

#include "comic_book_abstract_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ComicBookPdfExporter : public ComicBookAbstractExporter
{
public:
    ComicBookPdfExporter() = default;

    void exportTo(ComicBookTextModel* _model,
                  const ComicBookExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

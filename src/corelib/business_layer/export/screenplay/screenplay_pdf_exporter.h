#pragma once

#include "screenplay_abstract_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ScreenplayPdfExporter : public ScreenplayAbstractExporter
{
public:
    ScreenplayPdfExporter() = default;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(ScreenplayTextModel* _model,
                  const ScreenplayExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

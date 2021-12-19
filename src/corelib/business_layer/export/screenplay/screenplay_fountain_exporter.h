#pragma once

#include "screenplay_abstract_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ScreenplayFountainExporter : public ScreenplayAbstractExporter
{
public:
    ScreenplayFountainExporter() = default;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(ScreenplayTextModel* _model,
                  const ScreenplayExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

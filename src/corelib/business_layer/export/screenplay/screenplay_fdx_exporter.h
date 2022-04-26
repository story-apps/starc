#pragma once

#include "screenplay_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ScreenplayFdxExporter : public ScreenplayExporter
{
public:
    ScreenplayFdxExporter() = default;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(TextModel* _model, ExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

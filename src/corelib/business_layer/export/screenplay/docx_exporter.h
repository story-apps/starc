#pragma once

#include "abstract_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT DocxExporter : public AbstractExporter
{
public:
    DocxExporter() = default;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(ScreenplayTextModel* _model, const ExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

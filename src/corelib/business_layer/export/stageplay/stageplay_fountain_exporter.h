#pragma once

#include "stageplay_exporter.h"


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT StageplayFountainExporter : public StageplayExporter
{
public:
    StageplayFountainExporter() = default;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(AbstractModel* _model, ExportOptions& _exportOptions) const override;

    /**
     * @brief Экспортировать сценарий в заданном интервале текста
     */
    void exportTo(AbstractModel* _model, int _fromPosition, int _toPosition,
                  ExportOptions& _exportOptions) const;
};

} // namespace BusinessLayer

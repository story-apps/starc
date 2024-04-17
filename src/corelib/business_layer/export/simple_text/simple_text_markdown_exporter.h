#pragma once

#include "simple_text_exporter.h"

namespace BusinessLayer {

class CORE_LIBRARY_EXPORT SimpleTextMarkdownExporter : public SimpleTextExporter
{
public:
    SimpleTextMarkdownExporter() = default;

    /**
     * @brief Экспортировать текстовый документ
     */
    void exportTo(AbstractModel* _model, ExportOptions& _exportOptions) const override;

    /**
     * @brief Экспортировать текстовый документ в заданном интервале текста
     */
    void exportTo(AbstractModel* _model, int _fromPosition, int _toPosition,
                  ExportOptions& _exportOptions) const;
};

} // namespace BusinessLayer

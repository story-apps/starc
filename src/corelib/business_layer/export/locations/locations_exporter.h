#pragma once

#include <business_layer/export/abstract_exporter.h>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT LocationsExporter : virtual public AbstractExporter
{
protected:
    /**
     * @brief Создать документ для экспорта
     */
    TextDocument* createDocument(const ExportOptions& _exportOptions) const override;

    /**
     * @brief Получить шаблон конкретного документа
     */
    const TextTemplate& documentTemplate(const ExportOptions& _exportOptions) const override;

    /**
     * @brief Подготовить документ к экспорту в соответствии с заданными опциями
     * @note Владение документом передаётся клиенту
     */
    TextDocument* prepareDocument(AbstractModel* _model,
                                  const ExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

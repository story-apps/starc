#pragma once

#include <business_layer/export/abstract_exporter.h>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ScreenplayExporter : virtual public AbstractExporter
{
protected:
    /**
     * @brief Создать документ для экспорта
     */
    TextDocument* createDocument() const override;

    /**
     * @brief Получить шаблон конкретного документа
     */
    const TextTemplate& documentTemplate(const ExportOptions& _exportOptions) const override;

    /**
     * @brief Обработать блок необходимым образом в наследнике
     */
    bool prepareBlock(const ExportOptions& _exportOptions, TextCursor& _cursor) const override;
};

} // namespace BusinessLayer

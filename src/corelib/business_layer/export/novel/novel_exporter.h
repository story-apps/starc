#pragma once

#include <business_layer/export/abstract_exporter.h>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT NovelExporter : virtual public AbstractExporter
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
     * @brief Обработать блок необходимым образом в наследнике
     */
    bool prepareBlock(const ExportOptions& _exportOptions, TextCursor& _cursor) const override;

private:
    /**
     * @brief Обработан ли уже первый после части/главы блок заголовка сцены
     */
    mutable bool m_isFisrtSceneHeader = true;
};

} // namespace BusinessLayer

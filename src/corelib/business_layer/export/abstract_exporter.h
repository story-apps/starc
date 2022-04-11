#pragma once

#include <corelib_global.h>


namespace BusinessLayer {

struct ExportOptions;
class TextDocument;
class TextModel;
class TextTemplate;

/**
 * @brief Базовый класс для реализации экспортера сценария
 */
class CORE_LIBRARY_EXPORT AbstractExporter
{
public:
    virtual ~AbstractExporter()
    {
    }

    /**
     * @brief Экспорт сценария в файл
     */
    virtual void exportTo(TextModel* _model, ExportOptions& _exportOptions) const = 0;

protected:
    /**
     * @brief Создать документ для экспорта
     */
    virtual TextDocument* createDocument() const = 0;

    /**
     * @brief Получить шаблон конкретного документа
     */
    virtual const TextTemplate& documentTemplate(const ExportOptions& _exportOptions) const = 0;

    /**
     * @brief Подготовить документ к экспорту в соответствии с заданными опциями
     * @note Владение документом передаётся клиенту
     */
    TextDocument* prepareDocument(TextModel* _model, const ExportOptions& _exportOptions) const;
};

} // namespace BusinessLayer

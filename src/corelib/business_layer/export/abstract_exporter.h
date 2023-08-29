#pragma once

#include <corelib_global.h>


namespace BusinessLayer {

struct ExportOptions;
class TextCursor;
class TextDocument;
class AbstractModel;
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
    virtual void exportTo(AbstractModel* _model, ExportOptions& _exportOptions) const = 0;

protected:
    /**
     * @brief Создать документ для экспорта
     */
    virtual TextDocument* createDocument(const ExportOptions& _exportOptions) const = 0;

    /**
     * @brief Получить шаблон конкретного документа
     */
    virtual const TextTemplate& documentTemplate(const ExportOptions& _exportOptions) const = 0;

    /**
     * @brief Подготовить документ к экспорту в соответствии с заданными опциями
     * @note Владение документом передаётся клиенту
     *       Внутри реализован базовый экспортер для текстовых моделей, если нужно реализовать
     *       экспорт для модели другого типа, то метод нужно переопределить
     * @todo Вынести реализацию этого метода в AbstractTextExporter
     */
    virtual TextDocument* prepareDocument(AbstractModel* _model,
                                          const ExportOptions& _exportOptions) const;

    /**
     * @brief Обработать блок необходимым образом в наследнике
     */
    virtual bool prepareBlock(const ExportOptions& _exportOptions, TextCursor& _cursor) const;
};

} // namespace BusinessLayer

#pragma once

#include "abstract_exporter.h"

#include <QScopedPointer>


namespace BusinessLayer {

enum class TextParagraphType;

class CORE_LIBRARY_EXPORT AbstractDocxExporter : virtual public AbstractExporter
{
public:
    AbstractDocxExporter();
    ~AbstractDocxExporter() override;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(AbstractModel* _model, ExportOptions& _exportOptions) const override;

protected:
    /**
     * @brief Определить список стилей для экспорта
     */
    virtual QVector<TextParagraphType> paragraphTypes() const = 0;

    /**
     * @brief Обработать блок
     */
    virtual void processBlock(const TextCursor& _cursor, const ExportOptions& _exportOptions,
                              QString& _documentXml) const;

private:
    class Implementation;
    friend class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

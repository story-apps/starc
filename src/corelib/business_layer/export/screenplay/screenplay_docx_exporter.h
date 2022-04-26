#pragma once

#include "screenplay_exporter.h"

#include <business_layer/export/abstract_docx_exporter.h>

#include <QScopedPointer>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT ScreenplayDocxExporter : public ScreenplayExporter,
                                                   public AbstractDocxExporter
{
public:
    ScreenplayDocxExporter();

protected:
    /**
     * @brief Определить список стилей для экспорта
     */
    QVector<TextParagraphType> paragraphTypes() const override;

    /**
     * @brief Обработать блок
     */
    void processBlock(const TextCursor& _cursor, const ExportOptions& _exportOptions,
                      QString& _documentXml) const override;
};

} // namespace BusinessLayer

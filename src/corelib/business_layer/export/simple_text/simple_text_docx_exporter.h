#pragma once

#include "simple_text_exporter.h"

#include <business_layer/export/abstract_docx_exporter.h>

#include <QScopedPointer>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT SimpleTextDocxExporter : public SimpleTextExporter,
                                                   public AbstractDocxExporter
{
public:
    SimpleTextDocxExporter();

protected:
    /**
     * @brief Определить список стилей для экспорта
     */
    QVector<TextParagraphType> paragraphTypes() const override;
};

} // namespace BusinessLayer

#pragma once

#include "location_exporter.h"

#include <business_layer/export/abstract_docx_exporter.h>

#include <QScopedPointer>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT LocationDocxExporter : public LocationExporter,
                                                 public AbstractDocxExporter
{
public:
    LocationDocxExporter();

protected:
    /**
     * @brief Определить список стилей для экспорта
     */
    QVector<TextParagraphType> paragraphTypes() const override;
};

} // namespace BusinessLayer

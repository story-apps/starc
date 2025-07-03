#pragma once

#include "world_exporter.h"

#include <business_layer/export/abstract_docx_exporter.h>

#include <QScopedPointer>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT WorldDocxExporter : public WorldExporter, public AbstractDocxExporter
{
public:
    WorldDocxExporter();

protected:
    /**
     * @brief Определить список стилей для экспорта
     */
    QVector<TextParagraphType> paragraphTypes() const override;
};

} // namespace BusinessLayer

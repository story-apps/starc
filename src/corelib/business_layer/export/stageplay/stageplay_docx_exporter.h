#pragma once

#include "stageplay_exporter.h"

#include <business_layer/export/abstract_docx_exporter.h>

#include <QScopedPointer>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT StageplayDocxExporter : public StageplayExporter,
                                                  public AbstractDocxExporter
{
public:
    StageplayDocxExporter();

protected:
    /**
     * @brief Определить список стилей для экспорта
     */
    QVector<TextParagraphType> paragraphTypes() const override;
};

} // namespace BusinessLayer

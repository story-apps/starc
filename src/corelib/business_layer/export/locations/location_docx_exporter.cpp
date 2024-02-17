#include "location_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

LocationDocxExporter::LocationDocxExporter()
    : LocationExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> LocationDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,
    };
}

} // namespace BusinessLayer

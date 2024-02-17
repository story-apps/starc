#include "locations_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

LocationsDocxExporter::LocationsDocxExporter()
    : LocationsExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> LocationsDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,
    };
}

} // namespace BusinessLayer

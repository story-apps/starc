#include "world_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

WorldDocxExporter::WorldDocxExporter()
    : WorldExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> WorldDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,
    };
}

} // namespace BusinessLayer

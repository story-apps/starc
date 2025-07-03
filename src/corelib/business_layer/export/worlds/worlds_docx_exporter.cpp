#include "worlds_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

WorldsDocxExporter::WorldsDocxExporter()
    : WorldsExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> WorldsDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,
    };
}

} // namespace BusinessLayer

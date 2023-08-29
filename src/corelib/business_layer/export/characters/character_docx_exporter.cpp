#include "character_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

CharacterDocxExporter::CharacterDocxExporter()
    : CharacterExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> CharacterDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,
    };
}

} // namespace BusinessLayer

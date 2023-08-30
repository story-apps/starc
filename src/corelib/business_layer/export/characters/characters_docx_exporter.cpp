#include "characters_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

CharactersDocxExporter::CharactersDocxExporter()
    : CharactersExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> CharactersDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,
    };
}

} // namespace BusinessLayer

#include "stageplay_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

StageplayDocxExporter::StageplayDocxExporter()
    : StageplayExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> StageplayDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,     TextParagraphType::UnformattedText,
        TextParagraphType::SceneHeading,  TextParagraphType::Character,
        TextParagraphType::Parenthetical, TextParagraphType::Dialogue,
        TextParagraphType::Action,        TextParagraphType::InlineNote,
    };
}

} // namespace BusinessLayer

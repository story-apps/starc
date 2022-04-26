#include "audioplay_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

AudioplayDocxExporter::AudioplayDocxExporter()
    : AudioplayExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> AudioplayDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,    TextParagraphType::UnformattedText,
        TextParagraphType::SceneHeading, TextParagraphType::Character,
        TextParagraphType::Dialogue,     TextParagraphType::Sound,
        TextParagraphType::Music,        TextParagraphType::Cue,
        TextParagraphType::InlineNote,
    };
}

} // namespace BusinessLayer

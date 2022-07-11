#include "simple_text_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

SimpleTextDocxExporter::SimpleTextDocxExporter()
    : SimpleTextExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> SimpleTextDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,       TextParagraphType::ChapterHeading1,
        TextParagraphType::ChapterHeading2, TextParagraphType::ChapterHeading3,
        TextParagraphType::ChapterHeading4, TextParagraphType::ChapterHeading5,
        TextParagraphType::ChapterHeading6, TextParagraphType::Text,
        TextParagraphType::InlineNote,
    };
}

} // namespace BusinessLayer

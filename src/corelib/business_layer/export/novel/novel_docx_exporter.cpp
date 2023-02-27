#include "novel_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

NovelDocxExporter::NovelDocxExporter()
    : NovelExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> NovelDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,     TextParagraphType::UnformattedText,
        TextParagraphType::InlineNote,    TextParagraphType::PartHeading,
        TextParagraphType::PartFooter,    TextParagraphType::ChapterHeading,
        TextParagraphType::ChapterFooter, TextParagraphType::SceneHeading,
        TextParagraphType::Text,
    };
}

} // namespace BusinessLayer

#include "comic_book_docx_exporter.h"

#include <business_layer/templates/text_template.h>


namespace BusinessLayer {

ComicBookDocxExporter::ComicBookDocxExporter()
    : ComicBookExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> ComicBookDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,   TextParagraphType::UnformattedText,
        TextParagraphType::PageHeading, TextParagraphType::PanelHeading,
        TextParagraphType::Description, TextParagraphType::Character,
        TextParagraphType::Dialogue,    TextParagraphType::InlineNote,
    };
}

} // namespace BusinessLayer

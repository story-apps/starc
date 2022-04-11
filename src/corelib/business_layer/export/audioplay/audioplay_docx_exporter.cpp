#include "audioplay_docx_exporter.h"

#include "qtzip/QtZipWriter"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/document/text/text_document.h>
#include <business_layer/export/export_options.h>
#include <business_layer/model/text/text_model.h>
#include <business_layer/model/text/text_model_splitter_item.h>
#include <business_layer/templates/templates_facade.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QFile>
#include <QFontMetrics>
#include <QLocale>
#include <QTextBlock>
#include <QTextLayout>


namespace BusinessLayer {

AudioplayDocxExporter::AudioplayDocxExporter()
    : AudioplayExporter()
    , AbstractDocxExporter()
{
}

AudioplayDocxExporter::~AudioplayDocxExporter() = default;

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

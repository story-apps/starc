#include "audioplay_exporter.h"

#include <business_layer/document/audioplay/text/audioplay_text_document.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>

namespace BusinessLayer {

TextDocument* AudioplayExporter::createDocument() const
{
    return new AudioplayTextDocument;
}

const TextTemplate& AudioplayExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    return TemplatesFacade::audioplayTemplate(_exportOptions.templateId);
}
} // namespace BusinessLayer

#include "stageplay_exporter.h"

#include <business_layer/document/stageplay/text/stageplay_text_document.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>


namespace BusinessLayer {

TextDocument* StageplayExporter::createDocument() const
{
    auto document = new StageplayTextDocument;
    document->setCorrectionOptions(true);
    return document;
}

const TextTemplate& StageplayExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    return TemplatesFacade::stageplayTemplate(_exportOptions.templateId);
}
} // namespace BusinessLayer

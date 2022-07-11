#include "simple_text_exporter.h"

#include <business_layer/document/simple_text/simple_text_document.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/simple_text_template.h>
#include <business_layer/templates/templates_facade.h>


namespace BusinessLayer {

TextDocument* SimpleTextExporter::createDocument(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)

    auto document = new SimpleTextDocument;
    //    document->setCorrectionOptions(true);
    return document;
}

const TextTemplate& SimpleTextExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    return TemplatesFacade::simpleTextTemplate(_exportOptions.templateId);
}
} // namespace BusinessLayer

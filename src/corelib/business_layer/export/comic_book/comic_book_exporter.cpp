#include "comic_book_exporter.h"

#include <business_layer/document/comic_book/text/comic_book_text_document.h>
#include <business_layer/export/export_options.h>
#include <business_layer/templates/comic_book_template.h>
#include <business_layer/templates/templates_facade.h>


namespace BusinessLayer {

TextDocument* ComicBookExporter::createDocument() const
{
    auto document = new ComicBookTextDocument;
    document->setCorrectionOptions(true, true);
    return document;
}

const TextTemplate& ComicBookExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    return TemplatesFacade::comicBookTemplate(_exportOptions.templateId);
}

} // namespace BusinessLayer

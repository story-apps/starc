#include "export_options.h"

namespace BusinessLayer {

void ExportOptions::copy(const ExportOptions* _source, ExportOptions* _dest)
{
    _dest->filePath = _source->filePath;
    _dest->fileFormat = _source->fileFormat;
    _dest->templateId = _source->templateId;
    _dest->includeTitlePage = _source->includeTitlePage;
    _dest->includeSynopsis = _source->includeSynopsis;
    _dest->includeText = _source->includeText;
    _dest->includeFolders = _source->includeFolders;
    _dest->includeInlineNotes = _source->includeInlineNotes;
    _dest->includeReviewMarks = _source->includeReviewMarks;
    _dest->highlightCharacters = _source->highlightCharacters;
    _dest->highlightCharactersWithDialogues = _source->highlightCharactersWithDialogues;
    _dest->highlightCharactersList = _source->highlightCharactersList;
    _dest->watermark = _source->watermark;
    _dest->watermarkColor = _source->watermarkColor;
    _dest->header = _source->header;
    _dest->printHeaderOnTitlePage = _source->printHeaderOnTitlePage;
    _dest->footer = _source->footer;
    _dest->printFooterOnTitlePage = _source->printFooterOnTitlePage;
}

void DocumentsExportOptions::copy(const DocumentsExportOptions* _source,
                                  DocumentsExportOptions* _dest)
{
    ExportOptions::copy(_source, _dest);
    _dest->includeMainPhoto = _source->includeMainPhoto;
    _dest->documents = _source->documents;
    _dest->includeStoryRole = _source->includeStoryRole;
    _dest->includeOneLineDescription = _source->includeOneLineDescription;
    _dest->includeLongDescription = _source->includeLongDescription;
}

void DocumentExportOptions::copy(const DocumentExportOptions* _source, DocumentExportOptions* _dest)
{
    ExportOptions::copy(_source, _dest);
    _dest->includeMainPhoto = _source->includeMainPhoto;
    _dest->includeAdditionalPhotos = _source->includeAdditionalPhotos;
}

} // namespace BusinessLayer

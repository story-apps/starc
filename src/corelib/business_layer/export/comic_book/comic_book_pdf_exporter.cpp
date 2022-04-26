#include "comic_book_pdf_exporter.h"

#include "comic_book_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <utils/helpers/measurement_helper.h>

#include <QLocale>
#include <QPainter>


namespace BusinessLayer {

ComicBookPdfExporter::ComicBookPdfExporter()
    : ComicBookExporter()
    , AbstractPdfExporter()
{
}

void ComicBookPdfExporter::updateExportOptions(TextModel* _model,
                                               ExportOptions& _exportOptions) const
{
    const auto textModel = qobject_cast<ComicBookTextModel*>(_model);
    Q_ASSERT(textModel);

    _exportOptions.header = textModel->informationModel()->header();
    _exportOptions.footer = textModel->informationModel()->footer();
}

void ComicBookPdfExporter::printBlockDecorations(QPainter* _painter, qreal _pageYPos,
                                                 const QRectF& _body,
                                                 TextParagraphType _paragraphType,
                                                 const QRectF& _blockRect, const QTextBlock& _block,
                                                 const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_painter)
    Q_UNUSED(_pageYPos)
    Q_UNUSED(_body)
    Q_UNUSED(_paragraphType)
    Q_UNUSED(_blockRect)
    Q_UNUSED(_block)
    Q_UNUSED(_exportOptions)
}

} // namespace BusinessLayer

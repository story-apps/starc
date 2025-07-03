#include "worlds_pdf_exporter.h"


namespace BusinessLayer {

WorldsPdfExporter::WorldsPdfExporter()
    : WorldsExporter()
    , AbstractPdfExporter()
{
}

void WorldsPdfExporter::updateExportOptions(AbstractModel* _model,
                                            ExportOptions& _exportOptions) const
{
    Q_UNUSED(_model)
    Q_UNUSED(_exportOptions)
}

void WorldsPdfExporter::printBlockDecorations(QPainter* _painter, qreal _pageYPos,
                                              const QRectF& _body, TextParagraphType _paragraphType,
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

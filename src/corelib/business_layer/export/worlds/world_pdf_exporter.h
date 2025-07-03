#pragma once

#include "world_exporter.h"

#include <business_layer/export/abstract_pdf_exporter.h>


namespace BusinessLayer {

class CORE_LIBRARY_EXPORT WorldPdfExporter : public WorldExporter, public AbstractPdfExporter
{
public:
    WorldPdfExporter();

protected:
    /**
     * @brief Дописать в параметры экспорта данные зависящие от модели
     */
    void updateExportOptions(AbstractModel* _model, ExportOptions& _exportOptions) const override;

    /**
     * @brief Нарисовать декорацию блока
     */
    void printBlockDecorations(QPainter* _painter, qreal _pageYPos, const QRectF& _body,
                               TextParagraphType _paragraphType, const QRectF& _blockRect,
                               const QTextBlock& _block,
                               const ExportOptions& _exportOptions) const override;
};

} // namespace BusinessLayer

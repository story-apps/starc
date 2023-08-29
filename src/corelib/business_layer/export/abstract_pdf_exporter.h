#pragma once

#include "abstract_exporter.h"

#include <QScopedPointer>

class QAbstractTextDocumentLayout;
class QPainter;
class QRectF;
class QTextBlock;


namespace BusinessLayer {

enum class TextParagraphType;

class CORE_LIBRARY_EXPORT AbstractPdfExporter : virtual public AbstractExporter
{
public:
    AbstractPdfExporter();
    ~AbstractPdfExporter() override;

    /**
     * @brief Экспортировать сценарий
     */
    void exportTo(AbstractModel* _model, ExportOptions& _exportOptions) const override;

protected:
    /**
     * @brief Дописать в параметры экспорта данные зависящие от модели
     */
    virtual void updateExportOptions(AbstractModel* _model,
                                     ExportOptions& _exportOptions) const = 0;

    /**
     * @brief Нарисовать декорацию блока
     */
    virtual void printBlockDecorations(QPainter* _painter, qreal _pageYPos, const QRectF& _body,
                                       TextParagraphType _paragraphType, const QRectF& _blockRect,
                                       const QTextBlock& _block,
                                       const ExportOptions& _exportOptions) const = 0;

private:
    class Implementation;
    friend class Implementation;
    QScopedPointer<Implementation> d;
};

} // namespace BusinessLayer

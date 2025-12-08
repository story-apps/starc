#include "audioplay_pdf_exporter.h"

#include "audioplay_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QPainter>


namespace BusinessLayer {

AudioplayPdfExporter::AudioplayPdfExporter()
    : AudioplayExporter()
    , AbstractPdfExporter()
{
}

void AudioplayPdfExporter::updateExportOptions(AbstractModel* _model,
                                               ExportOptions& _exportOptions) const
{
    const auto textModel = qobject_cast<AudioplayTextModel*>(_model);
    Q_ASSERT(textModel);

    _exportOptions.header = textModel->informationModel()->header();
    _exportOptions.footer = textModel->informationModel()->footer();
}

void AudioplayPdfExporter::printBlockDecorations(QPainter* _painter, qreal _pageYPos,
                                                 const QRectF& _body,
                                                 TextParagraphType _paragraphType,
                                                 const QRectF& _blockRect, const QTextBlock& _block,
                                                 const ExportOptions& _exportOptions) const
{
    const auto& exportTemplate = documentTemplate(_exportOptions);
    const auto& exportOptions = static_cast<const AudioplayExportOptions&>(_exportOptions);

    //
    // Покажем номер блока, если необходимо
    //
    if (exportOptions.showBlockNumbers && !_block.text().isEmpty()
        && (_paragraphType == TextParagraphType::Dialogue
            || _paragraphType == TextParagraphType::Sound
            || _paragraphType == TextParagraphType::Music
            || _paragraphType == TextParagraphType::Cue)) {

        const auto blockData = static_cast<TextBlockData*>(_block.userData());
        if (blockData != nullptr) {
            QFont font = TextHelper::fineBlockCharFormat(_block).font();
            font.setUnderline(false);
            _painter->setFont(font);
            //
            const auto textItem = static_cast<AudioplayTextModelTextItem*>(blockData->item());
            if (textItem && textItem->number().has_value()) {
                const QString dialogueNumber = textItem->number()->text;
                const int numberDelta
                    = TextHelper::fineTextWidthF(dialogueNumber, _painter->font());
                const auto& characterBlockStyle
                    = exportTemplate.paragraphStyle(TextParagraphType::Character);
                QRectF dialogueNumberRect;
                if (QLocale().textDirection() == Qt::LeftToRight) {
                    if (characterBlockStyle.marginsOnHalfPage().left() > 0) {
                        dialogueNumberRect
                            = QRectF(MeasurementHelper::mmToPx(exportTemplate.pageMargins().left()),
                                     _blockRect.top(), numberDelta,
                                     TextHelper::fineLineSpacing(_painter->font()));
                    } else {
                        const int distanceBetweenSceneNumberAndText = 10;
                        dialogueNumberRect
                            = QRectF(0, _blockRect.top(),
                                     MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                         - distanceBetweenSceneNumberAndText,
                                     TextHelper::fineLineSpacing(_painter->font()));
                    }
                } else {
                    if (_block.blockFormat().rightMargin() > numberDelta) {
                        dialogueNumberRect
                            = QRectF(MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                         + _body.width() - numberDelta,
                                     _blockRect.top(), numberDelta,
                                     TextHelper::fineLineSpacing(_painter->font()));
                    } else {
                        const int distanceBetweenSceneNumberAndText = 10;
                        dialogueNumberRect
                            = QRectF(MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                         + _body.width() + distanceBetweenSceneNumberAndText,
                                     _blockRect.top(),
                                     MeasurementHelper::mmToPx(exportTemplate.pageMargins().right())
                                         - distanceBetweenSceneNumberAndText,
                                     TextHelper::fineLineSpacing(_painter->font()));
                    }
                }
                _painter->drawText(dialogueNumberRect, Qt::AlignRight | Qt::AlignVCenter,
                                   dialogueNumber);
            }
        }
    }

    //
    // Прорисовка заголовков блоков
    //
    const auto& paragraphStyle = exportTemplate.paragraphStyle(_paragraphType);
    if (!_block.text().isEmpty() && paragraphStyle.isTitleVisible()) {
        _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());

        //
        // Определим область для отрисовки (отступы используем от стиля персонажа)
        //
        const auto& characterBlockStyle
            = exportTemplate.paragraphStyle(TextParagraphType::Character);
        QRectF titleRect;
        if (QLocale().textDirection() == Qt::LeftToRight) {
            titleRect = QRectF(
                MeasurementHelper::mmToPx(exportTemplate.pageMargins().left()
                                          + characterBlockStyle.marginsOnHalfPage().left()),
                _blockRect.top(),
                _block.blockFormat().leftMargin()
                    - MeasurementHelper::mmToPx(characterBlockStyle.marginsOnHalfPage().left()),
                TextHelper::fineLineSpacing(_painter->font()));
        } else {
            //
            // FIXME: RTL
            //
        }

        QString space;
        space.fill(' ', 100);
        _painter->drawText(
            titleRect, Qt::AlignLeft | Qt::AlignTop,
            QString("%1:%2").arg(!paragraphStyle.title().isEmpty()
                                     ? paragraphStyle.title()
                                     : BusinessLayer::textParagraphTitle(_paragraphType),
                                 space));
    }

    //
    // Рисуем звёздочки ревизий
    //
    if (!_block.text().isEmpty() && exportOptions.includeReviewMarks) {
        //
        // Собираем ревизии для отображения
        //
        QVector<QPair<QRectF, QColor>> revisionMarks;
        for (const auto& format : _block.textFormats()) {
            if (const auto revision
                = format.format.property(TextBlockStyle::PropertyCommentsIsRevision).toStringList();
                !revision.isEmpty() && revision.constFirst() == "true") {
                int position = format.start;
                do {
                    if (position != format.start) {
                        ++position;
                    }

                    const auto line = _block.layout()->lineForTextPosition(position);
                    const auto linePos = line.position() + QPointF(0, _blockRect.top());
                    const QRectF rect(
                        _body.width()
                            - MeasurementHelper::mmToPx(exportTemplate.pageMargins().right()),
                        linePos.y() <= _pageYPos
                            ? (_pageYPos
                               + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                            : linePos.y(),
                        MeasurementHelper::mmToPx(exportTemplate.pageMargins().right()),
                        line.height());
                    const auto revisionColor = format.format.foreground().color();
                    //
                    // ... первая звёздочка, или звёздочка на следующей строке
                    //
                    if (revisionMarks.isEmpty() || revisionMarks.constLast().first != rect) {
                        revisionMarks.append({ rect, revisionColor });
                    }
                    //
                    // ... звёздочка на той же строке - проверяем уровень
                    //
                    else if (ColorHelper::revisionLevel(revisionMarks.constLast().second)
                             < ColorHelper::revisionLevel(revisionColor)) {
                        revisionMarks.last().second = revisionColor;
                    }
                    position = line.textStart() + line.textLength();
                } while (position < (format.start + format.length) && position != _block.length());
            }
        }

        _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());
        for (const auto& reviewMark : std::as_const(revisionMarks)) {
            _painter->setPen(reviewMark.second);
            _painter->drawText(reviewMark.first, Qt::AlignCenter, "*");
        }
    }
}

} // namespace BusinessLayer

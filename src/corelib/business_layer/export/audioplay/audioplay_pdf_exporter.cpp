#include "audioplay_pdf_exporter.h"

#include "audioplay_export_options.h"

#include <business_layer/document/audioplay/text/audioplay_text_corrector.h>
#include <business_layer/document/audioplay/text/audioplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/model/audioplay/audioplay_information_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_scene_item.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_text_item.h>
#include <business_layer/templates/audioplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <ui/widgets/text_edit/page/page_text_edit.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QAbstractTextDocumentLayout>
#include <QApplication>
#include <QLocale>
#include <QPainter>
#include <QPdfWriter>
#include <QTextBlock>
#include <QtMath>

#include <cmath>


namespace BusinessLayer {

AudioplayPdfExporter::AudioplayPdfExporter()
    : AudioplayExporter()
    , AbstractPdfExporter()
{
}

void AudioplayPdfExporter::updateExportOptions(TextModel* _model,
                                               ExportOptions& _exportOptions) const
{
    const auto audioplayModel = qobject_cast<AudioplayTextModel*>(_model);
    Q_ASSERT(audioplayModel);

    _exportOptions.header = audioplayModel->informationModel()->header();
    _exportOptions.footer = audioplayModel->informationModel()->footer();
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

        const auto blockData = dynamic_cast<TextBlockData*>(_block.userData());
        if (blockData != nullptr) {
            QFont font = _block.charFormat().font();
            font.setBold(false);
            font.setUnderline(false);
            _painter->setFont(font);
            //
            const auto textItem = static_cast<AudioplayTextModelTextItem*>(blockData->item());
            if (textItem && textItem->number().has_value()) {
                const QString dialogueNumber = textItem->number()->text;
                const int numberDelta = _painter->fontMetrics().horizontalAdvance(dialogueNumber);
                const auto characterBlockStyle
                    = exportTemplate.paragraphStyle(TextParagraphType::Character);
                QRectF dialogueNumberRect;
                if (QLocale().textDirection() == Qt::LeftToRight) {
                    if (characterBlockStyle.marginsOnHalfPage().left() > 0) {
                        dialogueNumberRect = QRectF(
                            MeasurementHelper::mmToPx(exportTemplate.pageMargins().left()),
                            _blockRect.top() <= _pageYPos
                                ? (_pageYPos
                                   + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                : _blockRect.top(),
                            numberDelta, _blockRect.height());
                    } else {
                        const int distanceBetweenSceneNumberAndText = 10;
                        dialogueNumberRect = QRectF(
                            0,
                            _blockRect.top() <= _pageYPos
                                ? (_pageYPos
                                   + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                : _blockRect.top(),
                            MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                - distanceBetweenSceneNumberAndText,
                            _blockRect.height());
                    }
                } else {
                    if (_block.blockFormat().rightMargin() > numberDelta) {
                        dialogueNumberRect = QRectF(
                            MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                + _body.width() - numberDelta,
                            _blockRect.top() <= _pageYPos
                                ? (_pageYPos
                                   + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                : _blockRect.top(),
                            numberDelta, _blockRect.height());
                    } else {
                        const int distanceBetweenSceneNumberAndText = 10;
                        dialogueNumberRect = QRectF(
                            MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                + _body.width() + distanceBetweenSceneNumberAndText,
                            _blockRect.top() <= _pageYPos
                                ? (_pageYPos
                                   + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                : _blockRect.top(),
                            MeasurementHelper::mmToPx(exportTemplate.pageMargins().right())
                                - distanceBetweenSceneNumberAndText,
                            _blockRect.height());
                    }
                }
                _painter->drawText(dialogueNumberRect, Qt::AlignRight | Qt::AlignTop,
                                   dialogueNumber);
            }
        }
    }

    //
    // Прорисовка заголовков блоков
    //
    const auto paragraphStyle = exportTemplate.paragraphStyle(_paragraphType);
    if (!_block.text().isEmpty() && paragraphStyle.isTitleVisible()) {
        _painter->setFont(_block.charFormat().font());

        //
        // Определим область для отрисовки (отступы используем от стиля персонажа)
        //
        const auto characterBlockStyle
            = exportTemplate.paragraphStyle(TextParagraphType::Character);
        QRectF dialogueNumberRect;
        if (QLocale().textDirection() == Qt::LeftToRight) {
            dialogueNumberRect = QRectF(
                MeasurementHelper::mmToPx(exportTemplate.pageMargins().left()
                                          + characterBlockStyle.marginsOnHalfPage().left()),
                _blockRect.top() <= _pageYPos
                    ? (_pageYPos + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                    : _blockRect.top(),
                _block.blockFormat().leftMargin()
                    - MeasurementHelper::mmToPx(characterBlockStyle.marginsOnHalfPage().left()),
                _blockRect.height());
        } else {
            //
            // FIXME: RTL
            //
        }

        QString space;
        space.fill(' ', 100);
        _painter->drawText(
            dialogueNumberRect, Qt::AlignLeft | Qt::AlignTop,
            QString("%1:%2").arg(!paragraphStyle.title().isEmpty()
                                     ? paragraphStyle.title()
                                     : BusinessLayer::textParagraphTitle(_paragraphType),
                                 space));
    }
}

} // namespace BusinessLayer

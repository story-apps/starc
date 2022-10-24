#include "screenplay_pdf_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/screenplay/text/screenplay_text_corrector.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QPainter>


namespace BusinessLayer {

ScreenplayPdfExporter::ScreenplayPdfExporter()
    : ScreenplayExporter()
    , AbstractPdfExporter()
{
}

void ScreenplayPdfExporter::updateExportOptions(TextModel* _model,
                                                ExportOptions& _exportOptions) const
{
    const auto textModel = qobject_cast<ScreenplayTextModel*>(_model);
    Q_ASSERT(textModel);

    _exportOptions.header = textModel->informationModel()->header();
    _exportOptions.footer = textModel->informationModel()->footer();
}

void ScreenplayPdfExporter::printBlockDecorations(
    QPainter* _painter, qreal _pageYPos, const QRectF& _body, TextParagraphType _paragraphType,
    const QRectF& _blockRect, const QTextBlock& _block, const ExportOptions& _exportOptions) const
{
    if (!_block.isVisible()) {
        return;
    }

    const auto& exportTemplate = documentTemplate(_exportOptions);
    const auto& exportOptions = static_cast<const ScreenplayExportOptions&>(_exportOptions);

    //
    // Покажем номер сцены, если необходимо
    //
    if (_paragraphType == TextParagraphType::SceneHeading && !_block.text().isEmpty()
        && exportOptions.showScenesNumbers) {
        const auto blockData = static_cast<TextBlockData*>(_block.userData());
        if (blockData != nullptr) {
            _painter->setFont(_block.charFormat().font());
            //
            const auto sceneItem
                = static_cast<ScreenplayTextModelSceneItem*>(blockData->item()->parent());
            const int distanceBetweenSceneNumberAndText = 10;

            if (exportOptions.showScenesNumbersOnLeft) {
                const QRectF leftSceneNumberRect(
                    0,
                    _blockRect.top() <= _pageYPos ? (
                        _pageYPos + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                                  : _blockRect.top(),
                    MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                        - distanceBetweenSceneNumberAndText,
                    _blockRect.height());
                _painter->drawText(leftSceneNumberRect, Qt::AlignRight | Qt::AlignTop,
                                   sceneItem->number()->text);
            }

            if (exportOptions.showScenesNumbersOnRight) {
                const QRectF rightSceneNumberRect(
                    _body.width() - MeasurementHelper::mmToPx(exportTemplate.pageMargins().right())
                        + distanceBetweenSceneNumberAndText,
                    _blockRect.top() <= _pageYPos ? (
                        _pageYPos + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                                  : _blockRect.top(),
                    MeasurementHelper::mmToPx(exportTemplate.pageMargins().right())
                        - distanceBetweenSceneNumberAndText,
                    _blockRect.height());
                _painter->drawText(rightSceneNumberRect, Qt::AlignLeft | Qt::AlignTop,
                                   sceneItem->number()->text);
            }
        }
    }
    //
    // Печатаем номер диалога, если необходимо, а также автоматические (ПРОД)
    //
    else if (_paragraphType == TextParagraphType::Character && !_block.text().isEmpty()) {
        //
        // Номера реплик
        //
        if (exportOptions.showDialoguesNumbers) {
            const auto blockData = static_cast<TextBlockData*>(_block.userData());
            if (blockData != nullptr) {
                _painter->setFont(_block.charFormat().font());
                //
                const auto textItem = static_cast<ScreenplayTextModelTextItem*>(blockData->item());
                if (textItem && textItem->number().has_value()) {
                    const QString dialogueNumber = textItem->number()->text;
                    const int numberDelta
                        = TextHelper::fineTextWidthF(dialogueNumber, _painter->font());
                    QRectF dialogueNumberRect;
                    if (QLocale().textDirection() == Qt::LeftToRight) {
                        if (_block.blockFormat().leftMargin() > numberDelta) {
                            dialogueNumberRect = QRectF(
                                MeasurementHelper::mmToPx(exportTemplate.pageMargins().left()),
                                _blockRect.top() <= _pageYPos ? (
                                    _pageYPos
                                    + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                                              : _blockRect.top(),
                                numberDelta, _blockRect.height());
                        } else {
                            const int distanceBetweenSceneNumberAndText = 10;
                            dialogueNumberRect = QRectF(
                                0,
                                _blockRect.top() <= _pageYPos ? (
                                    _pageYPos
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
                                _blockRect.top() <= _pageYPos ? (
                                    _pageYPos
                                    + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                                              : _blockRect.top(),
                                numberDelta, _blockRect.height());
                        } else {
                            const int distanceBetweenSceneNumberAndText = 10;
                            dialogueNumberRect = QRectF(
                                MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                    + _body.width() + distanceBetweenSceneNumberAndText,
                                _blockRect.top() <= _pageYPos ? (
                                    _pageYPos
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
        // Автоматические (ПРОД)
        //
        if (_block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCharacterContinued)
            && !_block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
            _painter->setFont(_block.charFormat().font());

            const auto continuedTerm = BusinessLayer::ScreenplayTextCorrector::continuedTerm();

            //
            // Почему-то если взять просто ширину последней строки текста, то получается
            // слишком широко в некоторых случаях, так, что постфикс рисуется очень далеко
            // от текста. Поэтому решил брать текст последней строки, добавлять к нему
            // постфикс, считать их совместную ширину и брать её, как конечную точку
            //
            const auto lastLineText
                = TextHelper::lastLineText(_block.text(), _painter->font(), _blockRect.width())
                + continuedTerm;
            const auto correctedBlockRect
                = QRectF({ _block.blockFormat().leftMargin()
                               + MeasurementHelper::mmToPx(exportTemplate.pageMargins().left()),
                           _blockRect.top() },
                         _blockRect.size());
            const QPoint bottomRight
                = QPoint(_blockRect.left() + _block.blockFormat().leftMargin()
                             + TextHelper::fineTextWidthF(lastLineText, _painter->font()),
                         correctedBlockRect.bottom());
            const QPoint topLeft = QPoint(
                bottomRight.x() - TextHelper::fineTextWidthF(continuedTerm, _painter->font()),
                correctedBlockRect.bottom()
                    - _painter->fontMetrics().boundingRect(continuedTerm).height());
            const QRect postfixRect(topLeft, bottomRight);
            _painter->drawText(postfixRect, Qt::AlignLeft | Qt::AlignBottom, continuedTerm);
        }
    }

    //
    // Прорисовка префикса/постфикса для блока текста, если это не пустая декорация
    //
    if (!_block.text().isEmpty()
        || !_block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
        const auto correctedBlockRect
            = QRectF({ _block.blockFormat().leftMargin()
                           + MeasurementHelper::mmToPx(exportTemplate.pageMargins().left()),
                       _blockRect.top() },
                     _blockRect.size());
        //
        // ... префикс
        //
        if (_block.charFormat().hasProperty(TextBlockStyle::PropertyPrefix)) {
            _painter->setFont(_block.charFormat().font());

            const auto prefix = _block.charFormat().stringProperty(TextBlockStyle::PropertyPrefix);
            auto prefixRect = _blockRect;
            prefixRect.setWidth(TextHelper::fineTextWidthF(prefix, _painter->font()));
            prefixRect.moveLeft(prefixRect.left() + _block.blockFormat().leftMargin()
                                - prefixRect.width());
            prefixRect.setHeight(_painter->fontMetrics().boundingRect(prefix).height());
            _painter->drawText(prefixRect, Qt::AlignLeft | Qt::AlignBottom, prefix);
        }
        //
        // ... постфикс
        //
        if (_block.charFormat().hasProperty(TextBlockStyle::PropertyPostfix)) {
            _painter->setFont(_block.charFormat().font());

            const auto postfix
                = _block.charFormat().stringProperty(TextBlockStyle::PropertyPostfix);

            //
            // Почему-то если взять просто ширину последней строки текста, то получается
            // слишком широко в некоторых случаях, так, что постфикс рисуется очень далеко
            // от текста. Поэтому решил брать текст последней строки, добавлять к нему
            // постфикс, считать их совместную ширину и брать её, как конечную точку
            //
            const auto lastLineText
                = TextHelper::lastLineText(_block.text(), _painter->font(), _blockRect.width())
                + postfix;
            const QPoint bottomRight
                = QPoint(_blockRect.left() + _block.blockFormat().leftMargin()
                             + TextHelper::fineTextWidthF(lastLineText, _painter->font()),
                         correctedBlockRect.bottom());
            const QPoint topLeft
                = QPoint(bottomRight.x() - TextHelper::fineTextWidthF(postfix, _painter->font()),
                         correctedBlockRect.bottom()
                             - _painter->fontMetrics().boundingRect(postfix).height());
            const QRect postfixRect(topLeft, bottomRight);
            _painter->drawText(postfixRect, Qt::AlignLeft | Qt::AlignBottom, postfix);
        }
    }
}

} // namespace BusinessLayer

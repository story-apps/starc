#include "stageplay_pdf_exporter.h"

#include "stageplay_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/stageplay/stageplay_information_model.h>
#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QLocale>
#include <QPainter>


namespace BusinessLayer {

StageplayPdfExporter::StageplayPdfExporter()
    : StageplayExporter()
    , AbstractPdfExporter()
{
}

void StageplayPdfExporter::updateExportOptions(AbstractModel* _model,
                                               ExportOptions& _exportOptions) const
{
    const auto textModel = qobject_cast<StageplayTextModel*>(_model);
    Q_ASSERT(textModel);

    _exportOptions.header = textModel->informationModel()->header();
    _exportOptions.footer = textModel->informationModel()->footer();
}

void StageplayPdfExporter::printBlockDecorations(QPainter* _painter, qreal _pageYPos,
                                                 const QRectF& _body,
                                                 TextParagraphType _paragraphType,
                                                 const QRectF& _blockRect, const QTextBlock& _block,
                                                 const ExportOptions& _exportOptions) const
{
    const auto& exportTemplate = documentTemplate(_exportOptions);
    const auto& exportOptions = static_cast<const StageplayExportOptions&>(_exportOptions);

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
            const auto textItem = static_cast<TextModelTextItem*>(blockData->item());
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
                                     _blockRect.top(), numberDelta, _blockRect.height());
                    } else {
                        const int distanceBetweenSceneNumberAndText = 10;
                        dialogueNumberRect
                            = QRectF(0, _blockRect.top(),
                                     MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                         - distanceBetweenSceneNumberAndText,
                                     _blockRect.height());
                    }
                } else {
                    if (_block.blockFormat().rightMargin() > numberDelta) {
                        dialogueNumberRect
                            = QRectF(MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                         + _body.width() - numberDelta,
                                     _blockRect.top(), numberDelta, _blockRect.height());
                    } else {
                        const int distanceBetweenSceneNumberAndText = 10;
                        dialogueNumberRect
                            = QRectF(MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                                         + _body.width() + distanceBetweenSceneNumberAndText,
                                     _blockRect.top(),
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
    // Прорисовка префикса/постфикса для блока текста, если это не пустая декорация
    //
    if (!_block.text().isEmpty()
        || !_block.blockFormat().boolProperty(TextBlockStyle::PropertyIsCorrection)) {
        //
        // ... вспомогательные функции для вычислении ширины заданной строки блока
        //
        auto lineWidth = [_block, _painter](int _lineIndex) {
            auto lineText = _block.text().mid(_block.layout()->lineAt(_lineIndex).textStart(),
                                              _block.layout()->lineAt(_lineIndex).textLength());
            if (_lineIndex < _block.layout()->lineCount() - 1) {
                lineText = lineText.trimmed();
            }
            return TextHelper::fineTextWidthF(lineText, _painter->font());
        };
        auto firstLineWidth = [&lineWidth] { return lineWidth(0); };
        auto lastLineWidth = [&lineWidth, lastLineIndex = _block.layout()->lineCount() - 1] {
            return lineWidth(lastLineIndex);
        };

        //
        // ... префикс
        //
        if (_block.charFormat().hasProperty(TextBlockStyle::PropertyPrefix)) {
            //
            // Настроим шрифт рисовальщика, он будет использоваться для вычисления ширины строк
            //
            _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());

            //
            // Определим параметры блока, необходимые для вычисления положения префикса
            //
            const qreal blockTextIndent = _block.blockFormat().textIndent();
            //
            // ... если в блоке всего одна строка, то красная строка выключена из геомерии блока,
            //     а значит её не нужно учитывать при вычислении пустой области образованной
            //     вследствии выравнивания текста внутри блока
            //
            const qreal alignmentDeltaBlockTextIndent
                = _block.layout()->lineCount() == 1 ? 0.0 : blockTextIndent;
            //
            // ... вычислим пустую область образованной из-за выравнивания текста внутри блока
            //     если выравнивание по левому краю, то её нет
            //
            qreal alignmentDelta = 0;
            if (_block.blockFormat().alignment().testFlag(Qt::AlignHCenter)) {
                //
                // ... если выравнивание по середине, то пустая область равномерно распологается с
                //     обеих сторон вокруг текста
                //
                alignmentDelta
                    += (_blockRect.width() - alignmentDeltaBlockTextIndent - firstLineWidth())
                    / 2.0;
            } else if (_block.blockFormat().alignment().testFlag(Qt::AlignRight)) {
                //
                // ... если выравнивание по правому краю, то пустая область распологается с
                //     левой стороны текста
                //
                alignmentDelta
                    += (_blockRect.width() - alignmentDeltaBlockTextIndent - firstLineWidth());
            }

            //
            // Определим сам префикс и область, в которой его нужно нарисовать
            //
            const auto prefix = _block.charFormat().stringProperty(TextBlockStyle::PropertyPrefix);
            const auto prefixWidth = TextHelper::fineTextWidthF(prefix, _painter->font());
            const auto prefixHeight = _painter->fontMetrics().boundingRect(prefix).height();
            const auto prefixLeft = _blockRect.left() // положение блока
                + (_block.layout()->lineCount() == 1 ? 0.0 : blockTextIndent) // красная строка
                + alignmentDelta // пустая область из-за выравнивания
                - prefixWidth; // ширина префикса
            const auto prefixTop = _blockRect.top();
            const QRectF prefixRect(QPointF(prefixLeft, prefixTop),
                                    QSizeF(prefixWidth, prefixHeight));

            //
            // Собственно рисуем декорацию
            //
            _painter->drawText(prefixRect, Qt::AlignLeft | Qt::AlignBottom, prefix);
        }
        //
        // ... постфикс
        //
        if (_block.charFormat().hasProperty(TextBlockStyle::PropertyPostfix)) {
            //
            // Настроим шрифт рисовальщика, он будет использоваться для вычисления ширины строк
            //
            _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());

            //
            // Определим параметры блока, необходимые для вычисления положения постфикса
            //
            // ... вычислим пустую область образованной из-за выравнивания текста внутри блока
            //     если выравнивание по левому краю, то её нет
            //
            qreal alignmentDelta = 0;
            if (_block.blockFormat().alignment().testFlag(Qt::AlignHCenter)) {
                //
                // ... если выравнивание по середине, то пустая область равномерно распологается с
                //     обеих сторон вокруг текста
                //
                alignmentDelta += (_blockRect.width() - lastLineWidth()) / 2.0;
            } else if (_block.blockFormat().alignment().testFlag(Qt::AlignRight)) {
                //
                // ... если выравнивание по правому краю, то пустая область распологается с
                //     левой стороны текста
                //
                alignmentDelta += (_blockRect.width() - lastLineWidth());
            }

            //
            // Определим сам постфикс и область, в которой его нужно нарисовать
            //
            const auto postfix
                = _block.charFormat().stringProperty(TextBlockStyle::PropertyPostfix);
            const auto postfixWidth = TextHelper::fineTextWidthF(postfix, _painter->font());
            const auto postfixHeight = _painter->fontMetrics().boundingRect(postfix).height();
            const auto postfixLeft = _blockRect.left() // положение блока
                + 0.0 // красная строка (не учитываем)
                + alignmentDelta // пустая область из-за выравнивания
                + lastLineWidth(); // ширина текста
            const auto postfixTop = _blockRect.bottom() - postfixHeight;
            const QRectF postfixRect(QPointF(postfixLeft, postfixTop),
                                     QSizeF(postfixWidth, postfixHeight));

            //
            // Собственно рисуем декорацию
            //
            _painter->drawText(postfixRect, Qt::AlignLeft | Qt::AlignBottom, postfix);
        }
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

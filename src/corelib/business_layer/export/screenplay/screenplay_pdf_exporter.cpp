#include "screenplay_pdf_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/screenplay/text/screenplay_text_corrector.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/color_helper.h>
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

void ScreenplayPdfExporter::updateExportOptions(AbstractModel* _model,
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
    const auto firstBlockYDelta =
#ifdef Q_OS_WINDOWS
        6
#else
        9
#endif
        ;
    QRectF blockRect = _blockRect;
    //    if (exportTemplate.paragraphStyle(_paragraphType).lineSpacingType()
    //        != TextBlockStyle::LineSpacingType::SingleLineSpacing) {
    auto l1 = _block.blockFormat().lineHeight();
    auto l2 = _block.layout()->lineAt(0).height();
    auto delta = l1 - l2;
    blockRect.moveTop(blockRect.top() + _block.blockFormat().lineHeight()
                      - _block.layout()->lineAt(0).height());
    //    }

    //
    // Покажем номер сцены, если необходимо
    //
    if (_paragraphType == TextParagraphType::SceneHeading && !_block.text().isEmpty()
        && exportOptions.showScenesNumbers) {
        const auto blockData = static_cast<TextBlockData*>(_block.userData());
        if (blockData != nullptr && blockData->item() != nullptr
            && blockData->item()->parent() != nullptr
            && blockData->item()->parent()->type() == TextModelItemType::Group
            && static_cast<TextGroupType>(blockData->item()->parent()->subtype())
                == TextGroupType::Scene) {
            _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());
            //
            const auto sceneItem
                = static_cast<ScreenplayTextModelSceneItem*>(blockData->item()->parent());
            const int distanceBetweenSceneNumberAndText = 10;

            if (exportOptions.showScenesNumbersOnLeft) {
                const QRectF leftSceneNumberRect(
                    0,
                    blockRect.top() <= _pageYPos
                        ? (_pageYPos + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top())
                           + delta)
                        : blockRect.top(),
                    MeasurementHelper::mmToPx(exportTemplate.pageMargins().left())
                        - distanceBetweenSceneNumberAndText,
                    blockRect.height());
                _painter->drawText(leftSceneNumberRect, Qt::AlignRight | Qt::AlignTop,
                                   sceneItem->number()->text);
            }

            if (exportOptions.showScenesNumbersOnRight) {
                const QRectF rightSceneNumberRect(
                    _body.width() - MeasurementHelper::mmToPx(exportTemplate.pageMargins().right())
                        + distanceBetweenSceneNumberAndText,
                    blockRect.top() <= _pageYPos ? (
                        _pageYPos + MeasurementHelper::mmToPx(exportTemplate.pageMargins().top()))
                                                 : blockRect.top(),
                    MeasurementHelper::mmToPx(exportTemplate.pageMargins().right())
                        - distanceBetweenSceneNumberAndText,
                    blockRect.height());
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
                _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());
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
            _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());

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
        // RTL
        //
        // TODO: Переделать из новой схемы в LTR
        //
        if (_block.text().isRightToLeft()) {
            //
            // ... префикс
            //
            if (_block.charFormat().hasProperty(TextBlockStyle::PropertyPrefix)) {
                _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());

                const auto prefix
                    = _block.charFormat().stringProperty(TextBlockStyle::PropertyPrefix);

                //
                // Почему-то если взять просто ширину последней строки текста, то получается
                // слишком широко в некоторых случаях, так, что постфикс рисуется очень далеко
                // от текста. Поэтому решил брать текст последней строки, добавлять к нему
                // постфикс, считать их совместную ширину и брать её, как конечную точку
                //
                const auto lastLineText
                    = TextHelper::lastLineText(_block.text(), _painter->font(), _blockRect.width());
                const QPoint bottomRight = QPoint(
                    _blockRect.left() + _block.blockFormat().leftMargin() + _blockRect.width()
                        - TextHelper::fineTextWidthF(lastLineText, _painter->font()),
                    correctedBlockRect.bottom());
                const QPoint topLeft
                    = QPoint(bottomRight.x() - TextHelper::fineTextWidthF(prefix, _painter->font()),
                             correctedBlockRect.bottom()
                                 - _painter->fontMetrics().boundingRect(prefix).height());
                const QRect postfixRect(topLeft, bottomRight);
                _painter->drawText(postfixRect, Qt::AlignLeft | Qt::AlignBottom, prefix);
            }
            //
            // ... постфикс
            //
            if (_block.charFormat().hasProperty(TextBlockStyle::PropertyPostfix)) {
                _painter->setFont(TextHelper::fineBlockCharFormat(_block).font());

                const auto postfix
                    = _block.charFormat().stringProperty(TextBlockStyle::PropertyPostfix);
                auto rect = _blockRect;
                rect.setWidth(TextHelper::fineTextWidthF(postfix, _painter->font()));
                rect.moveLeft(rect.left() + _block.blockFormat().leftMargin() + _blockRect.width());
                rect.setHeight(_painter->fontMetrics().boundingRect(postfix).height());
                _painter->drawText(rect, Qt::AlignLeft | Qt::AlignBottom, postfix);
            }
        }
        //
        // LTR
        //
        else {
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
            auto ___bt = _block.text();

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
                // ... если в блоке всего одна строка, то красная строка выключена из геомерии
                // блока,
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
                    // ... если выравнивание по середине, то пустая область равномерно распологается
                    // с
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
                const auto prefix
                    = _block.charFormat().stringProperty(TextBlockStyle::PropertyPrefix);
                const auto prefixWidth = TextHelper::fineTextWidthF(prefix, _painter->font());
                const auto prefixHeight = _painter->fontMetrics().boundingRect(prefix).height();
                const auto prefixLeft = _blockRect.left() // положение блока
                    + _block.blockFormat().leftMargin() // левый отступ блока
                    + blockTextIndent // красная строка
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
                const qreal blockTextIndent = _block.blockFormat().textIndent();
                //
                // ... вычислим пустую область образованной из-за выравнивания текста внутри блока
                //     если выравнивание по левому краю, то её нет
                //
                qreal alignmentDelta = 0;
                if (_block.blockFormat().alignment().testFlag(Qt::AlignHCenter)) {
                    //
                    // ... если выравнивание по середине, то пустая область равномерно распологается
                    // с
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
                    + _block.blockFormat().leftMargin() // левый отступ блока
                    + (_block.layout()->lineCount() == 1 ? blockTextIndent : 0.0) // красная строка
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
                    const auto linePos = line.position() + QPointF(0, blockRect.top());
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

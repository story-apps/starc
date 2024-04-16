#include "screenplay_docx_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/measurement_helper.h>
#include <utils/helpers/text_helper.h>

#include <QFontMetricsF>


namespace BusinessLayer {

ScreenplayDocxExporter::ScreenplayDocxExporter()
    : ScreenplayExporter()
    , AbstractDocxExporter()
{
}

QVector<TextParagraphType> ScreenplayDocxExporter::paragraphTypes() const
{
    return {
        TextParagraphType::Undefined,
        TextParagraphType::UnformattedText,
        TextParagraphType::InlineNote,
        TextParagraphType::ActHeading,
        TextParagraphType::ActFooter,
        TextParagraphType::SequenceHeading,
        TextParagraphType::SequenceFooter,
        TextParagraphType::SceneHeading,
        TextParagraphType::SceneCharacters,
        TextParagraphType::BeatHeading,
        TextParagraphType::Action,
        TextParagraphType::Character,
        TextParagraphType::Parenthetical,
        TextParagraphType::Dialogue,
        TextParagraphType::Lyrics,
        TextParagraphType::Transition,
        TextParagraphType::Shot,
    };
}

void ScreenplayDocxExporter::processBlock(const TextCursor& _cursor,
                                          const ExportOptions& _exportOptions,
                                          QString& _documentXml) const
{
    const auto currentBlockType = TextBlockStyle::forBlock(_cursor);
    const auto block = _cursor.block();
    const auto& exportOptions = static_cast<const ScreenplayExportOptions&>(_exportOptions);

    //
    // ... если необходимо, добавляем номер сцены
    //
    if (currentBlockType == TextParagraphType::SceneHeading && exportOptions.showScenesNumbers) {
        const auto blockData = static_cast<TextBlockData*>(block.userData());
        if (blockData != nullptr && blockData->item()->parent() != nullptr
            && blockData->item()->parent()->type() == TextModelItemType::Group
            && static_cast<TextGroupType>(blockData->item()->parent()->subtype())
                == TextGroupType::Scene) {
            const auto sceneItem
                = static_cast<ScreenplayTextModelSceneItem*>(blockData->item()->parent());
            const auto sceneNumber = sceneItem->number()->text + " ";
            const auto leftMargin = block.textDirection() == Qt::LeftToRight
                ? block.blockFormat().leftMargin()
                : block.blockFormat().rightMargin();
            const auto rightMargin = block.textDirection() == Qt::LeftToRight
                ? block.blockFormat().rightMargin()
                : block.blockFormat().leftMargin();
            _documentXml.append(QString("<w:ind w:left=\"%1\" w:right=\"%2\" w:hanging=\"%3\" />")
                                    .arg(MeasurementHelper::pxToTwips(leftMargin))
                                    .arg(MeasurementHelper::pxToTwips(rightMargin))
                                    .arg(MeasurementHelper::pxToTwips(TextHelper::fineTextWidthF(
                                        sceneNumber, block.charFormat().font()))));

            auto cursor = _cursor;
            cursor.setPosition(block.position());
            cursor.insertText(sceneNumber, block.charFormat());
        }
    }
    //
    // ... для ремарки подхачиваем отступ перед блоком и ширину самого блока
    //
    else if (currentBlockType == TextParagraphType::Parenthetical && !block.text().isEmpty()) {
        const QLatin1String prefix("(");
        const QLatin1String postfix(")");
        const auto leftMargin = block.textDirection() == Qt::LeftToRight
            ? block.blockFormat().leftMargin()
            : block.blockFormat().rightMargin();
        const auto rightMargin = block.textDirection() == Qt::LeftToRight
            ? block.blockFormat().rightMargin()
            : block.blockFormat().leftMargin();
        _documentXml.append(
            QString("<w:ind w:left=\"%1\" w:right=\"%2\" w:hanging=\"%3\" />")
                .arg(MeasurementHelper::pxToTwips(leftMargin))
                .arg(MeasurementHelper::pxToTwips(
                    rightMargin - TextHelper::fineTextWidthF(postfix, block.charFormat().font())))
                .arg(MeasurementHelper::pxToTwips(
                    TextHelper::fineTextWidthF(prefix, block.charFormat().font()))));

        auto cursor = _cursor;
        cursor.setPosition(block.position());
        cursor.insertText(prefix, block.charFormat());
        cursor.movePosition(TextCursor::EndOfBlock);
        cursor.insertText(postfix, block.charFormat());
    }
}

} // namespace BusinessLayer

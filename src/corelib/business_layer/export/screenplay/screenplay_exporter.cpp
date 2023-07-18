#include "screenplay_exporter.h"

#include "screenplay_export_options.h"

#include <business_layer/document/screenplay/text/screenplay_text_document.h>
#include <business_layer/document/text/text_block_data.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/model/screenplay/text/screenplay_text_block_parser.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <business_layer/templates/screenplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <utils/helpers/color_helper.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

TextDocument* ScreenplayExporter::createDocument(const ExportOptions& _exportOptions) const
{
    const auto& exportOptions = static_cast<const ScreenplayExportOptions&>(_exportOptions);

    auto document = new ScreenplayTextDocument;
    document->setCorrectionOptions(
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorContinueDialogueKey).toBool(),
        settingsValue(DataStorageLayer::kComponentsScreenplayEditorCorrectTextOnPageBreaksKey)
            .toBool());
    document->setTreatmentDocument(exportOptions.includeTreatment);
    return document;
}

const TextTemplate& ScreenplayExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    return TemplatesFacade::screenplayTemplate(_exportOptions.templateId);
}

bool ScreenplayExporter::prepareBlock(const ExportOptions& _exportOptions,
                                      TextCursor& _cursor) const
{
    const auto& exportOptions = static_cast<const ScreenplayExportOptions&>(_exportOptions);

    //
    // Скорем блок, если в нём нет необходимости
    //

    //
    // Если не нужно печатать, эту сцену, то удаляем её
    //
    if (!exportOptions.exportScenes.isEmpty()) {
        const auto blockData = static_cast<TextBlockData*>(_cursor.block().userData());
        bool needRemoveBlock = true;
        auto checkScene = [exportOptions, &needRemoveBlock](TextModelGroupItem* _item) {
            if (_item->groupType() == TextGroupType::Scene) {
                const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(_item);
                if (exportOptions.exportScenes.contains(sceneItem->number()->value)) {
                    needRemoveBlock = false;
                }
            }
        };
        if (blockData && blockData->item() && blockData->item()->parent()
            && blockData->item()->parent()->type() == TextModelItemType::Group) {

            const auto groupItem = static_cast<TextModelGroupItem*>(blockData->item()->parent());
            //
            // ... если этот блок и есть сцена, то проверим его
            //
            if (groupItem->groupType() == TextGroupType::Scene) {
                checkScene(groupItem);
            }
            //
            // ... в противном случае это бит, нужно проверить его родителя
            //
            else if (groupItem->parent()
                     && groupItem->parent()->type() == TextModelItemType::Group) {
                checkScene(static_cast<TextModelGroupItem*>(groupItem->parent()));
            }
        }
        if (needRemoveBlock) {
            _cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            if (_cursor.hasSelection()) {
                _cursor.deleteChar();
            }
            _cursor.movePosition(!_cursor.atEnd() ? QTextCursor::NextCharacter
                                                  : QTextCursor::PreviousCharacter,
                                 QTextCursor::KeepAnchor);
            _cursor.deleteChar();
            return true;
        }
    }

    //
    // Если нужно выделить реплики конкретного персонажа, добавим форматирование
    //
    if (_exportOptions.highlightCharacters) {
        QString currentCharacter;
        QColor currentCharacterColor;
        bool updateFormatting = false;
        switch (TextBlockStyle::forBlock(_cursor.block())) {
        case TextParagraphType::Character: {
            currentCharacter = ScreenplayCharacterParser::name(_cursor.block().text());
            currentCharacterColor = _exportOptions.highlightCharactersList.value(currentCharacter);
            updateFormatting = true;
            break;
        }

        case TextParagraphType::Parenthetical:
        case TextParagraphType::Dialogue:
        case TextParagraphType::Lyrics: {
            auto block = _cursor.block();
            while (block != block.document()->begin()) {
                if (TextBlockStyle::forBlock(block) == TextParagraphType::Character) {
                    currentCharacter = ScreenplayCharacterParser::name(block.text());
                    currentCharacterColor
                        = _exportOptions.highlightCharactersList.value(currentCharacter);
                    break;
                }

                block = block.previous();
            }
            updateFormatting = true;
            break;
        }

        default: {
            break;
        }
        }

        if (updateFormatting && currentCharacterColor.isValid()) {
            _cursor.movePosition(QTextCursor::StartOfBlock);
            _cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
            auto updateFormatting
                = [color = currentCharacterColor](const QTextCharFormat& _format) {
                      auto format = _format;
                      format.setBackground(color);
                      format.setForeground(ColorHelper::contrasted(color));
                      return format;
                  };
            TextHelper::updateSelectionFormatting(_cursor, updateFormatting);
        }
    }

    return false;
}
} // namespace BusinessLayer

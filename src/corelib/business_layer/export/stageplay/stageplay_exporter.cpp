#include "stageplay_exporter.h"

#include <business_layer/document/stageplay/text/stageplay_text_document.h>
#include <business_layer/document/text/text_cursor.h>
#include <business_layer/export/export_options.h>
#include <business_layer/model/stageplay/text/stageplay_text_block_parser.h>
#include <business_layer/templates/stageplay_template.h>
#include <business_layer/templates/templates_facade.h>
#include <utils/helpers/text_helper.h>

#include <QTextBlock>


namespace BusinessLayer {

TextDocument* StageplayExporter::createDocument(const ExportOptions& _exportOptions) const
{
    Q_UNUSED(_exportOptions)

    auto document = new StageplayTextDocument;
    document->setCorrectionOptions(true);
    return document;
}

const TextTemplate& StageplayExporter::documentTemplate(const ExportOptions& _exportOptions) const
{
    return TemplatesFacade::stageplayTemplate(_exportOptions.templateId);
}

bool StageplayExporter::prepareBlock(const ExportOptions& _exportOptions, TextCursor& _cursor) const
{
    //
    // Если нужно выделить реплики конкретного персонажа, добавим форматирование
    //
    if (_exportOptions.highlightCharacters) {
        QString currentCharacter;
        QColor currentCharacterColor;
        bool updateFormatting = false;
        switch (TextBlockStyle::forBlock(_cursor.block())) {
        case TextParagraphType::Character: {
            currentCharacter = StageplayCharacterParser::name(_cursor.block().text());
            currentCharacterColor = _exportOptions.highlightCharactersList.value(currentCharacter);
            updateFormatting = true;
            break;
        }

        case TextParagraphType::Parenthetical:
        case TextParagraphType::Dialogue:
        case TextParagraphType::Lyrics: {
            if (!_exportOptions.highlightCharactersWithDialogues) {
                break;
            }

            auto block = _cursor.block();
            while (block != block.document()->begin()) {
                if (TextBlockStyle::forBlock(block) == TextParagraphType::Character) {
                    currentCharacter = StageplayCharacterParser::name(block.text());
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
                      return format;
                  };
            TextHelper::updateSelectionFormatting(_cursor, updateFormatting);
        }
    }

    return false;
}
} // namespace BusinessLayer

#include "stageplay_fountain_exporter.h"

#include "stageplay_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/stageplay/text/stageplay_text_model_scene_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

StageplayFountainExporter::StageplayFountainExporter()
    : StageplayExporter()
    , AbstractMarkdownExporter({ '*', '_' })
{
}

StageplayFountainExporter::~StageplayFountainExporter() = default;


bool StageplayFountainExporter::processBlock(QString& _paragraph, const QTextBlock& _block,
                                             const ExportOptions& _exportOptions) const
{
    const auto& exportOptions = static_cast<const StageplayExportOptions&>(_exportOptions);

    switch (TextBlockStyle::forBlock(_block)) {
    case TextParagraphType::SceneHeading: {
        _paragraph.prepend('.');

        //
        // Если печатаем номера сцен, то добавим в конец этот номер, окруженный #
        //
        if (exportOptions.showBlockNumbers) {
            const auto blockData = static_cast<TextBlockData*>(_block.userData());
            if (blockData != nullptr && blockData->item()->parent() != nullptr
                && blockData->item()->parent()->type() == TextModelItemType::Group
                && static_cast<TextGroupType>(blockData->item()->parent()->subtype())
                    == TextGroupType::Scene) {
                const auto sceneItem
                    = static_cast<StageplayTextModelSceneItem*>(blockData->item()->parent());
                _paragraph += QString(" #%1#").arg(sceneItem->number()->text);
            }
        }
        return true;
    }

    case TextParagraphType::Character: {
        //
        // Если название персонажа не состоит из заглавных букв,
        // то необходимо добавить @ в начало
        //
        if (!TextHelper::isUppercase(_paragraph)) {
            _paragraph.prepend('@');
        }
        //
        // Если в конце имени персонажа стоит двоеточие, удалим его
        //
        if (_paragraph.endsWith(':')) {
            _paragraph.chop(1);
        }
        return true;
    }

    case TextParagraphType::Dialogue: {
        return true;
    }

    case TextParagraphType::Action: {
        //
        // Если действие в верхнем регистре, поставим перед ним "!"
        //
        if (TextHelper::isUppercase(_paragraph)) {
            _paragraph.prepend("!");
        }
        return true;
    }

    case TextParagraphType::InlineNote: {
        //
        // Обернем в /* и */
        //
        _paragraph = "/* " + _paragraph + " */";
        return true;
    }

    case TextParagraphType::UnformattedText: {
        return true;
    }

    default: {
        //
        // Игнорируем неизвестные блоки
        //
        return false;
    }
    }
}

QString StageplayFountainExporter::formatSymbols(TextSelectionTypes _type) const
{
    switch (_type) {
    case TextSelectionTypes::Bold: {
        return "**";
    }
    case TextSelectionTypes::Italic: {
        return "*";
    }
    case TextSelectionTypes::Underline: {
        return "_";
    }
    default: {
        return "";
    }
    }
}

void StageplayFountainExporter::addIndentationAtBegin(QString& _paragraph,
                                                      TextParagraphType _previosBlockType,
                                                      TextParagraphType _currentBlockType) const
{
    //
    // Таблица количества переносов строк между различными типами блоков
    // столбцы - предыдущий тип, строки - текущий
    //
    const QVector<QVector<int>> countIndentation = {
        // Undefined, SceneHeading, Character, Dialogue, Action/InlineNote/UnformattedText
        { 0, 0, 0, 0, 0 }, // Undefined
        { 0, 2, 3, 3, 3 }, // SceneHeading
        { 0, 2, 2, 2, 2 }, // Character
        { 0, 2, 1, 2, 2 }, // Dialogue
        { 0, 2, 2, 2, 2 }, // Action/InlineNote/UnformattedText
    };

    auto positionInTable = [](TextParagraphType _type) {
        switch (_type) {
        case TextParagraphType::SceneHeading: {
            return 1;
        }
        case TextParagraphType::Character: {
            return 2;
        }
        case TextParagraphType::Dialogue: {
            return 3;
        }
        case TextParagraphType::Action:
        case TextParagraphType::InlineNote:
        case TextParagraphType::UnformattedText: {
            return 4;
        }
        default: {
            return 0;
        }
        }
    };
    int column = positionInTable(_previosBlockType);
    int row = positionInTable(_currentBlockType);
    _paragraph.prepend(QString("\n").repeated(countIndentation[row][column]));
}

} // namespace BusinessLayer

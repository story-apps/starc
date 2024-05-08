#include "audioplay_fountain_exporter.h"

#include "audioplay_export_options.h"

#include <business_layer/document/text/text_block_data.h>
#include <business_layer/model/audioplay/text/audioplay_text_model_scene_item.h>
#include <business_layer/templates/text_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {


AudioplayFountainExporter::AudioplayFountainExporter()
    : AudioplayExporter()
    , AbstractMarkdownExporter({ '*', '_' })
{
}

AudioplayFountainExporter::~AudioplayFountainExporter() = default;

bool AudioplayFountainExporter::processBlock(QString& _paragraph, const QTextBlock& _block,
                                             const ExportOptions& _exportOptions) const
{
    const auto& exportOptions = static_cast<const AudioplayExportOptions&>(_exportOptions);

    //
    // Если нужны редакторские заметки, то вставляем их
    //
    if (exportOptions.includeReviewMarks) {
        QVector<QTextLayout::FormatRange> reviewMarks;
        for (const QTextLayout::FormatRange& format : _block.textFormats()) {
            if (format.format.boolProperty(TextBlockStyle::PropertyIsReviewMark)) {
                reviewMarks.push_back(format);
            }
        }
        for (int markIndex = reviewMarks.size() - 1; markIndex >= 0; --markIndex) {
            //
            // Извлечем список редакторских заметок для данной области блока
            //
            const QStringList comments = reviewMarks[markIndex]
                                             .format.property(TextBlockStyle::PropertyComments)
                                             .toStringList();
            //
            // Вставлять редакторские заметки нужно с конца, чтобы не сбилась их
            // позиция вставки
            //
            for (int commentIndex = comments.size() - 1; commentIndex >= 0; --commentIndex) {
                if (!comments[commentIndex].simplified().isEmpty()) {
                    _paragraph.insert(reviewMarks[markIndex].start + reviewMarks[markIndex].length,
                                      "[[" + comments[commentIndex] + "]]");
                }
            }
        }
    }

    switch (TextBlockStyle::forBlock(_block)) {
    case TextParagraphType::SceneHeading: {
        _paragraph.prepend('.');

        //
        // Если печатаем номера блоков, то добавим в конец этот номер, окруженный #
        //
        if (exportOptions.showBlockNumbers) {
            const auto blockData = static_cast<TextBlockData*>(_block.userData());
            if (blockData != nullptr && blockData->item()->parent() != nullptr
                && blockData->item()->parent()->type() == TextModelItemType::Group
                && static_cast<TextGroupType>(blockData->item()->parent()->subtype())
                    == TextGroupType::Scene) {
                const auto sceneItem
                    = static_cast<AudioplayTextModelSceneItem*>(blockData->item()->parent());
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

    case TextParagraphType::Dialogue:
    case TextParagraphType::Cue: {
        return true;
    }

    case TextParagraphType::Sound: {
        _paragraph.prepend("Sound: ");
        return true;
    }

    case TextParagraphType::Music: {
        _paragraph.prepend("Music: ");
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

QString AudioplayFountainExporter::formatSymbols(TextSelectionTypes _type) const
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

void AudioplayFountainExporter::addIndentationAtBegin(QString& _paragraph,
                                                      TextParagraphType _previosBlockType,
                                                      TextParagraphType _currentBlockType) const
{
    //
    // Таблица количества переносов строк между различными типами блоков
    // столбцы - предыдущий тип, строки - текущий
    //
    const QVector<QVector<int>> countIndentation = {
        // Undefined, SceneHeading, Character, Dialogue, Sound/Music/Cue/InlineNote/UnformattedText
        { 0, 0, 0, 0, 0 }, // Undefined
        { 0, 2, 3, 3, 3 }, // SceneHeading
        { 0, 2, 2, 2, 2 }, // Character
        { 0, 2, 1, 2, 2 }, // Dialogue
        { 0, 2, 2, 2, 2 }, // Sound/Music/Cue/InlineNote/UnformattedText
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
        case TextParagraphType::Sound:
        case TextParagraphType::Music:
        case TextParagraphType::Cue:
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

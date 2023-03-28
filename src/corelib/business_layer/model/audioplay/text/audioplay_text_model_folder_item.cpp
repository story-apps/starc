#include "audioplay_text_model_folder_item.h"

#include "audioplay_text_model.h"
#include "audioplay_text_model_scene_item.h"
#include "audioplay_text_model_text_item.h"

#include <business_layer/templates/audioplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

class AudioplayTextModelFolderItem::Implementation
{
public:
    //
    // Ридонли свойства, которые формируются по ходу работы с текстом
    //

    /**
     * @brief Длительность папки
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};


// ****


AudioplayTextModelFolderItem::AudioplayTextModelFolderItem(const AudioplayTextModel* _model,
                                                           TextFolderType _type)
    : TextModelFolderItem(_model, _type)
    , d(new Implementation)
{
}

AudioplayTextModelFolderItem::~AudioplayTextModelFolderItem() = default;

std::chrono::milliseconds AudioplayTextModelFolderItem::duration() const
{
    return d->duration;
}

QVariant AudioplayTextModelFolderItem::data(int _role) const
{
    switch (_role) {
    case FolderDurationRole: {
        const int duration = std::chrono::duration_cast<std::chrono::seconds>(d->duration).count();
        return duration;
    }

    default: {
        return TextModelFolderItem::data(_role);
    }
    }
}

void AudioplayTextModelFolderItem::handleChange()
{
    setHeading({});
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto childItem = static_cast<AudioplayTextModelFolderItem*>(child);
            d->duration += childItem->duration();
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<AudioplayTextModelSceneItem*>(child);
            d->duration += childItem->duration();
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<AudioplayTextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::ActHeading
                || childItem->paragraphType() == TextParagraphType::SequenceHeading) {
                setHeading(TextHelper::smartToUpper(childItem->text()));
            }
            d->duration += childItem->duration();
            break;
        }

        default:
            break;
        }
    }
}

} // namespace BusinessLayer

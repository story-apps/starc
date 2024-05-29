#include "stageplay_text_model_folder_item.h"

#include "stageplay_text_model.h"
#include "stageplay_text_model_scene_item.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

StageplayTextModelFolderItem::StageplayTextModelFolderItem(const StageplayTextModel* _model,
                                                           TextFolderType _type)
    : TextModelFolderItem(_model, _type)
{
}

StageplayTextModelFolderItem::~StageplayTextModelFolderItem() = default;

void StageplayTextModelFolderItem::handleChange()
{
    setHeading({});
    setWordsCount(0);
    setCharactersCount({});

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto childItem = static_cast<StageplayTextModelFolderItem*>(child);
            setWordsCount(wordsCount() + childItem->wordsCount());
            setCharactersCount({ charactersCount().first + childItem->charactersCount().first,
                                 charactersCount().second + childItem->charactersCount().second });
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<StageplayTextModelSceneItem*>(child);
            setWordsCount(wordsCount() + childItem->wordsCount());
            setCharactersCount({ charactersCount().first + childItem->charactersCount().first,
                                 charactersCount().second + childItem->charactersCount().second });
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<TextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::ActHeading
                || childItem->paragraphType() == TextParagraphType::SequenceHeading) {
                setHeading(childItem->text());
            }
            setWordsCount(wordsCount() + childItem->wordsCount());
            setCharactersCount({ charactersCount().first + childItem->charactersCount().first,
                                 charactersCount().second + childItem->charactersCount().second });
            break;
        }

        default:
            break;
        }
    }
}

} // namespace BusinessLayer

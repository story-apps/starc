#include "stageplay_text_model_folder_item.h"

#include "stageplay_text_model.h"
#include "stageplay_text_model_scene_item.h"

#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/stageplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

StageplayTextModelFolderItem::StageplayTextModelFolderItem(const StageplayTextModel* _model)
    : TextModelFolderItem(_model)
{
    setFolderType(TextFolderType::Sequence);
}

StageplayTextModelFolderItem::~StageplayTextModelFolderItem() = default;

void StageplayTextModelFolderItem::handleChange()
{
    setHeading({});

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Text: {
            auto childItem = static_cast<TextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::ActHeading
                || childItem->paragraphType() == TextParagraphType::SequenceHeading) {
                setHeading(TextHelper::smartToUpper(childItem->text()));
            }
            break;
        }

        default:
            break;
        }
    }
}

} // namespace BusinessLayer

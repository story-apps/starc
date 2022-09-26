#include "screenplay_text_model_folder_item.h"

#include "screenplay_text_model.h"
#include "screenplay_text_model_scene_item.h"
#include "screenplay_text_model_text_item.h"

#include <business_layer/templates/screenplay_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

class ScreenplayTextModelFolderItem::Implementation
{
public:
    /**
     * @brief Параметры отображения папки на доске с карточками
     */
    CardInfo cardInfo;

    //
    // Ридонли свойства, которые формируются по ходу работы с текстом
    //

    /**
     * @brief Длительность папки
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};


// ****


ScreenplayTextModelFolderItem::ScreenplayTextModelFolderItem(const ScreenplayTextModel* _model,
                                                             TextFolderType _type)
    : TextModelFolderItem(_model)
    , d(new Implementation)
{
    setFolderType(_type);
}

ScreenplayTextModelFolderItem::~ScreenplayTextModelFolderItem() = default;

const ScreenplayTextModelFolderItem::CardInfo& ScreenplayTextModelFolderItem::cardInfo() const
{
    return d->cardInfo;
}

void ScreenplayTextModelFolderItem::setCardInfo(const CardInfo& _info)
{
    if (d->cardInfo.geometry == _info.geometry && d->cardInfo.isOpened == _info.isOpened) {
        return;
    }

    d->cardInfo = _info;
    setChanged(true);
}

std::chrono::milliseconds ScreenplayTextModelFolderItem::duration() const
{
    return d->duration;
}

QVariant ScreenplayTextModelFolderItem::data(int _role) const
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

void ScreenplayTextModelFolderItem::handleChange()
{
    setHeading({});
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto childItem = static_cast<ScreenplayTextModelFolderItem*>(child);
            d->duration += childItem->duration();
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<ScreenplayTextModelSceneItem*>(child);
            d->duration += childItem->duration();
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<ScreenplayTextModelTextItem*>(child);
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

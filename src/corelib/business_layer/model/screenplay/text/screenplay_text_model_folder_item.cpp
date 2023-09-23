#include "screenplay_text_model_folder_item.h"

#include "screenplay_text_model.h"
#include "screenplay_text_model_beat_item.h"
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
     * @brief Количество слов
     */
    int wordsCount = 0;

    /**
     * @brief Количество символов
     */
    QPair<int, int> charactersCount;

    /**
     * @brief Длительность папки
     */
    std::chrono::milliseconds duration = std::chrono::milliseconds{ 0 };
};


// ****


ScreenplayTextModelFolderItem::ScreenplayTextModelFolderItem(const ScreenplayTextModel* _model,
                                                             TextFolderType _type)
    : TextModelFolderItem(_model, _type)
    , d(new Implementation)
{
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

int ScreenplayTextModelFolderItem::wordsCount() const
{
    return d->wordsCount;
}

QPair<int, int> ScreenplayTextModelFolderItem::charactersCount() const
{
    return d->charactersCount;
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

bool ScreenplayTextModelFolderItem::isFilterAccepted(const QString& _text, bool _isCaseSensitive,
                                                     int _filterType) const
{
    auto contains = [text = _text, cs = _isCaseSensitive ? Qt::CaseSensitive : Qt::CaseInsensitive](
                        const QString& _text) { return _text.contains(text, cs); };
    switch (_filterType) {
    default:
    case 0: {
        return contains(heading()) || contains(stamp());
    }

    case 1: {
        return contains(heading());
    }

    case 2:
    case 3: {
        return false;
    }
    }
}

void ScreenplayTextModelFolderItem::handleChange()
{
    setHeading({});
    d->wordsCount = 0;
    d->charactersCount = {};
    d->duration = std::chrono::seconds{ 0 };

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto folderItem = static_cast<ScreenplayTextModelFolderItem*>(child);
            d->wordsCount += folderItem->wordsCount();
            d->charactersCount.first += folderItem->charactersCount().first;
            d->charactersCount.second += folderItem->charactersCount().second;
            d->duration += folderItem->duration();
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<TextModelGroupItem*>(child);
            if (childItem->groupType() == TextGroupType::Scene) {
                const auto sceneItem = static_cast<ScreenplayTextModelSceneItem*>(childItem);
                d->wordsCount += sceneItem->wordsCount();
                d->charactersCount.first += sceneItem->charactersCount().first;
                d->charactersCount.second += sceneItem->charactersCount().second;
                d->duration += sceneItem->duration();
            } else {
                const auto beatItem = static_cast<ScreenplayTextModelBeatItem*>(childItem);
                d->wordsCount += beatItem->wordsCount();
                d->charactersCount.first += beatItem->charactersCount().first;
                d->charactersCount.second += beatItem->charactersCount().second;
                d->duration += beatItem->duration();
            }
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<ScreenplayTextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::ActHeading
                || childItem->paragraphType() == TextParagraphType::SequenceHeading) {
                setHeading(childItem->text());
            }
            d->wordsCount += childItem->wordsCount();
            d->charactersCount.first += childItem->charactersCount().first;
            d->charactersCount.second += childItem->charactersCount().second;
            d->duration += childItem->duration();
            break;
        }

        default:
            break;
        }
    }
}

} // namespace BusinessLayer

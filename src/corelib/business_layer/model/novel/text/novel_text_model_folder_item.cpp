#include "novel_text_model_folder_item.h"

#include "novel_text_model.h"
#include "novel_text_model_beat_item.h"
#include "novel_text_model_scene_item.h"
#include "novel_text_model_text_item.h"

#include <business_layer/templates/novel_template.h>
#include <utils/helpers/text_helper.h>


namespace BusinessLayer {

class NovelTextModelFolderItem::Implementation
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
};


// ****


NovelTextModelFolderItem::NovelTextModelFolderItem(const NovelTextModel* _model,
                                                   TextFolderType _type)
    : TextModelFolderItem(_model, _type)
    , d(new Implementation)
{
}

NovelTextModelFolderItem::~NovelTextModelFolderItem() = default;

const NovelTextModelFolderItem::CardInfo& NovelTextModelFolderItem::cardInfo() const
{
    return d->cardInfo;
}

void NovelTextModelFolderItem::setCardInfo(const CardInfo& _info)
{
    if (d->cardInfo.geometry == _info.geometry && d->cardInfo.isOpened == _info.isOpened) {
        return;
    }

    d->cardInfo = _info;
    setChanged(true);
}

int NovelTextModelFolderItem::wordsCount() const
{
    return d->wordsCount;
}

QPair<int, int> NovelTextModelFolderItem::charactersCount() const
{
    return d->charactersCount;
}

QVariant NovelTextModelFolderItem::data(int _role) const
{
    switch (_role) {
    case FolderWordCountRole: {
        return d->wordsCount;
    }

    default: {
        return TextModelFolderItem::data(_role);
    }
    }
}

bool NovelTextModelFolderItem::isFilterAccepted(const QString& _text, bool _isCaseSensitive,
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

void NovelTextModelFolderItem::handleChange()
{
    setHeading({});
    d->wordsCount = 0;
    d->charactersCount = {};

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto folderItem = static_cast<NovelTextModelFolderItem*>(child);
            d->wordsCount += folderItem->wordsCount();
            d->charactersCount.first += folderItem->charactersCount().first;
            d->charactersCount.second += folderItem->charactersCount().second;
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<TextModelGroupItem*>(child);
            if (childItem->groupType() == TextGroupType::Scene) {
                const auto sceneItem = static_cast<NovelTextModelSceneItem*>(childItem);
                d->wordsCount += sceneItem->wordsCount();
                d->charactersCount.first += sceneItem->charactersCount().first;
                d->charactersCount.second += sceneItem->charactersCount().second;
            } else {
                const auto beatItem = static_cast<NovelTextModelBeatItem*>(childItem);
                d->wordsCount += beatItem->wordsCount();
                d->charactersCount.first += beatItem->charactersCount().first;
                d->charactersCount.second += beatItem->charactersCount().second;
            }
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<NovelTextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::PartHeading
                || childItem->paragraphType() == TextParagraphType::ChapterHeading) {
                setHeading(childItem->text());
            }
            d->wordsCount += childItem->wordsCount();
            d->charactersCount.first += childItem->charactersCount().first;
            d->charactersCount.second += childItem->charactersCount().second;
            break;
        }

        default:
            break;
        }
    }
}

} // namespace BusinessLayer

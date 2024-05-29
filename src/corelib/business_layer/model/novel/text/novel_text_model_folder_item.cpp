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

QString NovelTextModelFolderItem::text() const
{
    QString text;
    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto folderItem = static_cast<NovelTextModelFolderItem*>(child);
            if (!text.isEmpty()) {
                text += "\n\n";
            }
            text += folderItem->text();
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<TextModelGroupItem*>(child);
            if (!text.isEmpty()) {
                text += "\n\n";
            }
            text += childItem->text();
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<NovelTextModelTextItem*>(child);
            if (!text.isEmpty()) {
                text += "\n";
            }
            text += childItem->text();
            break;
        }

        default:
            break;
        }
    }
    return text;
}

QVariant NovelTextModelFolderItem::data(int _role) const
{
    switch (_role) {
    case FolderWordCountRole: {
        return wordsCount();
    }

    case FolderCharacterCountRole: {
        return charactersCount().first;
    }

    case FolderCharacterCountWithSpacesRole: {
        return charactersCount().second;
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
    setWordsCount(0);
    setCharactersCount({});

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto folderItem = static_cast<NovelTextModelFolderItem*>(child);
            setWordsCount(wordsCount() + folderItem->wordsCount());
            setCharactersCount({ charactersCount().first + folderItem->charactersCount().first,
                                 charactersCount().second + folderItem->charactersCount().second });
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<TextModelGroupItem*>(child);
            if (childItem->groupType() == TextGroupType::Scene) {
                const auto sceneItem = static_cast<NovelTextModelSceneItem*>(childItem);
                setWordsCount(wordsCount() + sceneItem->wordsCount());
                setCharactersCount(
                    { charactersCount().first + sceneItem->charactersCount().first,
                      charactersCount().second + sceneItem->charactersCount().second });
            } else {
                const auto beatItem = static_cast<NovelTextModelBeatItem*>(childItem);
                setWordsCount(wordsCount() + beatItem->wordsCount());
                setCharactersCount(
                    { charactersCount().first + beatItem->charactersCount().first,
                      charactersCount().second + beatItem->charactersCount().second });
            }
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<NovelTextModelTextItem*>(child);
            if (childItem->paragraphType() == TextParagraphType::PartHeading
                || childItem->paragraphType() == TextParagraphType::ChapterHeading) {
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

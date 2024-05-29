#include "comic_book_text_model_folder_item.h"

#include "comic_book_text_model.h"
#include "comic_book_text_model_page_item.h"
#include "comic_book_text_model_panel_item.h"
#include "comic_book_text_model_text_item.h"

#include <business_layer/templates/text_template.h>

namespace BusinessLayer {

ComicBookTextModelFolderItem::ComicBookTextModelFolderItem(const ComicBookTextModel* _model,
                                                           TextFolderType _type)
    : TextModelFolderItem(_model, _type)
{
}

ComicBookTextModelFolderItem::~ComicBookTextModelFolderItem() = default;

void ComicBookTextModelFolderItem::handleChange()
{
    setHeading({});
    setWordsCount(0);
    setCharactersCount({});

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto childItem = static_cast<ComicBookTextModelPageItem*>(child);
            setWordsCount(wordsCount() + childItem->wordsCount());
            setCharactersCount({ charactersCount().first + childItem->charactersCount().first,
                                 charactersCount().second + childItem->charactersCount().second });
            break;
        }

        case TextModelItemType::Group: {
            auto groupItem = static_cast<TextModelGroupItem*>(child);
            if (groupItem->groupType() == TextGroupType::Page) {
                auto childItem = static_cast<ComicBookTextModelPageItem*>(child);
                setWordsCount(wordsCount() + childItem->wordsCount());
                setCharactersCount(
                    { charactersCount().first + childItem->charactersCount().first,
                      charactersCount().second + childItem->charactersCount().second });
            } else if (groupItem->groupType() == TextGroupType::Panel) {
                auto childItem = static_cast<ComicBookTextModelPanelItem*>(child);
                setWordsCount(wordsCount() + childItem->wordsCount());
                setCharactersCount(
                    { charactersCount().first + childItem->charactersCount().first,
                      charactersCount().second + childItem->charactersCount().second });
            }
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<ComicBookTextModelTextItem*>(child);
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

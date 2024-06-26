#include "simple_text_model_folder_item.h"

#include "simple_text_model.h"
#include "simple_text_model_chapter_item.h"

#include <business_layer/model/text/text_model_text_item.h>

namespace BusinessLayer {

SimpleTextModelFolderItem::SimpleTextModelFolderItem(const SimpleTextModel* _model,
                                                     TextFolderType _type)
    : TextModelFolderItem(_model, _type)
{
}

SimpleTextModelFolderItem::~SimpleTextModelFolderItem() = default;

void SimpleTextModelFolderItem::handleChange()
{
    setHeading({});
    setWordsCount(0);
    setCharactersCount({});

    for (int childIndex = 0; childIndex < childCount(); ++childIndex) {
        auto child = childAt(childIndex);
        switch (child->type()) {
        case TextModelItemType::Folder: {
            auto childItem = static_cast<SimpleTextModelFolderItem*>(child);
            setWordsCount(wordsCount() + childItem->wordsCount());
            setCharactersCount({ charactersCount().first + childItem->charactersCount().first,
                                 charactersCount().second + childItem->charactersCount().second });
            break;
        }

        case TextModelItemType::Group: {
            auto childItem = static_cast<SimpleTextModelChapterItem*>(child);
            setWordsCount(wordsCount() + childItem->wordsCount());
            setCharactersCount({ charactersCount().first + childItem->charactersCount().first,
                                 charactersCount().second + childItem->charactersCount().second });
            break;
        }

        case TextModelItemType::Text: {
            auto childItem = static_cast<TextModelTextItem*>(child);
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

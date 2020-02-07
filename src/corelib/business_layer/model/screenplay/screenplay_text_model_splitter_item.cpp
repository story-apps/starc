#include "screenplay_text_model_splitter_item.h"


namespace BusinessLayer
{

class ScreenplayTextModelSplitterItem::Implementation
{
public:
    explicit Implementation(ScreenplayTextModelSplitterItemType _type);

    const ScreenplayTextModelSplitterItemType type;
};

ScreenplayTextModelSplitterItem::Implementation::Implementation(ScreenplayTextModelSplitterItemType _type)
    : type(_type)
{
}


// ****


ScreenplayTextModelSplitterItem::ScreenplayTextModelSplitterItem(ScreenplayTextModelSplitterItemType _type)
    : ScreenplayTextModelItem(ScreenplayTextModelItemType::Splitter),
      d(new Implementation(_type))
{
}

ScreenplayTextModelSplitterItem::~ScreenplayTextModelSplitterItem() = default;

} // namespace BusinessLayer

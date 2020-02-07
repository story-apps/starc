#include "screenplay_text_model_item.h"


namespace BusinessLayer
{

class ScreenplayTextModelItem::Implementation
{
public:
    explicit Implementation(ScreenplayTextModelItemType _type);

    const ScreenplayTextModelItemType type;
};

ScreenplayTextModelItem::Implementation::Implementation(ScreenplayTextModelItemType _type)
    : type(_type)
{
}


// ****


ScreenplayTextModelItem::ScreenplayTextModelItem(ScreenplayTextModelItemType _type)
    : d(new Implementation(_type))
{
}

ScreenplayTextModelItem::~ScreenplayTextModelItem() = default;

ScreenplayTextModelItemType ScreenplayTextModelItem::type() const
{
    return d->type;
}

} // namespace BusinessLayer

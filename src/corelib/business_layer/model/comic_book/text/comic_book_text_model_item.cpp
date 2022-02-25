#include "comic_book_text_model_item.h"

#include <QVariant>


namespace BusinessLayer {

class ComicBookTextModelItem::Implementation
{
public:
    explicit Implementation(ComicBookTextModelItemType _type);

    const ComicBookTextModelItemType type;
};

ComicBookTextModelItem::Implementation::Implementation(ComicBookTextModelItemType _type)
    : type(_type)
{
}


// ****


ComicBookTextModelItem::ComicBookTextModelItem(ComicBookTextModelItemType _type)
    : d(new Implementation(_type))
{
}

ComicBookTextModelItem::~ComicBookTextModelItem() = default;

const ComicBookTextModelItemType& ComicBookTextModelItem::type() const
{
    return d->type;
}

ComicBookTextModelItem* ComicBookTextModelItem::parent() const
{
    return static_cast<ComicBookTextModelItem*>(AbstractModelItem::parent());
}

ComicBookTextModelItem* ComicBookTextModelItem::childAt(int _index) const
{
    return static_cast<ComicBookTextModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant ComicBookTextModelItem::data(int _role) const
{
    if (_role == Qt::UserRole) {
        return static_cast<int>(d->type);
    }

    return {};
}

} // namespace BusinessLayer

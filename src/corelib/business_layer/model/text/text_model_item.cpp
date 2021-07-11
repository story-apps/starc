#include "text_model_item.h"

#include <QVariant>


namespace BusinessLayer {

class TextModelItem::Implementation
{
public:
    explicit Implementation(TextModelItemType _type);

    const TextModelItemType type;
};

TextModelItem::Implementation::Implementation(TextModelItemType _type)
    : type(_type)
{
}


// ****


TextModelItem::TextModelItem(TextModelItemType _type)
    : d(new Implementation(_type))
{
}

TextModelItem::~TextModelItem() = default;

TextModelItemType TextModelItem::type() const
{
    return d->type;
}

TextModelItem* TextModelItem::parent() const
{
    return static_cast<TextModelItem*>(AbstractModelItem::parent());
}

TextModelItem* TextModelItem::childAt(int _index) const
{
    return static_cast<TextModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant TextModelItem::data(int _role) const
{
    if (_role == Qt::UserRole) {
        return static_cast<int>(d->type);
    }

    return {};
}

} // namespace BusinessLayer

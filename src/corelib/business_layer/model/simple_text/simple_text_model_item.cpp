#include "simple_text_model_item.h"

#include <QVariant>


namespace BusinessLayer {

class SimpleTextModelItem::Implementation
{
public:
    explicit Implementation(TextModelItemType _type);

    const TextModelItemType type;
};

SimpleTextModelItem::Implementation::Implementation(TextModelItemType _type)
    : type(_type)
{
}


// ****


SimpleTextModelItem::SimpleTextModelItem(TextModelItemType _type)
    : d(new Implementation(_type))
{
}

SimpleTextModelItem::~SimpleTextModelItem() = default;

const TextModelItemType& SimpleTextModelItem::type() const
{
    return d->type;
}

SimpleTextModelItem* SimpleTextModelItem::parent() const
{
    return static_cast<SimpleTextModelItem*>(AbstractModelItem::parent());
}

SimpleTextModelItem* SimpleTextModelItem::childAt(int _index) const
{
    return static_cast<SimpleTextModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant SimpleTextModelItem::data(int _role) const
{
    if (_role == Qt::UserRole) {
        return static_cast<int>(d->type);
    }

    return {};
}

} // namespace BusinessLayer

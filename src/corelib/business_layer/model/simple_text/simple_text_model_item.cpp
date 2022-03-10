#include "simple_text_model_item.h"

#include <QVariant>


namespace BusinessLayer {

class SimpleTextModelItem::Implementation
{
public:
    explicit Implementation(SimpleTextModelItemType _type);

    const SimpleTextModelItemType type;
};

SimpleTextModelItem::Implementation::Implementation(SimpleTextModelItemType _type)
    : type(_type)
{
}


// ****


SimpleTextModelItem::SimpleTextModelItem(SimpleTextModelItemType _type)
    : d(new Implementation(_type))
{
}

SimpleTextModelItem::~SimpleTextModelItem() = default;

const SimpleTextModelItemType& SimpleTextModelItem::type() const
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

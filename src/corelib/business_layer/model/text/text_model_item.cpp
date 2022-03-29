#include "text_model_item.h"

#include <QVariant>


namespace BusinessLayer {

class TextModelItem::Implementation
{
public:
    Implementation(TextModelItemType _type, const TextModel* _model);

    const TextModelItemType type;
    const TextModel* model = nullptr;
};

TextModelItem::Implementation::Implementation(TextModelItemType _type, const TextModel* _model)
    : type(_type)
    , model(_model)
{
}


// ****


TextModelItem::TextModelItem(TextModelItemType _type, const TextModel* _model)
    : d(new Implementation(_type, _model))
{
    Q_ASSERT(_model);
}

TextModelItem::~TextModelItem() = default;

const TextModelItemType& TextModelItem::type() const
{
    return d->type;
}

int TextModelItem::subtype() const
{
    return 0;
}

const TextModel* TextModelItem::model() const
{
    return d->model;
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

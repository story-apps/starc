#include "screenplay_text_model_item.h"

#include <QVariant>


namespace BusinessLayer {

class ScreenplayTextModelItem::Implementation
{
public:
    Implementation(ScreenplayTextModelItemType _type, const ScreenplayTextModel* _model);

    const ScreenplayTextModelItemType type;
    const ScreenplayTextModel* model = nullptr;
};

ScreenplayTextModelItem::Implementation::Implementation(ScreenplayTextModelItemType _type,
                                                        const ScreenplayTextModel* _model)
    : type(_type)
    , model(_model)
{
}


// ****


ScreenplayTextModelItem::ScreenplayTextModelItem(ScreenplayTextModelItemType _type,
                                                 const ScreenplayTextModel* _model)
    : d(new Implementation(_type, _model))
{
    Q_ASSERT(_model);
}

ScreenplayTextModelItem::~ScreenplayTextModelItem() = default;

ScreenplayTextModelItemType ScreenplayTextModelItem::type() const
{
    return d->type;
}

const ScreenplayTextModel* ScreenplayTextModelItem::model() const
{
    return d->model;
}

ScreenplayTextModelItem* ScreenplayTextModelItem::parent() const
{
    return static_cast<ScreenplayTextModelItem*>(AbstractModelItem::parent());
}

ScreenplayTextModelItem* ScreenplayTextModelItem::childAt(int _index) const
{
    return static_cast<ScreenplayTextModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant ScreenplayTextModelItem::data(int _role) const
{
    if (_role == Qt::UserRole) {
        return static_cast<int>(d->type);
    }

    return {};
}

} // namespace BusinessLayer

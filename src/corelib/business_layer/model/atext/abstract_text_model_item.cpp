#include "abstract_text_model_item.h"

#include <QVariant>


namespace BusinessLayer {

class AbstractTextModelItem::Implementation
{
public:
    Implementation(AbstractTextModelItemType _type, const AbstractTextModel* _model);

    const AbstractTextModelItemType type;
    const AbstractTextModel* model = nullptr;
};

AbstractTextModelItem::Implementation::Implementation(AbstractTextModelItemType _type,
                                                      const AbstractTextModel* _model)
    : type(_type)
    , model(_model)
{
}


// ****


AbstractTextModelItem::AbstractTextModelItem(AbstractTextModelItemType _type,
                                             const AbstractTextModel* _model)
    : d(new Implementation(_type, _model))
{
    Q_ASSERT(_model);
}

AbstractTextModelItem::~AbstractTextModelItem() = default;

const AbstractTextModelItemType& AbstractTextModelItem::type() const
{
    return d->type;
}

const AbstractTextModel* AbstractTextModelItem::model() const
{
    return d->model;
}

AbstractTextModelItem* AbstractTextModelItem::parent() const
{
    return static_cast<AbstractTextModelItem*>(AbstractModelItem::parent());
}

AbstractTextModelItem* AbstractTextModelItem::childAt(int _index) const
{
    return static_cast<AbstractTextModelItem*>(AbstractModelItem::childAt(_index));
}

QVariant AbstractTextModelItem::data(int _role) const
{
    if (_role == Qt::UserRole) {
        return static_cast<int>(d->type);
    }

    return {};
}

} // namespace BusinessLayer

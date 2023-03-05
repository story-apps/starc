#include "screenplay_breakdown_structure_model_proxy.h"

#include "screenplay_breakdown_structure_model_item.h"


namespace BusinessLayer {

class ScreenplayBreakdownStructureModelProxy::Implementation
{
public:
    explicit Implementation(ScreenplayBreakdownStructureModelProxy* _q);


    ScreenplayBreakdownStructureModelProxy* q = nullptr;

    /**
     * @brief Порядок сортировки модели
     */
    SortOrder sortOrder = SortOrder::Undefined;
};

ScreenplayBreakdownStructureModelProxy::Implementation::Implementation(
    ScreenplayBreakdownStructureModelProxy* _q)
    : q(_q)
{
}


// ****


ScreenplayBreakdownStructureModelProxy::ScreenplayBreakdownStructureModelProxy(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation(this))
{
    setDynamicSortFilter(false);
    setRecursiveFilteringEnabled(true);
}

ScreenplayBreakdownStructureModelProxy::~ScreenplayBreakdownStructureModelProxy() = default;

void ScreenplayBreakdownStructureModelProxy::setSortOrder(SortOrder _sortOrder)
{
    d->sortOrder = _sortOrder;
    sort(0);
}

bool ScreenplayBreakdownStructureModelProxy::lessThan(const QModelIndex& _sourceLeft,
                                                      const QModelIndex& _sourceRight) const
{
    switch (d->sortOrder) {
    default:
    case SortOrder::Undefined: {
        return false;
    }

    case SortOrder::ByDuration: {
        return _sourceLeft.data(ScreenplayBreakdownStructureModelItem::DurationRole).toInt()
            > _sourceRight.data(ScreenplayBreakdownStructureModelItem::DurationRole).toInt();
    }

    case SortOrder::Alphabetically: {
        return QSortFilterProxyModel::lessThan(_sourceLeft, _sourceRight);
    }
    }
}

} // namespace BusinessLayer

#include "structure_proxy_model.h"

#include "structure_model.h"
#include "structure_model_item.h"


namespace BusinessLayer
{

StructureProxyModel::StructureProxyModel(StructureModel* _parent)
    : QSortFilterProxyModel(_parent)
{
    setSourceModel(_parent);
}

StructureProxyModel::~StructureProxyModel() = default;

bool StructureProxyModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const
{
    const auto index = sourceModel()->index(_sourceRow, 0, _sourceParent);
    auto structure = qobject_cast<StructureModel*>(sourceModel());
    return structure->itemForIndex(index)->visible();
}

} // namespace BusinessLayer

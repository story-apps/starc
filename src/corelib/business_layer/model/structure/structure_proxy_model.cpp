#include "structure_proxy_model.h"

#include "structure_model.h"
#include "structure_model_item.h"

#include <domain/document_object.h>


namespace BusinessLayer {

StructureProxyModel::StructureProxyModel(StructureModel* _parent)
    : QSortFilterProxyModel(_parent)
{
    setSourceModel(_parent);
}

StructureProxyModel::~StructureProxyModel() = default;

bool StructureProxyModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const
{
    const auto index = sourceModel()->index(_sourceRow, 0, _sourceParent);
    const auto structure = qobject_cast<StructureModel*>(sourceModel());
    const auto item = structure->itemForIndex(index);
    switch (item->type()) {

    //
    // Отображаем группирующие элементы только если есть дети
    //
    case Domain::DocumentObjectType::Characters:
    case Domain::DocumentObjectType::Locations:
    case Domain::DocumentObjectType::Worlds: {
        return item->hasChildren();
    }

    //
    // Все остальные отображаем только по флагу элемента
    //
    default: {
        return item->isVisible();
    }
    }

    return false;
}

} // namespace BusinessLayer

#include "screenplay_text_structure_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_item.h>


namespace BusinessLayer
{

class ScreenplayTextStructureModel::Implementation
{
public:
    ScreenplayTextModel* screenplayModel = nullptr;
};


// ****


ScreenplayTextStructureModel::ScreenplayTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent),
      d(new Implementation)
{
}

ScreenplayTextStructureModel::~ScreenplayTextStructureModel() = default;

void ScreenplayTextStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    d->screenplayModel = qobject_cast<ScreenplayTextModel*>(_sourceModel);

    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool ScreenplayTextStructureModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const
{
    if (d->screenplayModel == nullptr) {
        return false;
    }

    const auto item = d->screenplayModel->itemForIndex(d->screenplayModel->index(_sourceRow, 0, _sourceParent));
    return item->type() == ScreenplayTextModelItemType::Folder
            || item->type() == ScreenplayTextModelItemType::Scene;
}

} // namespace BusinessLayer

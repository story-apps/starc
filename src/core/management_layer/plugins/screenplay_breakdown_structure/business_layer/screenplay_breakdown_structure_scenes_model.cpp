#include "screenplay_breakdown_structure_scenes_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/screenplay_template.h>

#include <QApplication>


namespace BusinessLayer {

class ScreenplayBreakdownStructureScenesModel::Implementation
{
public:
    ScreenplayTextModel* screenplayModel = nullptr;
};


// ****


ScreenplayBreakdownStructureScenesModel::ScreenplayBreakdownStructureScenesModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

ScreenplayBreakdownStructureScenesModel::~ScreenplayBreakdownStructureScenesModel() = default;

void ScreenplayBreakdownStructureScenesModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    d->screenplayModel = qobject_cast<ScreenplayTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool ScreenplayBreakdownStructureScenesModel::filterAcceptsRow(
    int _sourceRow, const QModelIndex& _sourceParent) const
{
    if (d->screenplayModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->screenplayModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->screenplayModel->itemForIndex(itemIndex);

    //
    // Показываем папки
    //
    if (item->type() == TextModelItemType::Folder) {
        return true;
    }
    //
    // Показываем сцены
    //
    else if (item->type() == TextModelItemType::Group) {
        const auto groupItem = static_cast<TextModelGroupItem*>(item);
        if (groupItem->groupType() == TextGroupType::Scene) {
            return true;
        }
    }
    //
    // Остальное не показываем
    //
    return false;
}

} // namespace BusinessLayer

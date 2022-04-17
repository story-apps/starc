#include "screenplay_treatment_structure_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/screenplay_template.h>

#include <QApplication>


namespace BusinessLayer {

class ScreenplayTreatmentStructureModel::Implementation
{
public:
    ScreenplayTextModel* screenplayModel = nullptr;
    bool showBeats = true;
};


// ****


ScreenplayTreatmentStructureModel::ScreenplayTreatmentStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

ScreenplayTreatmentStructureModel::~ScreenplayTreatmentStructureModel() = default;

void ScreenplayTreatmentStructureModel::showBeats(bool _show)
{
    if (d->showBeats == _show) {
        return;
    }

    d->showBeats = _show;
    invalidate();
}

void ScreenplayTreatmentStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    d->screenplayModel = qobject_cast<ScreenplayTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool ScreenplayTreatmentStructureModel::filterAcceptsRow(int _sourceRow,
                                                         const QModelIndex& _sourceParent) const
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
    // Показываем сцены и биты (если разрешены)
    //
    else if (item->type() == TextModelItemType::Group) {
        auto groupItem = static_cast<TextModelGroupItem*>(item);
        if (groupItem->groupType() == TextGroupType::Beat) {
            return d->showBeats;
        } else {
            return true;
        }
    }
    //
    // Остальное не показываем
    //
    return false;
}

} // namespace BusinessLayer

#include "stageplay_text_structure_model.h"

#include <business_layer/model/stageplay/text/stageplay_text_model.h>
#include <business_layer/model/text/text_model_item.h>

#include <QApplication>


namespace BusinessLayer {

class StageplayTextStructureModel::Implementation
{
public:
    StageplayTextModel* stageplayModel = nullptr;
};


// ****


StageplayTextStructureModel::StageplayTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

StageplayTextStructureModel::~StageplayTextStructureModel() = default;

void StageplayTextStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    if (d->stageplayModel) {
        d->stageplayModel->disconnect(this);
    }

    d->stageplayModel = qobject_cast<StageplayTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool StageplayTextStructureModel::filterAcceptsRow(int _sourceRow,
                                                   const QModelIndex& _sourceParent) const
{
    if (d->stageplayModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->stageplayModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->stageplayModel->itemForIndex(itemIndex);

    //
    // Показываем папки и сцены
    //
    if (item->type() == TextModelItemType::Folder || item->type() == TextModelItemType::Group) {
        return true;
    }
    //
    // Остальное не показываем
    //
    return false;
}

} // namespace BusinessLayer

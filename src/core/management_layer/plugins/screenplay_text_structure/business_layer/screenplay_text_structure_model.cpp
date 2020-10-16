#include "screenplay_text_structure_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_item.h>

#include <QApplication>


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
    if (d->screenplayModel) {
        d->screenplayModel->disconnect(this);
    }

    d->screenplayModel = qobject_cast<ScreenplayTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);

    if (d->screenplayModel != nullptr) {
        //
        // FIXME: Это сделано из-за того, что при перемещении элементов внутри модели сценария
        //        в позицию 0, на вторую попытку происходит падение внутри QSortFilterModel,
        //        видимо не успевает происходить какая-то внутренняя магия при синхронном удалении
        //        и последующей вставки элементов в модели
        //
        connect(d->screenplayModel, &ScreenplayTextModel::rowsRemoved, this, [] {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        });
    }
}

bool ScreenplayTextStructureModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const
{
    if (d->screenplayModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->screenplayModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->screenplayModel->itemForIndex(itemIndex);
    return item->type() == ScreenplayTextModelItemType::Folder
            || item->type() == ScreenplayTextModelItemType::Scene;
}

} // namespace BusinessLayer

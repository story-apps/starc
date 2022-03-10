#include "screenplay_text_structure_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/templates/screenplay_template.h>

#include <QApplication>


namespace BusinessLayer {

class ScreenplayTextStructureModel::Implementation
{
public:
    ScreenplayTextModel* screenplayModel = nullptr;
};


// ****


ScreenplayTextStructureModel::ScreenplayTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
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
}

bool ScreenplayTextStructureModel::filterAcceptsRow(int _sourceRow,
                                                    const QModelIndex& _sourceParent) const
{
    if (d->screenplayModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->screenplayModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->screenplayModel->itemForIndex(itemIndex);

    //
    // Показываем папки и сцены
    //
    if (item->type() == ScreenplayTextModelItemType::Folder
        || item->type() == ScreenplayTextModelItemType::Scene) {
        return true;
    }
    //
    // Из текста показываем только кадры, которые не являются корректировочными блоками
    //
    if (item->type() == ScreenplayTextModelItemType::Text) {
        const auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
        return !textItem->isCorrection() && textItem->paragraphType() == TextParagraphType::Shot;
    }
    //
    // Остальное не показываем
    //
    return false;
}

} // namespace BusinessLayer

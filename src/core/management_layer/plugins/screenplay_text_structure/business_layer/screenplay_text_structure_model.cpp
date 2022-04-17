#include "screenplay_text_structure_model.h"

#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_text_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/screenplay_template.h>

#include <QApplication>


namespace BusinessLayer {

class ScreenplayTextStructureModel::Implementation
{
public:
    ScreenplayTextModel* screenplayModel = nullptr;
    bool showBeats = true;
};


// ****


ScreenplayTextStructureModel::ScreenplayTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

ScreenplayTextStructureModel::~ScreenplayTextStructureModel() = default;

void ScreenplayTextStructureModel::showBeats(bool _show)
{
    if (d->showBeats == _show) {
        return;
    }

    d->showBeats = _show;
    invalidate();
}

void ScreenplayTextStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
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
    // Из текста показываем только кадры, которые не являются корректировочными блоками
    //
    else if (item->type() == TextModelItemType::Text) {
        const auto textItem = static_cast<ScreenplayTextModelTextItem*>(item);
        return !textItem->isCorrection() && textItem->paragraphType() == TextParagraphType::Shot;
    }
    //
    // Остальное не показываем
    //
    return false;
}

} // namespace BusinessLayer

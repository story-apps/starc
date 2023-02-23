#include "novel_outline_structure_model.h"

#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/novel/text/novel_text_model_text_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/novel_template.h>

#include <QApplication>


namespace BusinessLayer {

class NovelOutlineStructureModel::Implementation
{
public:
    NovelTextModel* novelModel = nullptr;
    bool showBeats = true;
};


// ****


NovelOutlineStructureModel::NovelOutlineStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

NovelOutlineStructureModel::~NovelOutlineStructureModel() = default;

void NovelOutlineStructureModel::showBeats(bool _show)
{
    if (d->showBeats == _show) {
        return;
    }

    d->showBeats = _show;
    invalidate();
}

void NovelOutlineStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    d->novelModel = qobject_cast<NovelTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool NovelOutlineStructureModel::filterAcceptsRow(int _sourceRow,
                                                  const QModelIndex& _sourceParent) const
{
    if (d->novelModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->novelModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->novelModel->itemForIndex(itemIndex);

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

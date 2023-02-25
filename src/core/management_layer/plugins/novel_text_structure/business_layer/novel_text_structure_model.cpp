#include "novel_text_structure_model.h"

#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/novel/text/novel_text_model_text_item.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <business_layer/templates/novel_template.h>

#include <QApplication>


namespace BusinessLayer {

class NovelTextStructureModel::Implementation
{
public:
    NovelTextModel* novelModel = nullptr;
    bool showBeats = true;
};


// ****


NovelTextStructureModel::NovelTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

NovelTextStructureModel::~NovelTextStructureModel() = default;

void NovelTextStructureModel::showBeats(bool _show)
{
    if (d->showBeats == _show) {
        return;
    }

    d->showBeats = _show;
    invalidate();
}

void NovelTextStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    d->novelModel = qobject_cast<NovelTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool NovelTextStructureModel::filterAcceptsRow(int _sourceRow,
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
    // Из текста показываем только кадры, которые не являются корректировочными блоками
    //
    else if (item->type() == TextModelItemType::Text) {
        const auto textItem = static_cast<NovelTextModelTextItem*>(item);
        return !textItem->isCorrection() && textItem->paragraphType() == TextParagraphType::Shot;
    }
    //
    // Остальное не показываем
    //
    return false;
}

} // namespace BusinessLayer

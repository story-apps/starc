#include "structure_proxy_model.h"

#include "structure_model.h"
#include "structure_model_item.h"

#include <domain/document_object.h>

#include <QSet>


namespace BusinessLayer {

class StructureProxyModel::Implementation
{
public:
    bool isSubitemsVisible = true;

    bool isProjectInfoVisible = true;
    bool isRecycleBinVisible = true;

    QSet<QUuid> itemsFilter;
};


// ****


StructureProxyModel::StructureProxyModel(StructureModel* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
    setSourceModel(_parent);
}

StructureProxyModel::~StructureProxyModel() = default;

void StructureProxyModel::setSubitemsVisible(bool _visible)
{
    if (d->isSubitemsVisible == _visible) {
        return;
    }

    d->isSubitemsVisible = _visible;
    invalidateFilter();
}

void StructureProxyModel::setProjectInfoVisible(bool _visible)
{
    if (d->isProjectInfoVisible == _visible) {
        return;
    }

    d->isProjectInfoVisible = _visible;
    invalidateFilter();
}

void StructureProxyModel::setRecycleBinVisible(bool _visible)
{
    if (d->isRecycleBinVisible == _visible) {
        return;
    }

    d->isRecycleBinVisible = _visible;
    invalidateFilter();
}

void StructureProxyModel::setItemsFilter(const QList<QUuid>& _uuids)
{
    const auto itemsFilter = QSet<QUuid>(_uuids.begin(), _uuids.end());
    if (d->itemsFilter == itemsFilter) {
        return;
    }

    d->itemsFilter = itemsFilter;
    invalidateFilter();
}

bool StructureProxyModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const
{
    const auto index = sourceModel()->index(_sourceRow, 0, _sourceParent);
    const auto structure = qobject_cast<StructureModel*>(sourceModel());
    const auto item = structure->itemForIndex(index);

    //
    // Скрываем элементы, которые не попадают в спискок доступных
    //
    if (!d->itemsFilter.isEmpty()) {
        auto uuidToCheck = item->uuid();
        switch (item->type()) {
        case Domain::DocumentObjectType::ScreenplayTitlePage:
        case Domain::DocumentObjectType::ScreenplaySynopsis:
        case Domain::DocumentObjectType::ScreenplayTreatment:
        case Domain::DocumentObjectType::ScreenplayText:
        case Domain::DocumentObjectType::ScreenplayStatistics:
        case Domain::DocumentObjectType::ComicBookTitlePage:
        case Domain::DocumentObjectType::ComicBookSynopsis:
        case Domain::DocumentObjectType::ComicBookText:
        case Domain::DocumentObjectType::ComicBookStatistics:
        case Domain::DocumentObjectType::AudioplayTitlePage:
        case Domain::DocumentObjectType::AudioplaySynopsis:
        case Domain::DocumentObjectType::AudioplayText:
        case Domain::DocumentObjectType::AudioplayStatistics:
        case Domain::DocumentObjectType::StageplayTitlePage:
        case Domain::DocumentObjectType::StageplaySynopsis:
        case Domain::DocumentObjectType::StageplayText:
        case Domain::DocumentObjectType::StageplayStatistics:
        case Domain::DocumentObjectType::NovelTitlePage:
        case Domain::DocumentObjectType::NovelSynopsis:
        case Domain::DocumentObjectType::NovelOutline:
        case Domain::DocumentObjectType::NovelText:
        case Domain::DocumentObjectType::NovelStatistics:
        case Domain::DocumentObjectType::Character:
        case Domain::DocumentObjectType::Location:
        case Domain::DocumentObjectType::World: {
            uuidToCheck = item->parent()->uuid();
            break;
        }

        default: {
            break;
        }
        }

        if (!d->itemsFilter.contains(uuidToCheck)) {
            return false;
        }
    }

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
    // Элементы наборов отображаем в зависимости от настроек
    //
    case Domain::DocumentObjectType::ScreenplayTitlePage:
    case Domain::DocumentObjectType::ScreenplaySynopsis:
    case Domain::DocumentObjectType::ScreenplayTreatment:
    case Domain::DocumentObjectType::ScreenplayText:
    case Domain::DocumentObjectType::ScreenplayStatistics:
    case Domain::DocumentObjectType::ComicBookTitlePage:
    case Domain::DocumentObjectType::ComicBookSynopsis:
    case Domain::DocumentObjectType::ComicBookText:
    case Domain::DocumentObjectType::ComicBookStatistics:
    case Domain::DocumentObjectType::AudioplayTitlePage:
    case Domain::DocumentObjectType::AudioplaySynopsis:
    case Domain::DocumentObjectType::AudioplayText:
    case Domain::DocumentObjectType::AudioplayStatistics:
    case Domain::DocumentObjectType::StageplayTitlePage:
    case Domain::DocumentObjectType::StageplaySynopsis:
    case Domain::DocumentObjectType::StageplayText:
    case Domain::DocumentObjectType::StageplayStatistics:
    case Domain::DocumentObjectType::NovelTitlePage:
    case Domain::DocumentObjectType::NovelSynopsis:
    case Domain::DocumentObjectType::NovelOutline:
    case Domain::DocumentObjectType::NovelText:
    case Domain::DocumentObjectType::NovelStatistics:
    case Domain::DocumentObjectType::Character:
    case Domain::DocumentObjectType::Location:
    case Domain::DocumentObjectType::World: {
        return d->isSubitemsVisible;
    }

    //
    // Конкретные документы отображаем в соответтсвии с настройками
    //
    case Domain::DocumentObjectType::Project: {
        return d->isProjectInfoVisible;
    }
    case Domain::DocumentObjectType::RecycleBin: {
        return d->isRecycleBinVisible;
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

#include "comic_book_text_structure_model.h"

#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/comic_book_template.h>

#include <QApplication>


namespace BusinessLayer {

class ComicBookTextStructureModel::Implementation
{
public:
    ComicBookTextModel* comicBookModel = nullptr;
};


// ****


ComicBookTextStructureModel::ComicBookTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

ComicBookTextStructureModel::~ComicBookTextStructureModel() = default;

void ComicBookTextStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    d->comicBookModel = qobject_cast<ComicBookTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool ComicBookTextStructureModel::filterAcceptsRow(int _sourceRow,
                                                   const QModelIndex& _sourceParent) const
{
    if (d->comicBookModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->comicBookModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->comicBookModel->itemForIndex(itemIndex);

    //
    // Показываем папки, страницы и панели
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

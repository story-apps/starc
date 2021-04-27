#include "simple_text_structure_model.h"

#include <business_layer/model/text/text_model.h>
#include <business_layer/model/text/text_model_item.h>
#include <business_layer/model/text/text_model_text_item.h>
#include <business_layer/templates/text_template.h>

#include <QApplication>


namespace BusinessLayer
{

class SimpleTextStructureModel::Implementation
{
public:
    TextModel* textModel = nullptr;
};


// ****


SimpleTextStructureModel::SimpleTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent),
      d(new Implementation)
{
}

SimpleTextStructureModel::~SimpleTextStructureModel() = default;

void SimpleTextStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    if (d->textModel) {
        d->textModel->disconnect(this);
    }

    d->textModel = qobject_cast<TextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);

    if (d->textModel != nullptr) {
        //
        // FIXME: Это сделано из-за того, что при перемещении элементов внутри модели сценария
        //        в позицию 0, на вторую попытку происходит падение внутри QSortFilterModel,
        //        видимо не успевает происходить какая-то внутренняя магия при синхронном удалении
        //        и последующей вставки элементов в модели
        //
        connect(d->textModel, &TextModel::rowsRemoved, this, [] {
            QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        });
    }
}

bool SimpleTextStructureModel::filterAcceptsRow(int _sourceRow, const QModelIndex& _sourceParent) const
{
    if (d->textModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->textModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->textModel->itemForIndex(itemIndex);

    //
    // Показываем главы
    //
    if (item->type() == TextModelItemType::Chapter) {
        return true;
    }
    //
    // Остальное не показываем
    //
    return false;
}

} // namespace BusinessLayer

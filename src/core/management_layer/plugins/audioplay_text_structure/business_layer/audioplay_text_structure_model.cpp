#include "audioplay_text_structure_model.h"

#include <business_layer/model/audioplay/text/audioplay_text_model.h>
#include <business_layer/model/text/text_model_item.h>

#include <QApplication>


namespace BusinessLayer {

class AudioplayTextStructureModel::Implementation
{
public:
    AudioplayTextModel* audioplayModel = nullptr;
};


// ****


AudioplayTextStructureModel::AudioplayTextStructureModel(QObject* _parent)
    : QSortFilterProxyModel(_parent)
    , d(new Implementation)
{
}

AudioplayTextStructureModel::~AudioplayTextStructureModel() = default;

void AudioplayTextStructureModel::setSourceModel(QAbstractItemModel* _sourceModel)
{
    if (d->audioplayModel) {
        d->audioplayModel->disconnect(this);
    }

    d->audioplayModel = qobject_cast<AudioplayTextModel*>(_sourceModel);
    QSortFilterProxyModel::setSourceModel(_sourceModel);
}

bool AudioplayTextStructureModel::filterAcceptsRow(int _sourceRow,
                                                   const QModelIndex& _sourceParent) const
{
    if (d->audioplayModel == nullptr) {
        return false;
    }

    const auto itemIndex = d->audioplayModel->index(_sourceRow, 0, _sourceParent);
    const auto item = d->audioplayModel->itemForIndex(itemIndex);

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

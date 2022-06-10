#include "recycle_bin_model.h"


namespace BusinessLayer {

class RecycleBinModel::Implementation
{
public:
    int documentsToRemoveSize = 0;
};


RecycleBinModel::RecycleBinModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

RecycleBinModel::~RecycleBinModel()
{
}

int RecycleBinModel::documentsToRemoveSize() const
{
    return d->documentsToRemoveSize;
}

void RecycleBinModel::setDocumentsToRemoveSize(int _size)
{
    if (d->documentsToRemoveSize == _size) {
        return;
    }

    d->documentsToRemoveSize = _size;
    emit documentsToRemoveSizeChanged(d->documentsToRemoveSize);
}

void RecycleBinModel::initDocument()
{
}

void RecycleBinModel::clearDocument()
{
}

QByteArray RecycleBinModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

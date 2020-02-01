#include "recycle_bin_model.h"


namespace BusinessLayer
{

RecycleBinModel::RecycleBinModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
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

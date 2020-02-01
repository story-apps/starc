#include "screenplay_synopsis_model.h"


namespace BusinessLayer
{

ScreenplaySynopsisModel::ScreenplaySynopsisModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
}

void ScreenplaySynopsisModel::initDocument()
{
}

void ScreenplaySynopsisModel::clearDocument()
{
}

QByteArray ScreenplaySynopsisModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

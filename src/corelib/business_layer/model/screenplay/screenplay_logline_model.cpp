#include "screenplay_logline_model.h"


namespace BusinessLayer
{

ScreenplayLoglineModel::ScreenplayLoglineModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
}

void ScreenplayLoglineModel::initDocument()
{
}

void ScreenplayLoglineModel::clearDocument()
{
}

QByteArray ScreenplayLoglineModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

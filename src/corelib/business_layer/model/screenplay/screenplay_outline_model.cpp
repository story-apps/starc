#include "screenplay_outline_model.h"


namespace BusinessLayer
{

ScreenplayOutlineModel::ScreenplayOutlineModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
}

void ScreenplayOutlineModel::initDocument()
{
}

void ScreenplayOutlineModel::clearDocument()
{
}

QByteArray ScreenplayOutlineModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

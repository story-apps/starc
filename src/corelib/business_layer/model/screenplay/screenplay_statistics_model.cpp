#include "screenplay_statistics_model.h"


namespace BusinessLayer
{

ScreenplayStatisticsModel::ScreenplayStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
{
}

void ScreenplayStatisticsModel::initDocument()
{
}

void ScreenplayStatisticsModel::clearDocument()
{
}

QByteArray ScreenplayStatisticsModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

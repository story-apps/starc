#include "screenplay_series_statistics_model.h"


namespace BusinessLayer {

class ScreenplaySeriesStatisticsModel::Implementation
{
public:
};


ScreenplaySeriesStatisticsModel::ScreenplaySeriesStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ScreenplaySeriesStatisticsModel::~ScreenplaySeriesStatisticsModel()
{
}

void ScreenplaySeriesStatisticsModel::initDocument()
{
}

void ScreenplaySeriesStatisticsModel::clearDocument()
{
}

QByteArray ScreenplaySeriesStatisticsModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

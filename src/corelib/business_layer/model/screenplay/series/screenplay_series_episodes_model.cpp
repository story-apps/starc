#include "screenplay_series_episodes_model.h"


namespace BusinessLayer {

class ScreenplaySeriesEpisodesModel::Implementation
{
public:
};


ScreenplaySeriesEpisodesModel::ScreenplaySeriesEpisodesModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ScreenplaySeriesEpisodesModel::~ScreenplaySeriesEpisodesModel()
{
}

void ScreenplaySeriesEpisodesModel::initDocument()
{
}

void ScreenplaySeriesEpisodesModel::clearDocument()
{
}

QByteArray ScreenplaySeriesEpisodesModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

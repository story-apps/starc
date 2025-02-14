#include "screenplay_series_episodes_model.h"

#include "../text/screenplay_text_model.h"
#include "screenplay_series_information_model.h"


namespace BusinessLayer {

class ScreenplaySeriesEpisodesModel::Implementation
{
public:
    /**
     * @brief Модель информации о проекте
     */
    ScreenplaySeriesInformationModel* informationModel = nullptr;

    /**
     * @brief Список серий
     */
    QVector<ScreenplayTextModel*> episodes;
};


ScreenplaySeriesEpisodesModel::ScreenplaySeriesEpisodesModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ScreenplaySeriesEpisodesModel::~ScreenplaySeriesEpisodesModel()
{
}

void ScreenplaySeriesEpisodesModel::setInformationModel(ScreenplaySeriesInformationModel* _model)
{
    if (d->informationModel == _model) {
        return;
    }

    d->informationModel = _model;
}

ScreenplaySeriesInformationModel* ScreenplaySeriesEpisodesModel::informationModel() const
{
    return d->informationModel;
}

QVector<ScreenplayTextModel*> ScreenplaySeriesEpisodesModel::episodes() const
{
    return d->episodes;
}

void ScreenplaySeriesEpisodesModel::setEpisodes(const QVector<ScreenplayTextModel*>& _episodes)
{
    if (d->episodes == _episodes) {
        return;
    }

    d->episodes = _episodes;
    emit episodesChanged(_episodes);
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

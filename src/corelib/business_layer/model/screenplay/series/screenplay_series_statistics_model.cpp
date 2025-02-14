#include "screenplay_series_statistics_model.h"

#include "screenplay_series_episodes_model.h"
#include "screenplay_series_information_model.h"

#include <business_layer/plots/screenplay/series/screenplay_series_characters_activity_plot.h>
#include <business_layer/reports/screenplay/series/screenplay_series_cast_report.h>
#include <business_layer/reports/screenplay/series/screenplay_series_dialogues_report.h>
#include <business_layer/reports/screenplay/series/screenplay_series_location_report.h>
#include <business_layer/reports/screenplay/series/screenplay_series_scene_report.h>
#include <business_layer/reports/screenplay/series/screenplay_series_summary_report.h>


namespace BusinessLayer {

class ScreenplaySeriesStatisticsModel::Implementation
{
public:
    ScreenplaySeriesEpisodesModel* episodesModel = nullptr;

    ScreenplaySeriesSummaryReport summaryReport;
    ScreenplaySeriesSceneReport sceneReport;
    ScreenplaySeriesLocationReport locationReport;
    ScreenplaySeriesCastReport castReport;
    ScreenplaySeriesDialoguesReport dialoguesReport;
    //
    ScreenplaySeriesCharactersActivityPlot charactersActivityPlot;
};


ScreenplaySeriesStatisticsModel::ScreenplaySeriesStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ScreenplaySeriesStatisticsModel::~ScreenplaySeriesStatisticsModel()
{
}

QString ScreenplaySeriesStatisticsModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Statistics"), d->episodesModel->informationModel()->name());
}

ScreenplaySeriesEpisodesModel* ScreenplaySeriesStatisticsModel::episodesModel() const
{
    return d->episodesModel;
}

void ScreenplaySeriesStatisticsModel::setEpisodesModel(ScreenplaySeriesEpisodesModel* _model)
{
    d->episodesModel = _model;

    d->summaryReport.build(d->episodesModel);
}

void ScreenplaySeriesStatisticsModel::updateReports()
{
    if (d->episodesModel == nullptr) {
        return;
    }

    d->summaryReport.build(d->episodesModel);
    d->sceneReport.build(d->episodesModel);
    d->locationReport.build(d->episodesModel);
    d->castReport.build(d->episodesModel);
    d->dialoguesReport.build(d->episodesModel);
    d->charactersActivityPlot.build(d->episodesModel);
}

const ScreenplaySeriesSummaryReport& ScreenplaySeriesStatisticsModel::summaryReport() const
{
    return d->summaryReport;
}

const ScreenplaySeriesSceneReport& ScreenplaySeriesStatisticsModel::sceneReport() const
{
    return d->sceneReport;
}

void ScreenplaySeriesStatisticsModel::setSceneReportParameters(bool _showCharacters, int _sortBy)
{
    d->sceneReport.setParameters(_showCharacters, _sortBy);
    d->sceneReport.build(d->episodesModel);
}

const ScreenplaySeriesLocationReport& ScreenplaySeriesStatisticsModel::locationReport() const
{
    return d->locationReport;
}

void ScreenplaySeriesStatisticsModel::setLocationReportParameters(bool _extendedView, int _sortBy)
{
    d->locationReport.setParameters(_extendedView, _sortBy);
    d->locationReport.build(d->episodesModel);
}

const ScreenplaySeriesCastReport& ScreenplaySeriesStatisticsModel::castReport() const
{
    return d->castReport;
}

void ScreenplaySeriesStatisticsModel::setCastReportParameters(bool _showDetails, bool _showWords,
                                                              int _sortBy)
{
    d->castReport.setParameters(_showDetails, _showWords, _sortBy);
    d->castReport.build(d->episodesModel);
}

const ScreenplaySeriesDialoguesReport& ScreenplaySeriesStatisticsModel::dialoguesReport() const
{
    return d->dialoguesReport;
}

void ScreenplaySeriesStatisticsModel::setDialoguesReportParameters(
    const QVector<QString>& _visibleCharacters)
{
    d->dialoguesReport.setParameters(_visibleCharacters);
    d->dialoguesReport.build(d->episodesModel);
}

const ScreenplaySeriesCharactersActivityPlot& ScreenplaySeriesStatisticsModel::
    charactersActivityPlot() const
{
    return d->charactersActivityPlot;
}

void ScreenplaySeriesStatisticsModel::setCharactersActivityPlotParameters(
    const QVector<QString>& _visibleCharacters)
{
    d->charactersActivityPlot.setParameters(_visibleCharacters);
    d->charactersActivityPlot.build(d->episodesModel);
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

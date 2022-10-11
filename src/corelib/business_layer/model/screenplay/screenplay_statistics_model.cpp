#include "screenplay_statistics_model.h"

#include "text/screenplay_text_model.h"

#include <business_layer/plots/screenplay/screenplay_characters_activity_plot.h>
#include <business_layer/plots/screenplay/screenplay_structure_analysis_plot.h>
#include <business_layer/reports/screenplay/screenplay_cast_report.h>
#include <business_layer/reports/screenplay/screenplay_gender_report.h>
#include <business_layer/reports/screenplay/screenplay_location_report.h>
#include <business_layer/reports/screenplay/screenplay_scene_report.h>
#include <business_layer/reports/screenplay/screenplay_summary_report.h>


namespace BusinessLayer {

class ScreenplayStatisticsModel::Implementation
{
public:
    ScreenplayTextModel* textModel = nullptr;

    ScreenplaySummaryReport summaryReport;
    ScreenplaySceneReport sceneReport;
    ScreenplayLocationReport locationReport;
    ScreenplayCastReport castReport;
    ScreenplayGenderReport genderReport;
    //
    ScreenplayStructureAnalysisPlot structureAnalysisPlot;
    ScreenplayCharactersActivityPlot charactersActivityPlot;
};

ScreenplayStatisticsModel::ScreenplayStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ScreenplayStatisticsModel::~ScreenplayStatisticsModel() = default;

ScreenplayTextModel* ScreenplayStatisticsModel::textModel() const
{
    return d->textModel;
}

void ScreenplayStatisticsModel::setScreenplayTextModel(ScreenplayTextModel* _model)
{
    d->textModel = _model;

    d->summaryReport.build(d->textModel);
}

void ScreenplayStatisticsModel::updateReports()
{
    if (d->textModel == nullptr) {
        return;
    }

    d->summaryReport.build(d->textModel);
    d->sceneReport.build(d->textModel);
    d->castReport.build(d->textModel);
    d->locationReport.build(d->textModel);
    d->genderReport.build(d->textModel);
    d->structureAnalysisPlot.build(d->textModel);
    d->charactersActivityPlot.build(d->textModel);
}

const ScreenplaySummaryReport& ScreenplayStatisticsModel::summaryReport() const
{
    return d->summaryReport;
}

const ScreenplaySceneReport& ScreenplayStatisticsModel::sceneReport() const
{
    return d->sceneReport;
}

void ScreenplayStatisticsModel::setSceneReportParameters(int _sortBy)
{
    d->sceneReport.setParameters(_sortBy);
    d->sceneReport.build(d->textModel);
}

const ScreenplayLocationReport& ScreenplayStatisticsModel::locationReport() const
{
    return d->locationReport;
}

void ScreenplayStatisticsModel::setLocationReportParameters(int _sortBy)
{
    d->locationReport.setParameters(_sortBy);
    d->locationReport.build(d->textModel);
}

const ScreenplayCastReport& ScreenplayStatisticsModel::castReport() const
{
    return d->castReport;
}

void ScreenplayStatisticsModel::setCastReportParameters(int _sortBy)
{
    d->castReport.setParameters(_sortBy);
    d->castReport.build(d->textModel);
}

const ScreenplayGenderReport& ScreenplayStatisticsModel::genderReport() const
{
    return d->genderReport;
}

const ScreenplayStructureAnalysisPlot& ScreenplayStatisticsModel::structureAnalysisPlot() const
{
    return d->structureAnalysisPlot;
}

void ScreenplayStatisticsModel::setStructureAnalysisPlotParameters(bool _sceneDuration,
                                                                   bool _actionDuration,
                                                                   bool _dialoguesDuration,
                                                                   bool _charactersCount,
                                                                   bool _dialoguesCount)
{
    d->structureAnalysisPlot.setParameters(_sceneDuration, _actionDuration, _dialoguesDuration,
                                           _charactersCount, _dialoguesCount);
    d->structureAnalysisPlot.build(d->textModel);
}

const ScreenplayCharactersActivityPlot& ScreenplayStatisticsModel::charactersActivityPlot() const
{
    return d->charactersActivityPlot;
}

void ScreenplayStatisticsModel::setCharactersActivityPlotParameters(
    const QVector<QString>& _visibleCharacters)
{
    d->charactersActivityPlot.setParameters(_visibleCharacters);
    d->charactersActivityPlot.build(d->textModel);
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

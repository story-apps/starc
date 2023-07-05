#include "audioplay_statistics_model.h"

#include "audioplay_information_model.h"
#include "text/audioplay_text_model.h"

#include <business_layer/plots/audioplay/audioplay_characters_activity_plot.h>
#include <business_layer/plots/audioplay/audioplay_structure_analysis_plot.h>
#include <business_layer/reports/audioplay/audioplay_cast_report.h>
#include <business_layer/reports/audioplay/audioplay_dialogues_report.h>
#include <business_layer/reports/audioplay/audioplay_gender_report.h>
#include <business_layer/reports/audioplay/audioplay_location_report.h>
#include <business_layer/reports/audioplay/audioplay_scene_report.h>
#include <business_layer/reports/audioplay/audioplay_summary_report.h>


namespace BusinessLayer {

class AudioplayStatisticsModel::Implementation
{
public:
    AudioplayTextModel* textModel = nullptr;

    AudioplaySummaryReport summaryReport;
    AudioplaySceneReport sceneReport;
    AudioplayLocationReport locationReport;
    AudioplayCastReport castReport;
    AudioplayDialoguesReport dialoguesReport;
    AudioplayGenderReport genderReport;
    //
    AudioplayStructureAnalysisPlot structureAnalysisPlot;
    AudioplayCharactersActivityPlot charactersActivityPlot;
};

AudioplayStatisticsModel::AudioplayStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

AudioplayStatisticsModel::~AudioplayStatisticsModel() = default;

QString AudioplayStatisticsModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Statistics"), d->textModel->informationModel()->name());
}

AudioplayTextModel* AudioplayStatisticsModel::textModel() const
{
    return d->textModel;
}

void AudioplayStatisticsModel::setAudioplayTextModel(AudioplayTextModel* _model)
{
    d->textModel = _model;

    d->summaryReport.build(d->textModel);
}

void AudioplayStatisticsModel::updateReports()
{
    if (d->textModel == nullptr) {
        return;
    }

    d->summaryReport.build(d->textModel);
    d->sceneReport.build(d->textModel);
    d->castReport.build(d->textModel);
    d->dialoguesReport.build(d->textModel);
    d->locationReport.build(d->textModel);
    d->genderReport.build(d->textModel);
    d->structureAnalysisPlot.build(d->textModel);
    d->charactersActivityPlot.build(d->textModel);
}

const AudioplaySummaryReport& AudioplayStatisticsModel::summaryReport() const
{
    return d->summaryReport;
}

const AudioplaySceneReport& AudioplayStatisticsModel::sceneReport() const
{
    return d->sceneReport;
}

void AudioplayStatisticsModel::setSceneReportParameters(int _sortBy)
{
    d->sceneReport.setParameters(_sortBy);
    d->sceneReport.build(d->textModel);
}

const AudioplayLocationReport& AudioplayStatisticsModel::locationReport() const
{
    return d->locationReport;
}

void AudioplayStatisticsModel::setLocationReportParameters(int _sortBy)
{
    d->locationReport.setParameters(_sortBy);
    d->locationReport.build(d->textModel);
}

const AudioplayCastReport& AudioplayStatisticsModel::castReport() const
{
    return d->castReport;
}

void AudioplayStatisticsModel::setCastReportParameters(int _sortBy)
{
    d->castReport.setParameters(_sortBy);
    d->castReport.build(d->textModel);
}

const AudioplayDialoguesReport& AudioplayStatisticsModel::dialoguesReport() const
{
    return d->dialoguesReport;
}

void AudioplayStatisticsModel::setDialoguesReportParameters(
    const QVector<QString>& _visibleCharacters)
{
    d->dialoguesReport.setParameters(_visibleCharacters);
    d->dialoguesReport.build(d->textModel);
}

const AudioplayGenderReport& AudioplayStatisticsModel::genderReport() const
{
    return d->genderReport;
}

const AudioplayStructureAnalysisPlot& AudioplayStatisticsModel::structureAnalysisPlot() const
{
    return d->structureAnalysisPlot;
}

void AudioplayStatisticsModel::setStructureAnalysisPlotParameters(bool _sceneDuration,
                                                                  bool _actionDuration,
                                                                  bool _dialoguesDuration,
                                                                  bool _charactersCount,
                                                                  bool _dialoguesCount)
{
    d->structureAnalysisPlot.setParameters(_sceneDuration, _actionDuration, _dialoguesDuration,
                                           _charactersCount, _dialoguesCount);
    d->structureAnalysisPlot.build(d->textModel);
}

const AudioplayCharactersActivityPlot& AudioplayStatisticsModel::charactersActivityPlot() const
{
    return d->charactersActivityPlot;
}

void AudioplayStatisticsModel::setCharactersActivityPlotParameters(
    const QVector<QString>& _visibleCharacters)
{
    d->charactersActivityPlot.setParameters(_visibleCharacters);
    d->charactersActivityPlot.build(d->textModel);
}

void AudioplayStatisticsModel::initDocument()
{
}

void AudioplayStatisticsModel::clearDocument()
{
}

QByteArray AudioplayStatisticsModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

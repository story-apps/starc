#include "audioplay_statistics_model.h"

#include "text/audioplay_text_model.h"

#include <business_layer/reports/audioplay/audioplay_summary_report.h>


namespace BusinessLayer {

class AudioplayStatisticsModel::Implementation
{
public:
    AudioplayTextModel* textModel = nullptr;

    /**
     * @brief Сводный отчёт
     */
    AudioplaySummaryReport summaryReport;
};

AudioplayStatisticsModel::AudioplayStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

AudioplayStatisticsModel::~AudioplayStatisticsModel() = default;

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
}

const AudioplaySummaryReport& AudioplayStatisticsModel::summaryReport() const
{
    return d->summaryReport;
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

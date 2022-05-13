#include "stageplay_statistics_model.h"

#include "text/stageplay_text_model.h"

#include <business_layer/reports/stageplay/stageplay_summary_report.h>


namespace BusinessLayer {

class StageplayStatisticsModel::Implementation
{
public:
    StageplayTextModel* textModel = nullptr;

    /**
     * @brief Сводный отчёт
     */
    StageplaySummaryReport summaryReport;
};

StageplayStatisticsModel::StageplayStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

StageplayStatisticsModel::~StageplayStatisticsModel() = default;

void StageplayStatisticsModel::setStageplayTextModel(StageplayTextModel* _model)
{
    d->textModel = _model;

    d->summaryReport.build(d->textModel);
}

void StageplayStatisticsModel::updateReports()
{
    if (d->textModel == nullptr) {
        return;
    }

    d->summaryReport.build(d->textModel);
}

const StageplaySummaryReport& StageplayStatisticsModel::summaryReport() const
{
    return d->summaryReport;
}

void StageplayStatisticsModel::initDocument()
{
}

void StageplayStatisticsModel::clearDocument()
{
}

QByteArray StageplayStatisticsModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

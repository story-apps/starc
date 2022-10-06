#include "screenplay_statistics_model.h"

#include "text/screenplay_text_model.h"

#include <business_layer/reports/screenplay/screenplay_cast_report.h>
#include <business_layer/reports/screenplay/screenplay_gender_report.h>
#include <business_layer/reports/screenplay/screenplay_scene_report.h>
#include <business_layer/reports/screenplay/screenplay_summary_report.h>


namespace BusinessLayer {

class ScreenplayStatisticsModel::Implementation
{
public:
    ScreenplayTextModel* textModel = nullptr;

    ScreenplaySummaryReport summaryReport;
    ScreenplaySceneReport sceneReport;
    ScreenplayCastReport castReport;
    ScreenplayGenderReport genderReport;
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
    d->genderReport.build(d->textModel);
}

const ScreenplaySummaryReport& ScreenplayStatisticsModel::summaryReport() const
{
    return d->summaryReport;
}

const ScreenplaySceneReport& ScreenplayStatisticsModel::sceneReport() const
{
    return d->sceneReport;
}

const ScreenplayCastReport& ScreenplayStatisticsModel::castReport() const
{
    return d->castReport;
}

const ScreenplayGenderReport& ScreenplayStatisticsModel::genderReport() const
{
    return d->genderReport;
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

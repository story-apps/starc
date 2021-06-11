#include "screenplay_statistics_model.h"

#include "text/screenplay_text_model.h"

#include <business_layer/reports/screenplay/summary_report.h>


namespace BusinessLayer {

class ScreenplayStatisticsModel::Implementation
{
public:
    ScreenplayTextModel* textModel = nullptr;

    /**
     * @brief Сводный отчёт
     */
    ScreenplaySummaryReport summaryReport;
};

ScreenplayStatisticsModel::ScreenplayStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ScreenplayStatisticsModel::~ScreenplayStatisticsModel() = default;

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
}

const ScreenplaySummaryReport& ScreenplayStatisticsModel::summaryReport() const
{
    return d->summaryReport;
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

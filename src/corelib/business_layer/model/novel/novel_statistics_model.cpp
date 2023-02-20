#include "novel_statistics_model.h"

#include "novel_information_model.h"
#include "text/novel_text_model.h"

#include <business_layer/reports/novel/novel_summary_report.h>


namespace BusinessLayer {

class NovelStatisticsModel::Implementation
{
public:
    NovelTextModel* textModel = nullptr;

    /**
     * @brief Сводный отчёт
     */
    NovelSummaryReport summaryReport;
};

NovelStatisticsModel::NovelStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

NovelStatisticsModel::~NovelStatisticsModel() = default;

QString NovelStatisticsModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Statistics"), d->textModel->informationModel()->name());
}

void NovelStatisticsModel::setNovelTextModel(NovelTextModel* _model)
{
    d->textModel = _model;

    d->summaryReport.build(d->textModel);
}

void NovelStatisticsModel::updateReports()
{
    if (d->textModel == nullptr) {
        return;
    }

    d->summaryReport.build(d->textModel);
}

const NovelSummaryReport& NovelStatisticsModel::summaryReport() const
{
    return d->summaryReport;
}

void NovelStatisticsModel::initDocument()
{
}

void NovelStatisticsModel::clearDocument()
{
}

QByteArray NovelStatisticsModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

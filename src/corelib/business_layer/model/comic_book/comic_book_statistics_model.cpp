#include "comic_book_statistics_model.h"

#include "comic_book_information_model.h"
#include "text/comic_book_text_model.h"

#include <business_layer/reports/comic_book/comic_book_summary_report.h>


namespace BusinessLayer {

class ComicBookStatisticsModel::Implementation
{
public:
    ComicBookTextModel* textModel = nullptr;

    /**
     * @brief Сводный отчёт
     */
    ComicBookSummaryReport summaryReport;
};

ComicBookStatisticsModel::ComicBookStatisticsModel(QObject* _parent)
    : AbstractModel({}, _parent)
    , d(new Implementation)
{
}

ComicBookStatisticsModel::~ComicBookStatisticsModel() = default;

QString ComicBookStatisticsModel::documentName() const
{
    return QString("%1 | %2").arg(tr("Statistics"), d->textModel->informationModel()->name());
}

void ComicBookStatisticsModel::setComicBookTextModel(ComicBookTextModel* _model)
{
    d->textModel = _model;

    d->summaryReport.build(d->textModel);
}

void ComicBookStatisticsModel::updateReports()
{
    if (d->textModel == nullptr) {
        return;
    }

    d->summaryReport.build(d->textModel);
}

const ComicBookSummaryReport& ComicBookStatisticsModel::summaryReport() const
{
    return d->summaryReport;
}

void ComicBookStatisticsModel::initDocument()
{
}

void ComicBookStatisticsModel::clearDocument()
{
}

QByteArray ComicBookStatisticsModel::toXml() const
{
    return {};
}

} // namespace BusinessLayer

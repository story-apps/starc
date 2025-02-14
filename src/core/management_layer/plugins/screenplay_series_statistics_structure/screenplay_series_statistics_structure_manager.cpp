#include "screenplay_series_statistics_structure_manager.h"

#include "screenplay_series_statistics_structure_view.h"

#include <business_layer/model/screenplay/series/screenplay_series_episodes_model.h>
#include <business_layer/model/screenplay/series/screenplay_series_information_model.h>
#include <business_layer/model/screenplay/series/screenplay_series_statistics_model.h>


namespace ManagementLayer {

class ScreenplayStatisticsStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayStatisticsStructureView* createView();


    /**
     * @brief Текущая модель сценария
     */
    QPointer<BusinessLayer::ScreenplaySeriesStatisticsModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayStatisticsStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayStatisticsStructureView*> allViews;
};

ScreenplayStatisticsStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ScreenplayStatisticsStructureView* ScreenplayStatisticsStructureManager::Implementation::
    createView()
{
    allViews.append(new Ui::ScreenplayStatisticsStructureView);
    return allViews.last();
}


// ****


ScreenplayStatisticsStructureManager::ScreenplayStatisticsStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayStatisticsStructureView::currentReportIndexChanged, this,
            &ScreenplayStatisticsStructureManager::currentReportIndexChanged);
    connect(d->view, &Ui::ScreenplayStatisticsStructureView::currentPlotIndexChanged, this,
            &ScreenplayStatisticsStructureManager::currentPlotIndexChanged);
}

ScreenplayStatisticsStructureManager::~ScreenplayStatisticsStructureManager() = default;

QObject* ScreenplayStatisticsStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* ScreenplayStatisticsStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayStatisticsStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* ScreenplayStatisticsStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* ScreenplayStatisticsStructureManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* ScreenplayStatisticsStructureManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void ScreenplayStatisticsStructureManager::resetModels()
{
    setModel(nullptr);
}

void ScreenplayStatisticsStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr && d->model->episodesModel() != nullptr
        && d->model->episodesModel()->informationModel() != nullptr) {
        disconnect(d->model->episodesModel()->informationModel(),
                   &BusinessLayer::ScreenplaySeriesInformationModel::nameChanged, d->view, nullptr);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::ScreenplaySeriesStatisticsModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr && d->model->episodesModel() != nullptr
        && d->model->episodesModel()->informationModel() != nullptr) {
        auto updateTitle = [this] {
            d->view->setTitle(QString("%1 | %2").arg(
                tr("Statistics"), d->model->episodesModel()->informationModel()->name()));
        };
        updateTitle();
        connect(d->model->episodesModel()->informationModel(),
                &BusinessLayer::ScreenplaySeriesInformationModel::nameChanged, d->view,
                updateTitle);
    }
}

} // namespace ManagementLayer

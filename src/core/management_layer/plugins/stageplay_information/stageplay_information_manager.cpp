#include "stageplay_information_manager.h"

#include "stageplay_information_view.h"

#include <business_layer/model/stageplay/stageplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class StageplayInformationManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::StageplayInformationView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::StageplayInformationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::StageplayInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::StageplayInformationView*> allViews;
};

StageplayInformationManager::Implementation::Implementation()
{
    view = createView();
}

Ui::StageplayInformationView* StageplayInformationManager::Implementation::createView()
{
    allViews.append(new Ui::StageplayInformationView);
    return allViews.last();
}


// ****


StageplayInformationManager::StageplayInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

StageplayInformationManager::~StageplayInformationManager() = default;

void StageplayInformationManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        d->view->disconnect(d->model);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::StageplayInformationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setName(d->model->name());
        d->view->setTagline(d->model->tagline());
        d->view->setLogline(d->model->logline());
        d->view->setTitlePageVisible(d->model->titlePageVisible());
        d->view->setSynopsisVisible(d->model->synopsisVisible());
        d->view->setStageplayTextVisible(d->model->stageplayTextVisible());
        d->view->setStageplayStatisticsVisible(d->model->stageplayStatisticsVisible());

        connect(d->model, &BusinessLayer::StageplayInformationModel::nameChanged, d->view,
                &Ui::StageplayInformationView::setName);
        connect(d->model, &BusinessLayer::StageplayInformationModel::taglineChanged, d->view,
                &Ui::StageplayInformationView::setTagline);
        connect(d->model, &BusinessLayer::StageplayInformationModel::loglineChanged, d->view,
                &Ui::StageplayInformationView::setLogline);
        connect(d->model, &BusinessLayer::StageplayInformationModel::titlePageVisibleChanged,
                d->view, &Ui::StageplayInformationView::setTitlePageVisible);
        connect(d->model, &BusinessLayer::StageplayInformationModel::synopsisVisibleChanged,
                d->view, &Ui::StageplayInformationView::setSynopsisVisible);
        connect(d->model, &BusinessLayer::StageplayInformationModel::stageplayTextVisibleChanged,
                d->view, &Ui::StageplayInformationView::setStageplayTextVisible);
        connect(d->model,
                &BusinessLayer::StageplayInformationModel::stageplayStatisticsVisibleChanged,
                d->view, &Ui::StageplayInformationView::setStageplayStatisticsVisible);
        //
        connect(d->view, &Ui::StageplayInformationView::nameChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setName);
        connect(d->view, &Ui::StageplayInformationView::taglineChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setTagline);
        connect(d->view, &Ui::StageplayInformationView::loglineChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setLogline);
        connect(d->view, &Ui::StageplayInformationView::titlePageVisibleChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setTitlePageVisible);
        connect(d->view, &Ui::StageplayInformationView::synopsisVisibleChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setSynopsisVisible);
        connect(d->view, &Ui::StageplayInformationView::stageplayTextVisibleChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setStageplayTextVisible);
        connect(d->view, &Ui::StageplayInformationView::stageplayStatisticsVisibleChanged, d->model,
                &BusinessLayer::StageplayInformationModel::setStageplayStatisticsVisible);
    }
}

Ui::IDocumentView* StageplayInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* StageplayInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer

#include "screenplay_information_manager.h"

#include "screenplay_information_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ScreenplayInformationManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayInformationView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    BusinessLayer::ScreenplayInformationModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayInformationView*> allViews;
};

ScreenplayInformationManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ScreenplayInformationView* ScreenplayInformationManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayInformationView);
    return allViews.last();
}


// ****


ScreenplayInformationManager::ScreenplayInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ScreenplayInformationManager::~ScreenplayInformationManager() = default;

void ScreenplayInformationManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setName(d->model->name());
        d->view->setTagline(d->model->tagline());
        d->view->setLogline(d->model->logline());
        d->view->setTitlePageVisible(d->model->titlePageVisible());
        d->view->setSynopsisVisible(d->model->synopsisVisible());
        d->view->setTreatmentVisible(d->model->treatmentVisible());
        d->view->setScreenplayTextVisible(d->model->screenplayTextVisible());
        d->view->setScreenplayStatisticsVisible(d->model->screenplayStatisticsVisible());

        connect(d->model, &BusinessLayer::ScreenplayInformationModel::nameChanged, d->view,
                &Ui::ScreenplayInformationView::setName);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::taglineChanged, d->view,
                &Ui::ScreenplayInformationView::setTagline);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::loglineChanged, d->view,
                &Ui::ScreenplayInformationView::setLogline);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::titlePageVisibleChanged,
                d->view, &Ui::ScreenplayInformationView::setTitlePageVisible);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::synopsisVisibleChanged,
                d->view, &Ui::ScreenplayInformationView::setSynopsisVisible);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::treatmentVisibleChanged,
                d->view, &Ui::ScreenplayInformationView::setTreatmentVisible);
        connect(d->model, &BusinessLayer::ScreenplayInformationModel::screenplayTextVisibleChanged,
                d->view, &Ui::ScreenplayInformationView::setScreenplayTextVisible);
        connect(d->model,
                &BusinessLayer::ScreenplayInformationModel::screenplayStatisticsVisibleChanged,
                d->view, &Ui::ScreenplayInformationView::setScreenplayStatisticsVisible);
        //
        connect(d->view, &Ui::ScreenplayInformationView::nameChanged, d->model,
                &BusinessLayer::ScreenplayInformationModel::setName);
        connect(d->view, &Ui::ScreenplayInformationView::taglineChanged, d->model,
                &BusinessLayer::ScreenplayInformationModel::setTagline);
        connect(d->view, &Ui::ScreenplayInformationView::loglineChanged, d->model,
                &BusinessLayer::ScreenplayInformationModel::setLogline);
        connect(d->view, &Ui::ScreenplayInformationView::titlePageVisibleChanged, d->model,
                &BusinessLayer::ScreenplayInformationModel::setTitlePageVisible);
        connect(d->view, &Ui::ScreenplayInformationView::synopsisVisibleChanged, d->model,
                &BusinessLayer::ScreenplayInformationModel::setSynopsisVisible);
        connect(d->view, &Ui::ScreenplayInformationView::treatmentVisibleChanged, d->model,
                &BusinessLayer::ScreenplayInformationModel::setTreatmentVisible);
        connect(d->view, &Ui::ScreenplayInformationView::screenplayTextVisibleChanged, d->model,
                &BusinessLayer::ScreenplayInformationModel::setScreenplayTextVisible);
        connect(d->view, &Ui::ScreenplayInformationView::screenplayStatisticsVisibleChanged,
                d->model,
                &BusinessLayer::ScreenplayInformationModel::setScreenplayStatisticsVisible);
    }
}

QWidget* ScreenplayInformationManager::view()
{
    return d->view;
}

QWidget* ScreenplayInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer

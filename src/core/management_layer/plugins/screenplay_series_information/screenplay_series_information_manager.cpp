#include "screenplay_series_information_manager.h"

#include "screenplay_series_information_view.h"

#include <business_layer/model/screenplay/series/screenplay_series_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ScreenplaySeriesInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::ScreenplaySeriesInformationView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model,
                         Ui::ScreenplaySeriesInformationView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplaySeriesInformationView* view = nullptr;
    Ui::ScreenplaySeriesInformationView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ScreenplaySeriesInformationView> view;
        QPointer<BusinessLayer::ScreenplaySeriesInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::ScreenplaySeriesInformationView* ScreenplaySeriesInformationManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ScreenplaySeriesInformationView;
    setModelForView(_model, view);
    return view;
}

void ScreenplaySeriesInformationManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplaySeriesInformationView* _view)
{
    constexpr int invalidIndex = -1;
    int viewIndex = invalidIndex;
    for (int index = 0; index < allViews.size(); ++index) {
        if (allViews[index].view == _view) {
            if (allViews[index].model == _model) {
                return;
            }

            viewIndex = index;
            break;
        }
    }

    //
    // Разрываем соединения со старой моделью
    //
    if (viewIndex != invalidIndex && allViews[viewIndex].model != nullptr) {
        _view->disconnect(allViews[viewIndex].model);
        allViews[viewIndex].model->disconnect(_view);
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::ScreenplaySeriesInformationModel*>(_model);

    //
    // Обновляем связь представления с моделью
    //
    if (viewIndex != invalidIndex) {
        allViews[viewIndex].model = model;
    }
    //
    // Или сохраняем связь представления с моделью
    //
    else {
        allViews.append({ _view, model });
    }

    //
    // Настраиваем соединения с новой моделью
    //
    if (model != nullptr) {
        _view->setName(model->name());
        _view->setTagline(model->tagline());
        _view->setLogline(model->logline());
        _view->setTitlePageVisible(model->titlePageVisible());
        _view->setSynopsisVisible(model->synopsisVisible());
        _view->setTreatmentVisible(model->treatmentVisible());
        _view->setScreenplayVisible(model->screenplayTextVisible());
        _view->setStatisticsVisible(model->screenplayStatisticsVisible());

        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::nameChanged, _view,
                &Ui::ScreenplaySeriesInformationView::setName);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::taglineChanged, _view,
                &Ui::ScreenplaySeriesInformationView::setTagline);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::loglineChanged, _view,
                &Ui::ScreenplaySeriesInformationView::setLogline);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::titlePageVisibleChanged,
                _view, &Ui::ScreenplaySeriesInformationView::setTitlePageVisible);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::synopsisVisibleChanged,
                _view, &Ui::ScreenplaySeriesInformationView::setSynopsisVisible);
        connect(model, &BusinessLayer::ScreenplaySeriesInformationModel::treatmentVisibleChanged,
                _view, &Ui::ScreenplaySeriesInformationView::setTreatmentVisible);
        connect(model,
                &BusinessLayer::ScreenplaySeriesInformationModel::screenplayTextVisibleChanged,
                _view, &Ui::ScreenplaySeriesInformationView::setScreenplayVisible);
        connect(
            model,
            &BusinessLayer::ScreenplaySeriesInformationModel::screenplayStatisticsVisibleChanged,
            _view, &Ui::ScreenplaySeriesInformationView::setStatisticsVisible);
        //
        connect(_view, &Ui::ScreenplaySeriesInformationView::nameChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setName);
        connect(_view, &Ui::ScreenplaySeriesInformationView::taglineChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setTagline);
        connect(_view, &Ui::ScreenplaySeriesInformationView::loglineChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setLogline);
        connect(_view, &Ui::ScreenplaySeriesInformationView::titlePageVisibleChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setTitlePageVisible);
        connect(_view, &Ui::ScreenplaySeriesInformationView::synopsisVisibleChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setSynopsisVisible);
        connect(_view, &Ui::ScreenplaySeriesInformationView::treatmentVisibleChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setTreatmentVisible);
        connect(_view, &Ui::ScreenplaySeriesInformationView::screenplayVisibleChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setScreenplayTextVisible);
        connect(_view, &Ui::ScreenplaySeriesInformationView::statisticsVisibleChanged, model,
                &BusinessLayer::ScreenplaySeriesInformationModel::setScreenplayStatisticsVisible);
    }
}


// ****


ScreenplaySeriesInformationManager::ScreenplaySeriesInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ScreenplaySeriesInformationManager::~ScreenplaySeriesInformationManager() = default;

Ui::IDocumentView* ScreenplaySeriesInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplaySeriesInformationManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ScreenplaySeriesInformationManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ScreenplaySeriesInformationManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ScreenplaySeriesInformationManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ScreenplaySeriesInformationManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ScreenplaySeriesInformationManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer

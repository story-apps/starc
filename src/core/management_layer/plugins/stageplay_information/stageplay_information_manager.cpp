#include "stageplay_information_manager.h"

#include "stageplay_information_view.h"

#include <business_layer/model/stageplay/stageplay_information_model.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class StageplayInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::StageplayInformationView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::StageplayInformationView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::StageplayInformationView* view = nullptr;
    Ui::StageplayInformationView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::StageplayInformationView> view;
        QPointer<BusinessLayer::StageplayInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::StageplayInformationView* StageplayInformationManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::StageplayInformationView;
    setModelForView(_model, view);
    return view;
}

void StageplayInformationManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::StageplayInformationView* _view)
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
    }

    //
    // Определяем новую модель
    //
    auto model = qobject_cast<BusinessLayer::StageplayInformationModel*>(_model);

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
        _view->setStageplayTextVisible(model->stageplayTextVisible());
        _view->setStageplayStatisticsVisible(model->stageplayStatisticsVisible());

        connect(model, &BusinessLayer::StageplayInformationModel::nameChanged, _view,
                &Ui::StageplayInformationView::setName);
        connect(model, &BusinessLayer::StageplayInformationModel::taglineChanged, _view,
                &Ui::StageplayInformationView::setTagline);
        connect(model, &BusinessLayer::StageplayInformationModel::loglineChanged, _view,
                &Ui::StageplayInformationView::setLogline);
        connect(model, &BusinessLayer::StageplayInformationModel::titlePageVisibleChanged, _view,
                &Ui::StageplayInformationView::setTitlePageVisible);
        connect(model, &BusinessLayer::StageplayInformationModel::synopsisVisibleChanged, _view,
                &Ui::StageplayInformationView::setSynopsisVisible);
        connect(model, &BusinessLayer::StageplayInformationModel::stageplayTextVisibleChanged,
                _view, &Ui::StageplayInformationView::setStageplayTextVisible);
        connect(model, &BusinessLayer::StageplayInformationModel::stageplayStatisticsVisibleChanged,
                _view, &Ui::StageplayInformationView::setStageplayStatisticsVisible);
        //
        connect(_view, &Ui::StageplayInformationView::nameChanged, model,
                &BusinessLayer::StageplayInformationModel::setName);
        connect(_view, &Ui::StageplayInformationView::taglineChanged, model,
                &BusinessLayer::StageplayInformationModel::setTagline);
        connect(_view, &Ui::StageplayInformationView::loglineChanged, model,
                &BusinessLayer::StageplayInformationModel::setLogline);
        connect(_view, &Ui::StageplayInformationView::titlePageVisibleChanged, model,
                &BusinessLayer::StageplayInformationModel::setTitlePageVisible);
        connect(_view, &Ui::StageplayInformationView::synopsisVisibleChanged, model,
                &BusinessLayer::StageplayInformationModel::setSynopsisVisible);
        connect(_view, &Ui::StageplayInformationView::stageplayTextVisibleChanged, model,
                &BusinessLayer::StageplayInformationModel::setStageplayTextVisible);
        connect(_view, &Ui::StageplayInformationView::stageplayStatisticsVisibleChanged, model,
                &BusinessLayer::StageplayInformationModel::setStageplayStatisticsVisible);
    }
}


// ****


StageplayInformationManager::StageplayInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

StageplayInformationManager::~StageplayInformationManager() = default;

Ui::IDocumentView* StageplayInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* StageplayInformationManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* StageplayInformationManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* StageplayInformationManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* StageplayInformationManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void StageplayInformationManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void StageplayInformationManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer

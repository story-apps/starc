#include "screenplay_information_manager.h"

#include "screenplay_information_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <domain/document_object.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ScreenplayInformationManager::Implementation
{
public:
    explicit Implementation(ScreenplayInformationManager* _q);

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayInformationView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model,
                         Ui::ScreenplayInformationView* _view);


    ScreenplayInformationManager* q = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayInformationView* view = nullptr;
    Ui::ScreenplayInformationView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ScreenplayInformationView> view;
        QPointer<BusinessLayer::ScreenplayInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

ScreenplayInformationManager::Implementation::Implementation(ScreenplayInformationManager* _q)
    : q(_q)
{
}

Ui::ScreenplayInformationView* ScreenplayInformationManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ScreenplayInformationView;
    setModelForView(_model, view);
    return view;
}

void ScreenplayInformationManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ScreenplayInformationView* _view)
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
    auto model = qobject_cast<BusinessLayer::ScreenplayInformationModel*>(_model);

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
        _view->setScreenplayTextVisible(model->screenplayTextVisible());
        _view->setScreenplayStatisticsVisible(model->screenplayStatisticsVisible());

        connect(model, &BusinessLayer::ScreenplayInformationModel::nameChanged, _view,
                &Ui::ScreenplayInformationView::setName);
        connect(model, &BusinessLayer::ScreenplayInformationModel::taglineChanged, _view,
                &Ui::ScreenplayInformationView::setTagline);
        connect(model, &BusinessLayer::ScreenplayInformationModel::loglineChanged, _view,
                &Ui::ScreenplayInformationView::setLogline);
        connect(model, &BusinessLayer::ScreenplayInformationModel::titlePageVisibleChanged, _view,
                &Ui::ScreenplayInformationView::setTitlePageVisible);
        connect(model, &BusinessLayer::ScreenplayInformationModel::synopsisVisibleChanged, _view,
                &Ui::ScreenplayInformationView::setSynopsisVisible);
        connect(model, &BusinessLayer::ScreenplayInformationModel::treatmentVisibleChanged, _view,
                &Ui::ScreenplayInformationView::setTreatmentVisible);
        connect(model, &BusinessLayer::ScreenplayInformationModel::screenplayTextVisibleChanged,
                _view, &Ui::ScreenplayInformationView::setScreenplayTextVisible);
        connect(model,
                &BusinessLayer::ScreenplayInformationModel::screenplayStatisticsVisibleChanged,
                _view, &Ui::ScreenplayInformationView::setScreenplayStatisticsVisible);
        //
        connect(_view, &Ui::ScreenplayInformationView::nameChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setName);
        connect(_view, &Ui::ScreenplayInformationView::taglineChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setTagline);
        connect(_view, &Ui::ScreenplayInformationView::loglineChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setLogline);
        connect(_view, &Ui::ScreenplayInformationView::titlePageVisibleChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setTitlePageVisible);
        connect(_view, &Ui::ScreenplayInformationView::synopsisVisibleChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setSynopsisVisible);
        connect(_view, &Ui::ScreenplayInformationView::treatmentVisibleChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setTreatmentVisible);
        connect(_view, &Ui::ScreenplayInformationView::screenplayTextVisibleChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setScreenplayTextVisible);
        connect(_view, &Ui::ScreenplayInformationView::screenplayStatisticsVisibleChanged, model,
                &BusinessLayer::ScreenplayInformationModel::setScreenplayStatisticsVisible);

        //
        connect(_view, &Ui::ScreenplayInformationView::sendDocumentToReviewRequested, q,
                [this, model](const QString& _comment) {
                    emit q->sendDocumentToReviewRequested(model->document()->uuid(), _comment);
                });
    }
}


// ****


ScreenplayInformationManager::ScreenplayInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation(this))
{
}

ScreenplayInformationManager::~ScreenplayInformationManager() = default;

QObject* ScreenplayInformationManager::asQObject()
{
    return this;
}

Ui::IDocumentView* ScreenplayInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayInformationManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ScreenplayInformationManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayInformationManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ScreenplayInformationManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ScreenplayInformationManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ScreenplayInformationManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer

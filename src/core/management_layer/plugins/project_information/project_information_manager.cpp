#include "project_information_manager.h"

#include "project_information_view.h"

#include <business_layer/model/project/project_information_model.h>
#include <ui/widgets/image/image_cropping_dialog.h>

#include <QApplication>
#include <QFileDialog>


namespace ManagementLayer {

class ProjectInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::ProjectInformationView* createView(BusinessLayer::AbstractModel* _model);

    /**
     * @brief Связать заданную модель и представление
     */
    void setModelForView(BusinessLayer::AbstractModel* _model, Ui::ProjectInformationView* _view);


    /**
     * @brief Предаставление для основного окна
     */
    Ui::ProjectInformationView* view = nullptr;
    Ui::ProjectInformationView* secondaryView = nullptr;

    /**
     * @brief Все созданные представления с моделями, которые в них отображаются
     */
    struct ViewAndModel {
        QPointer<Ui::ProjectInformationView> view;
        QPointer<BusinessLayer::ProjectInformationModel> model;
    };
    QVector<ViewAndModel> allViews;
};

Ui::ProjectInformationView* ProjectInformationManager::Implementation::createView(
    BusinessLayer::AbstractModel* _model)
{
    auto view = new Ui::ProjectInformationView;
    setModelForView(_model, view);
    return view;
}

void ProjectInformationManager::Implementation::setModelForView(
    BusinessLayer::AbstractModel* _model, Ui::ProjectInformationView* _view)
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
    auto model = qobject_cast<BusinessLayer::ProjectInformationModel*>(_model);

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
        _view->setLogline(model->logline());
        _view->setCover(model->cover());

        connect(model, &BusinessLayer::ProjectInformationModel::nameChanged, _view,
                &Ui::ProjectInformationView::setName);
        connect(model, &BusinessLayer::ProjectInformationModel::loglineChanged, _view,
                &Ui::ProjectInformationView::setLogline);
        connect(model, &BusinessLayer::ProjectInformationModel::coverChanged, _view,
                &Ui::ProjectInformationView::setCover);
        //
        connect(_view, &Ui::ProjectInformationView::nameChanged, model,
                &BusinessLayer::ProjectInformationModel::setName);
        connect(_view, &Ui::ProjectInformationView::loglineChanged, model,
                &BusinessLayer::ProjectInformationModel::setLogline);
        connect(_view, &Ui::ProjectInformationView::coverChanged, model,
                &BusinessLayer::ProjectInformationModel::setCover);
    }
}


// ****


ProjectInformationManager::ProjectInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ProjectInformationManager::~ProjectInformationManager() = default;

Ui::IDocumentView* ProjectInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* ProjectInformationManager::view(BusinessLayer::AbstractModel* _model)
{
    if (d->view == nullptr) {
        d->view = d->createView(_model);
    } else {
        d->setModelForView(_model, d->view);
    }

    return d->view;
}

Ui::IDocumentView* ProjectInformationManager::secondaryView()
{
    return d->secondaryView;
}

Ui::IDocumentView* ProjectInformationManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    if (d->secondaryView == nullptr) {
        d->secondaryView = d->createView(_model);
    } else {
        d->setModelForView(_model, d->secondaryView);
    }

    return d->secondaryView;
}

Ui::IDocumentView* ProjectInformationManager::createView(BusinessLayer::AbstractModel* _model)
{
    return d->createView(_model);
}

void ProjectInformationManager::resetModels()
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        d->setModelForView(nullptr, viewAndModel.view);
    }
}

void ProjectInformationManager::setEditingMode(DocumentEditingMode _mode)
{
    for (auto& viewAndModel : d->allViews) {
        if (viewAndModel.view.isNull()) {
            continue;
        }

        viewAndModel.view->setEditingMode(_mode);
    }
}

} // namespace ManagementLayer

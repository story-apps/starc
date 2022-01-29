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
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ProjectInformationView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    QPointer<BusinessLayer::ProjectInformationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ProjectInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ProjectInformationView*> allViews;
};

ProjectInformationManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ProjectInformationView* ProjectInformationManager::Implementation::createView()
{
    allViews.append(new Ui::ProjectInformationView);
    return allViews.last();
}


// ****


ProjectInformationManager::ProjectInformationManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
}

ProjectInformationManager::~ProjectInformationManager() = default;

void ProjectInformationManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ProjectInformationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        d->view->setName(d->model->name());
        d->view->setLogline(d->model->logline());
        d->view->setCover(d->model->cover());

        connect(d->model, &BusinessLayer::ProjectInformationModel::nameChanged, d->view,
                &Ui::ProjectInformationView::setName);
        connect(d->model, &BusinessLayer::ProjectInformationModel::loglineChanged, d->view,
                &Ui::ProjectInformationView::setLogline);
        connect(d->model, &BusinessLayer::ProjectInformationModel::coverChanged, d->view,
                &Ui::ProjectInformationView::setCover);
        //
        connect(d->view, &Ui::ProjectInformationView::nameChanged, d->model,
                &BusinessLayer::ProjectInformationModel::setName);
        connect(d->view, &Ui::ProjectInformationView::loglineChanged, d->model,
                &BusinessLayer::ProjectInformationModel::setLogline);
        connect(d->view, &Ui::ProjectInformationView::coverChanged, d->model,
                &BusinessLayer::ProjectInformationModel::setCover);
    }
}

Ui::IDocumentView* ProjectInformationManager::view()
{
    return d->view;
}

Ui::IDocumentView* ProjectInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer

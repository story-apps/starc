#include "project_information_manager.h"

#include "project_information_view.h"

#include <business_layer/model/project_information/project_information_model.h>


namespace ManagementLayer
{

class ProjectInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление для основного окна
     */
    void initView();

    /**
     * @brief Создать представление
     */
    Ui::ProjectInformationView* createView();


    /**
     * @brief Текущая модель представления основного окна
     */
    BusinessLayer::ProjectInformationModel* model = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ProjectInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ProjectInformationView*> allViews;
};

void ProjectInformationManager::Implementation::initView()
{
    if (view == nullptr) {
        view = createView();
    }
}

Ui::ProjectInformationView* ProjectInformationManager::Implementation::createView()
{
    allViews.append(new Ui::ProjectInformationView);
    return allViews.last();
}


// ****


ProjectInformationManager::ProjectInformationManager(QObject* _parent)
    : QObject(_parent),
      d(new Implementation)
{
}

ProjectInformationManager::~ProjectInformationManager() = default;

void ProjectInformationManager::setModel(BusinessLayer::AbstractModel* _model)
{
    d->initView();

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

        connect(d->model, &BusinessLayer::ProjectInformationModel::nameChanged,
                d->view, &Ui::ProjectInformationView::setName);
        connect(d->model, &BusinessLayer::ProjectInformationModel::loglineChanged,
                d->view, &Ui::ProjectInformationView::setLogline);
        connect(d->view, &Ui::ProjectInformationView::nameChanged,
                d->model, &BusinessLayer::ProjectInformationModel::setName);
        connect(d->view, &Ui::ProjectInformationView::loglineChanged,
                d->model, &BusinessLayer::ProjectInformationModel::setLogline);
    }
}

QWidget* ProjectInformationManager::view()
{
    d->initView();
    return d->view;
}

QWidget* ProjectInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer

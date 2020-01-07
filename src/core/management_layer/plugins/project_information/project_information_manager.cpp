#include "project_information_manager.h"

#include "project_information_view.h"


namespace ManagementLayer
{

class ProjectInformationManager::Implementation
{
public:
    /**
     * @brief Создать представление
     */
    Ui::ProjectInformationView* createView();


    /**
     * @brief Предаставление для основного окна
     */
    Ui::ProjectInformationView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ProjectInformationView*> allViews;
};

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

}

QWidget* ProjectInformationManager::view()
{
    if (d->view == nullptr) {
        d->view = d->createView();
    }
    return d->view;
}

QWidget* ProjectInformationManager::createView()
{
    return d->createView();
}

} // namespace ManagementLayer

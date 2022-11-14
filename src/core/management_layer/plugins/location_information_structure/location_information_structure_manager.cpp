#include "location_information_structure_manager.h"

#include "location_information_structure_view.h"

#include <business_layer/model/locations/location_model.h>


namespace ManagementLayer {

class LocationInformationStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::LocationInformationStructureView* createView();


    /**
     * @brief Текущая модель сценария
     */
    QPointer<BusinessLayer::LocationModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::LocationInformationStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::LocationInformationStructureView*> allViews;
};

LocationInformationStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::LocationInformationStructureView* LocationInformationStructureManager::Implementation::
    createView()
{
    allViews.append(new Ui::LocationInformationStructureView);
    return allViews.last();
}


// ****


LocationInformationStructureManager::LocationInformationStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::LocationInformationStructureView::traitsCategoryIndexChanged, this,
            &LocationInformationStructureManager::traitsCategoryIndexChanged);
}

LocationInformationStructureManager::~LocationInformationStructureManager() = default;

QObject* LocationInformationStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* LocationInformationStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* LocationInformationStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* LocationInformationStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* LocationInformationStructureManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* LocationInformationStructureManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void LocationInformationStructureManager::resetModels()
{
    setModel(nullptr);
}

void LocationInformationStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        disconnect(d->model, &BusinessLayer::LocationModel::nameChanged, d->view, nullptr);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::LocationModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        auto updateTitle = [this] { d->view->setTitle(d->model->name()); };
        updateTitle();
        connect(d->model, &BusinessLayer::LocationModel::nameChanged, d->view, updateTitle);
    }
}

} // namespace ManagementLayer

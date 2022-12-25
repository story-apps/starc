#include "world_information_structure_manager.h"

#include "world_information_structure_view.h"

#include <business_layer/model/worlds/world_model.h>


namespace ManagementLayer {

class WorldInformationStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::WorldInformationStructureView* createView();


    /**
     * @brief Текущая модель сценария
     */
    QPointer<BusinessLayer::WorldModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::WorldInformationStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::WorldInformationStructureView*> allViews;
};

WorldInformationStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::WorldInformationStructureView* WorldInformationStructureManager::Implementation::createView()
{
    allViews.append(new Ui::WorldInformationStructureView);
    return allViews.last();
}


// ****


WorldInformationStructureManager::WorldInformationStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::WorldInformationStructureView::traitsCategoryIndexChanged, this,
            &WorldInformationStructureManager::traitsCategoryIndexChanged);
}

WorldInformationStructureManager::~WorldInformationStructureManager() = default;

QObject* WorldInformationStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* WorldInformationStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* WorldInformationStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* WorldInformationStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* WorldInformationStructureManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* WorldInformationStructureManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void WorldInformationStructureManager::resetModels()
{
    setModel(nullptr);
}

void WorldInformationStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        disconnect(d->model, &BusinessLayer::WorldModel::nameChanged, d->view, nullptr);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::WorldModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        auto updateTitle = [this] { d->view->setTitle(d->model->name()); };
        updateTitle();
        connect(d->model, &BusinessLayer::WorldModel::nameChanged, d->view, updateTitle);
    }
}

} // namespace ManagementLayer

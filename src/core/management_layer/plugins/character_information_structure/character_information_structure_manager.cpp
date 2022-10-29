#include "character_information_structure_manager.h"

#include "character_information_structure_view.h"

#include <business_layer/model/characters/character_model.h>


namespace ManagementLayer {

class CharacterInformationStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::CharacterInformationStructureView* createView();


    /**
     * @brief Текущая модель сценария
     */
    QPointer<BusinessLayer::CharacterModel> model;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::CharacterInformationStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::CharacterInformationStructureView*> allViews;
};

CharacterInformationStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::CharacterInformationStructureView* CharacterInformationStructureManager::Implementation::
    createView()
{
    allViews.append(new Ui::CharacterInformationStructureView);
    return allViews.last();
}


// ****


CharacterInformationStructureManager::CharacterInformationStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::CharacterInformationStructureView::traitsCategoryIndexChanged, this,
            &CharacterInformationStructureManager::traitsCategoryIndexChanged);
}

CharacterInformationStructureManager::~CharacterInformationStructureManager() = default;

QObject* CharacterInformationStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* CharacterInformationStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* CharacterInformationStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* CharacterInformationStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* CharacterInformationStructureManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* CharacterInformationStructureManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void CharacterInformationStructureManager::resetModels()
{
    setModel(nullptr);
}

void CharacterInformationStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        disconnect(d->model, &BusinessLayer::CharacterModel::nameChanged, d->view, nullptr);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::CharacterModel*>(_model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        auto updateTitle = [this] { d->view->setTitle(d->model->name()); };
        updateTitle();
        connect(d->model, &BusinessLayer::CharacterModel::nameChanged, d->view, updateTitle);
    }
}

} // namespace ManagementLayer

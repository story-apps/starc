#include "screenplay_breakdown_structure_manager.h"

#include "business_layer/screenplay_breakdown_structure_characters_model.h"
#include "business_layer/screenplay_breakdown_structure_locations_model.h"
#include "business_layer/screenplay_breakdown_structure_scenes_model.h"
#include "ui/screenplay_breakdown_structure_view.h"

#include <business_layer/model/screenplay/screenplay_information_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_folder_item.h>
#include <business_layer/model/screenplay/text/screenplay_text_model_scene_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/color_picker/color_picker.h>
#include <ui/widgets/context_menu/context_menu.h>

#include <QAction>
#include <QWidgetAction>

#include <optional>


namespace ManagementLayer {

class ScreenplayBreakdownStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayBreakdownStructureView* createView();

    /**
     * @brief Текущая модель сценария
     */
    QPointer<BusinessLayer::ScreenplayTextModel> model;

    /**
     * @brief Индекс модели, который необходимо выделить
     * @note Используется в случаях, когда в навигаторе установлена не та модель, что отображается
     *       в редакторе, когда будет установлена нужная модель, в навигаторе будет выделен элемент
     *       с данным индексом
     */
    QModelIndex modelIndexToSelect;

    /**
     * @brief Модели отображения структуры сценария
     */
    BusinessLayer::ScreenplayBreakdownStructureScenesModel* structureModel = nullptr;
    BusinessLayer::ScreenplayBreakdownStructureCharactersModel* charactersModel = nullptr;
    BusinessLayer::ScreenplayBreakdownStructureLocationsModel* locationsModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayBreakdownStructureView* view = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayBreakdownStructureView*> allViews;
};

ScreenplayBreakdownStructureManager::Implementation::Implementation()
{
    view = createView();
}

Ui::ScreenplayBreakdownStructureView* ScreenplayBreakdownStructureManager::Implementation::
    createView()
{
    allViews.append(new Ui::ScreenplayBreakdownStructureView);
    return allViews.last();
}


// ****


ScreenplayBreakdownStructureManager::ScreenplayBreakdownStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayBreakdownStructureView::currentSceneModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto sourceIndex = d->structureModel->mapToSource(_index);
                emit currentModelIndexChanged(sourceIndex);
                d->locationsModel->setSourceModelCurrentIndex(sourceIndex);
            });
    connect(d->view, &Ui::ScreenplayBreakdownStructureView::currentLocationSceneModelIndexChanged,
            this, [this](const QModelIndex& _index) {
                emit currentModelIndexChanged(_index);
                d->locationsModel->setSourceModelCurrentIndex(_index);
            });
}

ScreenplayBreakdownStructureManager::~ScreenplayBreakdownStructureManager() = default;

QObject* ScreenplayBreakdownStructureManager::asQObject()
{
    return this;
}

bool ScreenplayBreakdownStructureManager::isNavigationManager() const
{
    return true;
}

Ui::IDocumentView* ScreenplayBreakdownStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayBreakdownStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* ScreenplayBreakdownStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* ScreenplayBreakdownStructureManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* ScreenplayBreakdownStructureManager::createView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void ScreenplayBreakdownStructureManager::resetModels()
{
    setModel(nullptr);
}

void ScreenplayBreakdownStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);

    d->view->reconfigure();
}

void ScreenplayBreakdownStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(viewCurrentModelIndexChanged(QModelIndex)), this,
            SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
}

void ScreenplayBreakdownStructureManager::setEditingMode(DocumentEditingMode _mode)
{
    d->view->setEditingMode(_mode);
}

void ScreenplayBreakdownStructureManager::setCurrentModelIndex(const QModelIndex& _index)
{
    if (!_index.isValid()) {
        return;
    }

    if (d->model != _index.model()) {
        d->modelIndexToSelect = _index;
        return;
    }

    QSignalBlocker signalBlocker(this);

    //
    // Из редактора карточек мы получаем индексы сцен и папок
    //
    auto indexForSelect = d->structureModel->mapFromSource(_index);
    //
    // Из редактора сценария мы получаем индексы текстовых элементов, они хранятся внутри
    // папок, сцен или битов, которые как раз и отображаются в навигаторе
    //
    if (!indexForSelect.isValid()) {
        indexForSelect = d->structureModel->mapFromSource(_index.parent());
    }
    //
    // ... когда биты скрыты в навигаторе, берём папку или сцену, в которой они находятся
    //
    if (!indexForSelect.isValid()) {
        indexForSelect = d->structureModel->mapFromSource(_index.parent().parent());
    }
    d->view->setCurrentModelIndex(indexForSelect);

    const auto sourceIndexForSelect = d->structureModel->mapToSource(indexForSelect);
    d->locationsModel->setSourceModelCurrentIndex(sourceIndexForSelect);

    d->modelIndexToSelect = {};
}

void ScreenplayBreakdownStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr && d->model->informationModel() != nullptr) {
        d->model->informationModel()->disconnect(d->view);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::ScreenplayTextModel*>(_model);

    //
    // Создаём прокси модели, если ещё не были созданы и настриваем их
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::ScreenplayBreakdownStructureScenesModel(d->view);
    }
    if (d->charactersModel == nullptr) {
        d->charactersModel
            = new BusinessLayer::ScreenplayBreakdownStructureCharactersModel(d->view);
    }
    if (d->locationsModel == nullptr) {
        d->locationsModel = new BusinessLayer::ScreenplayBreakdownStructureLocationsModel(d->view);
    }
    d->view->setModels(d->structureModel, d->charactersModel, d->locationsModel);

    //
    // Помещаем модель с данными в прокси
    //
    d->structureModel->setSourceModel(d->model);
    d->charactersModel->setSourceModel(d->model);
    d->locationsModel->setSourceModel(d->model);

    //
    // Настраиваем соединения с новой моделью
    //
    if (d->model != nullptr) {
        auto updateTitle = [this] {
            d->view->setTitle(
                QString("%1 | %2").arg(tr("Screenplay"), d->model->informationModel()->name()));
        };
        updateTitle();
        connect(d->model->informationModel(),
                &BusinessLayer::ScreenplayInformationModel::nameChanged, d->view, updateTitle);
    }

    //
    // Если элемент к выделению уже задан, выберем его в структуре
    //
    if (d->modelIndexToSelect.isValid()) {
        setCurrentModelIndex(d->modelIndexToSelect);
    }

    //
    // Переконфигурируемся
    //
    reconfigure({});
}

} // namespace ManagementLayer

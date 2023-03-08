#include "screenplay_breakdown_structure_manager.h"

#include "business_layer/screenplay_breakdown_structure_characters_model.h"
#include "business_layer/screenplay_breakdown_structure_locations_model.h"
#include "business_layer/screenplay_breakdown_structure_model_item.h"
#include "business_layer/screenplay_breakdown_structure_model_proxy.h"
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
     * @brief Настроить контекстное меню
     */
    void prepareContextMenuForCharacters(const QModelIndexList& _indexes);
    void prepareContextMenuForLocations(const QModelIndexList& _indexes);


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
    BusinessLayer::ScreenplayBreakdownStructureModelProxy* locationsProxyModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayBreakdownStructureView* view = nullptr;

    /**
     * @brief Контекстное меню для элементов навигатора
     */
    ContextMenu* contextMenu = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayBreakdownStructureView*> allViews;
};

ScreenplayBreakdownStructureManager::Implementation::Implementation()
{
    view = createView();
    contextMenu = new ContextMenu(view);
}

Ui::ScreenplayBreakdownStructureView* ScreenplayBreakdownStructureManager::Implementation::
    createView()
{
    allViews.append(new Ui::ScreenplayBreakdownStructureView);
    return allViews.last();
}

void ScreenplayBreakdownStructureManager::Implementation::prepareContextMenuForCharacters(
    const QModelIndexList& _indexes)
{
    Q_UNUSED(_indexes)

    //
    // Настроим внешний вид меню
    //
    contextMenu->setBackgroundColor(Ui::DesignSystem::color().background());
    contextMenu->setTextColor(Ui::DesignSystem::color().onBackground());

    //
    // Настроим действия меню
    //
    QVector<QAction*> actions;
    //
    auto expandAllAction = new QAction(tr("Expand all"));
    expandAllAction->setIconText(u8"\U000F004C");
    connect(expandAllAction, &QAction::triggered, view, [this] { view->expandCharacters(); });
    actions.append(expandAllAction);
    //
    auto collapseAllAction = new QAction(tr("Collapse all"));
    collapseAllAction->setIconText(u8"\U000F0044");
    connect(collapseAllAction, &QAction::triggered, view,
            [this] { view->collapseAllCharacters(); });
    actions.append(collapseAllAction);
    //
    auto sortAlphabeticallyAction = new QAction(tr("Sort alphabetically"));
    sortAlphabeticallyAction->setSeparator(true);
    sortAlphabeticallyAction->setIconText(u8"\U000F033C");
    connect(sortAlphabeticallyAction, &QAction::triggered, view, [this] {
        charactersModel->sortBy(BusinessLayer::ScreenplayBreakdownSortOrder::Alphabetically);
    });
    actions.append(sortAlphabeticallyAction);
    //
    auto sortByScriptOrderAction = new QAction(tr("Sort by script order"));
    sortByScriptOrderAction->setIconText(u8"\U000f68c0");
    connect(sortByScriptOrderAction, &QAction::triggered, view, [this] {
        charactersModel->sortBy(BusinessLayer::ScreenplayBreakdownSortOrder::ByScriptOrder);
    });
    actions.append(sortByScriptOrderAction);
    //
    auto sortByDurationAction = new QAction(tr("Sort by duration"));
    sortByDurationAction->setIconText(u8"\U000f68c0");
    connect(sortByDurationAction, &QAction::triggered, view, [this] {
        charactersModel->sortBy(BusinessLayer::ScreenplayBreakdownSortOrder::ByDuration);
    });
    actions.append(sortByDurationAction);

    contextMenu->setActions(actions);
}

void ScreenplayBreakdownStructureManager::Implementation::prepareContextMenuForLocations(
    const QModelIndexList& _indexes)
{
    Q_UNUSED(_indexes)

    //
    // Настроим внешний вид меню
    //
    contextMenu->setBackgroundColor(Ui::DesignSystem::color().background());
    contextMenu->setTextColor(Ui::DesignSystem::color().onBackground());

    //
    // Настроим действия меню
    //
    QVector<QAction*> actions;
    //
    auto expandLevel1Action = new QAction(tr("Expand level 1"));
    expandLevel1Action->setIconText(u8"\U000F0616");
    connect(expandLevel1Action, &QAction::triggered, view, [this] { view->expandLocations(0); });
    actions.append(expandLevel1Action);
    //
    auto expandLevel2Action = new QAction(tr("Expand level 2"));
    expandLevel2Action->setIconText(u8"\U000f68c0");
    connect(expandLevel2Action, &QAction::triggered, view, [this] { view->expandLocations(1); });
    actions.append(expandLevel2Action);
    //
    auto expandAllAction = new QAction(tr("Expand all"));
    expandAllAction->setIconText(u8"\U000F004C");
    connect(expandAllAction, &QAction::triggered, view, [this] { view->expandLocations(); });
    actions.append(expandAllAction);
    //
    auto collapseAllAction = new QAction(tr("Collapse all"));
    collapseAllAction->setIconText(u8"\U000F0044");
    connect(collapseAllAction, &QAction::triggered, view, [this] { view->collapseAllLocations(); });
    actions.append(collapseAllAction);
    //
    auto sortAlphabeticallyAction = new QAction(tr("Sort alphabetically"));
    sortAlphabeticallyAction->setSeparator(true);
    sortAlphabeticallyAction->setIconText(u8"\U000F033C");
    connect(sortAlphabeticallyAction, &QAction::triggered, view, [this] {
        locationsProxyModel->sortBy(
            BusinessLayer::ScreenplayBreakdownStructureModelProxy::SortOrder::Alphabetically);
    });
    actions.append(sortAlphabeticallyAction);
    //
    auto sortByDurationAction = new QAction(tr("Sort by duration"));
    sortByDurationAction->setIconText(u8"\U000F04BF");
    connect(sortByDurationAction, &QAction::triggered, view, [this] {
        locationsProxyModel->sortBy(
            BusinessLayer::ScreenplayBreakdownStructureModelProxy::SortOrder::ByDuration);
    });
    actions.append(sortByDurationAction);

    contextMenu->setActions(actions);
}


// ****


ScreenplayBreakdownStructureManager::ScreenplayBreakdownStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(this, &ScreenplayBreakdownStructureManager::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                d->charactersModel->setSourceModelCurrentIndex(_index);
                d->locationsModel->setSourceModelCurrentIndex(_index);
            });
    connect(d->view, &Ui::ScreenplayBreakdownStructureView::currentSceneModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                const auto sourceIndex = d->structureModel->mapToSource(_index);
                emit currentModelIndexChanged(sourceIndex);
            });
    connect(d->view, &Ui::ScreenplayBreakdownStructureView::currentCharacterSceneModelIndexChanged,
            this, &ScreenplayBreakdownStructureManager::currentModelIndexChanged);
    connect(d->view, &Ui::ScreenplayBreakdownStructureView::charactersViewContextMenuRequested,
            this, [this](const QPoint& _pos) {
                if (d->view->selectedIndexes().isEmpty()) {
                    return;
                }

                d->prepareContextMenuForCharacters(d->view->selectedIndexes());
                d->contextMenu->showContextMenu(d->view->mapToGlobal(_pos));
            });
    connect(d->view, &Ui::ScreenplayBreakdownStructureView::currentLocationSceneModelIndexChanged,
            this, &ScreenplayBreakdownStructureManager::currentModelIndexChanged);
    connect(d->view, &Ui::ScreenplayBreakdownStructureView::locationsViewContextMenuRequested, this,
            [this](const QPoint& _pos) {
                if (d->view->selectedIndexes().isEmpty()) {
                    return;
                }

                d->prepareContextMenuForLocations(d->view->selectedIndexes());
                d->contextMenu->showContextMenu(d->view->mapToGlobal(_pos));
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
    d->charactersModel->setSourceModelCurrentIndex(sourceIndexForSelect);
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
        d->locationsProxyModel = new BusinessLayer::ScreenplayBreakdownStructureModelProxy(d->view);
        d->locationsProxyModel->setSourceModel(d->locationsModel);
    }
    d->view->setModels(d->structureModel, d->charactersModel, d->locationsProxyModel);

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

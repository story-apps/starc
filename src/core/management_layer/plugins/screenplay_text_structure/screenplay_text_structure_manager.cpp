#include "screenplay_text_structure_manager.h"

#include "business_layer/screenplay_text_structure_model.h"
#include "ui/screenplay_text_structure_view.h"

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

class ScreenplayTextStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ScreenplayTextStructureView* createView();

    /**
     * @brief Работа с параметрами отображения представления
     */
    void loadViewSettings();
    void saveViewSettings();

    /**
     * @brief Настроить контекстное меню
     */
    void updateContextMenu(const QModelIndexList& _indexes);


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
     * @brief Модель отображения структуры сценария
     */
    BusinessLayer::ScreenplayTextStructureModel* structureModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ScreenplayTextStructureView* view = nullptr;

    /**
     * @brief Контекстное меню для элементов навигатора
     */
    ContextMenu* contextMenu = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ScreenplayTextStructureView*> allViews;
};

ScreenplayTextStructureManager::Implementation::Implementation()
{
    view = createView();
    contextMenu = new ContextMenu(view);

    loadViewSettings();
}

Ui::ScreenplayTextStructureView* ScreenplayTextStructureManager::Implementation::createView()
{
    allViews.append(new Ui::ScreenplayTextStructureView);
    return allViews.last();
}

void ScreenplayTextStructureManager::Implementation::loadViewSettings()
{
    view->loadViewSettings();
}

void ScreenplayTextStructureManager::Implementation::saveViewSettings()
{
    view->saveViewSettings();
}

void ScreenplayTextStructureManager::Implementation::updateContextMenu(
    const QModelIndexList& _indexes)
{
    if (_indexes.isEmpty()) {
        return;
    }

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
    // ... для одного элемента
    //
    if (_indexes.size() == 1) {
        const auto itemIndex = structureModel->mapToSource(_indexes.constFirst());
        std::optional<QColor> itemColor;
        const auto item = model->itemForIndex(itemIndex);
        if (item->type() == BusinessLayer::TextModelItemType::Folder) {
            const auto folderItem
                = static_cast<BusinessLayer::ScreenplayTextModelFolderItem*>(item);
            itemColor = folderItem->color();
        } else if (item->type() == BusinessLayer::TextModelItemType::Group) {
            const auto sceneItem = static_cast<BusinessLayer::ScreenplayTextModelSceneItem*>(item);
            itemColor = sceneItem->color();
        }
        if (itemColor.has_value()) {
            auto colorMenu = new QAction;
            colorMenu->setText(tr("Color"));
            actions.append(colorMenu);

            auto colorAction = new QWidgetAction(colorMenu);
            auto colorPicker = new ColorPicker;
            colorAction->setDefaultWidget(colorPicker);
            colorPicker->setColorCanBeDeselected(true);
            colorPicker->setSelectedColor(itemColor.value());
            colorPicker->setBackgroundColor(Ui::DesignSystem::color().background());
            colorPicker->setTextColor(Ui::DesignSystem::color().onBackground());
            connect(colorPicker, &ColorPicker::selectedColorChanged, view,
                    [this, itemColor, item](const QColor& _color) {
                        if (item->type() == BusinessLayer::TextModelItemType::Folder) {
                            const auto folderItem
                                = static_cast<BusinessLayer::ScreenplayTextModelFolderItem*>(item);
                            folderItem->setColor(_color);
                        } else if (item->type() == BusinessLayer::TextModelItemType::Group) {
                            const auto sceneItem
                                = static_cast<BusinessLayer::ScreenplayTextModelSceneItem*>(item);
                            sceneItem->setColor(_color);
                        }

                        model->updateItem(item);
                        contextMenu->hideContextMenu();
                    });
        }
    }
    //
    // ... для нескольких
    //
    else {
        //
        // Цвета показываем только для папок и сцен
        //
    }

    contextMenu->setActions(actions);
}


// ****


ScreenplayTextStructureManager::ScreenplayTextStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ScreenplayTextStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit currentModelIndexChanged(d->structureModel->mapToSource(_index));
            });
    connect(d->view, &Ui::ScreenplayTextStructureView::customContextMenuRequested, this,
            [this](const QPoint& _pos) {
                if (d->view->selectedIndexes().isEmpty()) {
                    return;
                }

                d->updateContextMenu(d->view->selectedIndexes());
                d->contextMenu->showContextMenu(d->view->mapToGlobal(_pos));
            });
    connect(d->view, &Ui::ScreenplayTextStructureView::pasteBeatNamePressed, this,
            &ScreenplayTextStructureManager::pasteBeatNameToEditorRequested);
}

ScreenplayTextStructureManager::~ScreenplayTextStructureManager() = default;

QObject* ScreenplayTextStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* ScreenplayTextStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* ScreenplayTextStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* ScreenplayTextStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* ScreenplayTextStructureManager::secondaryView(
    BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* ScreenplayTextStructureManager::createView(BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void ScreenplayTextStructureManager::resetModels()
{
    setModel(nullptr);
}

void ScreenplayTextStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);

    const bool showBeats
        = settingsValue(DataStorageLayer::kComponentsScreenplayNavigatorShowBeatsKey).toBool();
    d->structureModel->showBeats(showBeats);

    d->view->reconfigure();
}

void ScreenplayTextStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
            SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
}

void ScreenplayTextStructureManager::saveSettings()
{
    d->saveViewSettings();
}

void ScreenplayTextStructureManager::setEditingMode(DocumentEditingMode _mode)
{
    d->view->setEditingMode(_mode);
}

void ScreenplayTextStructureManager::setCurrentModelIndex(const QModelIndex& _index)
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
    // Из редактора сценария мы получаем индексы текстовых элементов, они хранятся внутри
    // папок, сцен или битов, которые как раз и отображаются в навигаторе
    //
    auto indexForSelect = d->structureModel->mapFromSource(_index.parent());
    //
    // ... когда биты скрыты в навигаторе, берём папку или сцену, в которой они находятся
    //
    if (!indexForSelect.isValid()) {
        indexForSelect = d->structureModel->mapFromSource(_index.parent().parent());
    }
    d->view->setCurrentModelIndex(_index.parent(), indexForSelect);
    d->modelIndexToSelect = {};
}

void ScreenplayTextStructureManager::setModel(BusinessLayer::AbstractModel* _model)
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
    // Создаём прокси модель, если ещё не была создана и настриваем её
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::ScreenplayTextStructureModel(d->view);
        d->view->setModel(d->structureModel);
    }

    //
    // Помещаем модель с данными в прокси
    //
    d->structureModel->setSourceModel(d->model);

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

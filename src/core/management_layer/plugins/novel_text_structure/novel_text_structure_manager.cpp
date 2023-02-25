#include "novel_text_structure_manager.h"

#include "business_layer/novel_text_structure_model.h"
#include "ui/novel_text_structure_view.h"

#include <business_layer/model/novel/novel_information_model.h>
#include <business_layer/model/novel/text/novel_text_model.h>
#include <business_layer/model/novel/text/novel_text_model_folder_item.h>
#include <business_layer/model/novel/text/novel_text_model_scene_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/color_picker/color_picker.h>
#include <ui/widgets/context_menu/context_menu.h>

#include <QAction>
#include <QWidgetAction>

#include <optional>


namespace ManagementLayer {

class NovelTextStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::NovelTextStructureView* createView();

    /**
     * @brief Настроить контекстное меню
     */
    void updateContextMenu(const QModelIndexList& _indexes);


    /**
     * @brief Текущая модель сценария
     */
    QPointer<BusinessLayer::NovelTextModel> model;

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
    BusinessLayer::NovelTextStructureModel* structureModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::NovelTextStructureView* view = nullptr;

    /**
     * @brief Контекстное меню для элементов навигатора
     */
    ContextMenu* contextMenu = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::NovelTextStructureView*> allViews;
};

NovelTextStructureManager::Implementation::Implementation()
{
    view = createView();
    contextMenu = new ContextMenu(view);
}

Ui::NovelTextStructureView* NovelTextStructureManager::Implementation::createView()
{
    allViews.append(new Ui::NovelTextStructureView);
    return allViews.last();
}

void NovelTextStructureManager::Implementation::updateContextMenu(const QModelIndexList& _indexes)
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
            const auto folderItem = static_cast<BusinessLayer::NovelTextModelFolderItem*>(item);
            itemColor = folderItem->color();
        } else if (item->type() == BusinessLayer::TextModelItemType::Group) {
            const auto sceneItem = static_cast<BusinessLayer::NovelTextModelSceneItem*>(item);
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
                                = static_cast<BusinessLayer::NovelTextModelFolderItem*>(item);
                            folderItem->setColor(_color);
                        } else if (item->type() == BusinessLayer::TextModelItemType::Group) {
                            const auto sceneItem
                                = static_cast<BusinessLayer::NovelTextModelSceneItem*>(item);
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
        bool isSameColor = true;
        std::optional<QColor> itemColor;
        QVector<BusinessLayer::TextModelItem*> items;
        for (const auto& index : _indexes) {
            const auto itemIndex = structureModel->mapToSource(index);
            const auto item = model->itemForIndex(itemIndex);
            QColor color;
            if (item->type() == BusinessLayer::TextModelItemType::Folder) {
                const auto folderItem = static_cast<BusinessLayer::TextModelFolderItem*>(item);
                color = folderItem->color();
            } else if (item->type() == BusinessLayer::TextModelItemType::Group) {
                const auto sceneItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
                color = sceneItem->color();
            }

            if (!itemColor.has_value()) {
                itemColor = color;
            } else if (isSameColor && itemColor.value() != color) {
                isSameColor = false;
            }

            items.append(item);
        }

        //
        // ... цвет
        //

        auto colorMenu = new QAction;
        colorMenu->setText(tr("Color"));
        actions.append(colorMenu);

        auto colorAction = new QWidgetAction(colorMenu);
        auto colorPicker = new ColorPicker;
        colorAction->setDefaultWidget(colorPicker);
        colorPicker->setColorCanBeDeselected(true);
        colorPicker->setSelectedColor(isSameColor ? itemColor.value() : QColor());
        colorPicker->setBackgroundColor(Ui::DesignSystem::color().background());
        colorPicker->setTextColor(Ui::DesignSystem::color().onBackground());
        connect(colorPicker, &ColorPicker::selectedColorChanged, view,
                [this, itemColor, items](const QColor& _color) {
                    for (auto item : items) {
                        if (item->type() == BusinessLayer::TextModelItemType::Folder) {
                            const auto folderItem
                                = static_cast<BusinessLayer::TextModelFolderItem*>(item);
                            folderItem->setColor(_color);
                        } else if (item->type() == BusinessLayer::TextModelItemType::Group) {
                            const auto sceneItem
                                = static_cast<BusinessLayer::TextModelGroupItem*>(item);
                            sceneItem->setColor(_color);
                        }

                        model->updateItem(item);
                    }
                    contextMenu->hideContextMenu();
                });
    }

    contextMenu->setActions(actions);
}


// ****


NovelTextStructureManager::NovelTextStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::NovelTextStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit currentModelIndexChanged(d->structureModel->mapToSource(_index));
            });
    connect(d->view, &Ui::NovelTextStructureView::customContextMenuRequested, this,
            [this](const QPoint& _pos) {
                if (d->view->selectedIndexes().isEmpty()) {
                    return;
                }

                d->updateContextMenu(d->view->selectedIndexes());
                d->contextMenu->showContextMenu(d->view->mapToGlobal(_pos));
            });
}

NovelTextStructureManager::~NovelTextStructureManager() = default;

QObject* NovelTextStructureManager::asQObject()
{
    return this;
}

bool NovelTextStructureManager::isNavigationManager() const
{
    return true;
}

Ui::IDocumentView* NovelTextStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* NovelTextStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* NovelTextStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* NovelTextStructureManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* NovelTextStructureManager::createView(BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void NovelTextStructureManager::resetModels()
{
    setModel(nullptr);
}

void NovelTextStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);

    const bool showBeats
        = settingsValue(DataStorageLayer::kComponentsNovelNavigatorShowBeatsKey).toBool()
        && settingsValue(DataStorageLayer::kComponentsNovelNavigatorShowBeatsInTextKey).toBool();
    d->structureModel->showBeats(showBeats);

    d->view->reconfigure();
}

void NovelTextStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(viewCurrentModelIndexChanged(QModelIndex)), this,
            SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
}

void NovelTextStructureManager::setEditingMode(DocumentEditingMode _mode)
{
    d->view->setEditingMode(_mode);
}

void NovelTextStructureManager::setCurrentModelIndex(const QModelIndex& _index)
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
    d->modelIndexToSelect = {};
}

void NovelTextStructureManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::NovelTextModel*>(_model);

    //
    // Создаём прокси модель, если ещё не была создана и настриваем её
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::NovelTextStructureModel(d->view);
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
                QString("%1 | %2").arg(tr("Novel"), d->model->informationModel()->name()));
        };
        updateTitle();
        connect(d->model->informationModel(), &BusinessLayer::NovelInformationModel::nameChanged,
                d->view, updateTitle);
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

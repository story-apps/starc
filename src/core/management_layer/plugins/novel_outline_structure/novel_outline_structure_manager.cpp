#include "novel_outline_structure_manager.h"

#include "business_layer/novel_outline_structure_model.h"
#include "ui/novel_outline_structure_view.h"

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
#include <QTimer>
#include <QWidgetAction>

#include <optional>


namespace ManagementLayer {

class NovelOutlineStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::NovelOutlineStructureView* createView();

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
    BusinessLayer::NovelOutlineStructureModel* structureModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::NovelOutlineStructureView* view = nullptr;

    /**
     * @brief Контекстное меню для элементов навигатора
     */
    ContextMenu* contextMenu = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::NovelOutlineStructureView*> allViews;
};

NovelOutlineStructureManager::Implementation::Implementation()
{
    view = createView();
    contextMenu = new ContextMenu(view);
}

Ui::NovelOutlineStructureView* NovelOutlineStructureManager::Implementation::createView()
{
    allViews.append(new Ui::NovelOutlineStructureView);
    return allViews.last();
}

void NovelOutlineStructureManager::Implementation::updateContextMenu(
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

    //
    // ... для любого количества
    //
    auto expandAll = new QAction(tr("Expand all"));
    expandAll->setSeparator(!actions.isEmpty());
    expandAll->setIconText(u8"\U000F004C");
    connect(expandAll, &QAction::triggered, view, &Ui::NovelOutlineStructureView::expandAll);
    actions.append(expandAll);

    auto collapseAll = new QAction(tr("Collapse all"));
    collapseAll->setIconText(u8"\U000F0044");
    connect(collapseAll, &QAction::triggered, view, &Ui::NovelOutlineStructureView::collapseAll);
    actions.append(collapseAll);

    contextMenu->setActions(actions);
}


// ****


NovelOutlineStructureManager::NovelOutlineStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::NovelOutlineStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit currentModelIndexChanged(d->structureModel->mapToSource(_index));
            });
    connect(d->view, &Ui::NovelOutlineStructureView::customContextMenuRequested, this,
            [this](const QPoint& _pos) {
                if (d->view->selectedIndexes().isEmpty()) {
                    return;
                }

                d->updateContextMenu(d->view->selectedIndexes());
                d->contextMenu->showContextMenu(d->view->mapToGlobal(_pos));
            });
}

NovelOutlineStructureManager::~NovelOutlineStructureManager() = default;

QObject* NovelOutlineStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* NovelOutlineStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* NovelOutlineStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* NovelOutlineStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* NovelOutlineStructureManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* NovelOutlineStructureManager::createView(BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void NovelOutlineStructureManager::resetModels()
{
    setModel(nullptr);
}

void NovelOutlineStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);

    const bool showBeats
        = settingsValue(DataStorageLayer::kComponentsNovelNavigatorShowBeatsKey).toBool()
        && settingsValue(DataStorageLayer::kComponentsNovelNavigatorShowBeatsInOutlineKey).toBool();
    d->structureModel->showBeats(showBeats);

    d->view->reconfigure();
}

void NovelOutlineStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
            SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
}

void NovelOutlineStructureManager::setEditingMode(DocumentEditingMode _mode)
{
    d->view->setEditingMode(_mode);
}

void NovelOutlineStructureManager::setCurrentModelIndex(const QModelIndex& _index)
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
    // ... когда быти скрыты в навигаторе, берём папку или сцену, в которой они находятся
    //
    if (!indexForSelect.isValid()) {
        indexForSelect = d->structureModel->mapFromSource(_index.parent().parent());
    }
    d->view->setCurrentModelIndex(indexForSelect);
    d->modelIndexToSelect = {};
}

void NovelOutlineStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        disconnect(d->model->informationModel(), &BusinessLayer::NovelInformationModel::nameChanged,
                   d->view, nullptr);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::NovelTextModel*>(_model);

    //
    // Создаём прокси модель, если ещё не была создана и настриваем её
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::NovelOutlineStructureModel(d->view);
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
                QString("%1 | %2").arg(tr("Outline"), d->model->informationModel()->name()));
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

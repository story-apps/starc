#include "simple_text_structure_manager.h"

#include "business_layer/simple_text_structure_model.h"
#include "ui/simple_text_structure_view.h"

#include <business_layer/model/simple_text/simple_text_model.h>
#include <business_layer/model/text/text_model_group_item.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/color_picker/color_picker.h>
#include <ui/widgets/context_menu/context_menu.h>

#include <QAction>
#include <QWidgetAction>

#include <optional>


namespace ManagementLayer {

class SimpleTextStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::SimpleTextStructureView* createView();

    /**
     * @brief Настроить контекстное меню
     */
    void updateContextMenu(const QModelIndexList& _indexes);


    /**
     * @brief Текущая модель документа
     */
    QPointer<BusinessLayer::SimpleTextModel> model;

    /**
     * @brief Индекс модели, который необходимо выделить
     * @note Используется в случаях, когда в навигаторе установлена не та модель, что отображается
     *       в редакторе, когда будет установлена нужная модель, в навигаторе будет выделен элемент
     *       с данным индексом
     */
    QModelIndex modelIndexToSelect;

    /**
     * @brief Модель отображения структуры документа
     */
    BusinessLayer::SimpleTextStructureModel* structureModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::SimpleTextStructureView* view = nullptr;

    /**
     * @brief Контекстное меню для элементов навигатора
     */
    ContextMenu* contextMenu = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::SimpleTextStructureView*> allViews;
};

SimpleTextStructureManager::Implementation::Implementation()
{
    view = createView();
    contextMenu = new ContextMenu(view);
}

Ui::SimpleTextStructureView* SimpleTextStructureManager::Implementation::createView()
{
    allViews.append(new Ui::SimpleTextStructureView);
    return allViews.last();
}

void SimpleTextStructureManager::Implementation::updateContextMenu(const QModelIndexList& _indexes)
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
        if (item->type() == BusinessLayer::TextModelItemType::Group) {
            const auto sceneItem = static_cast<BusinessLayer::TextModelGroupItem*>(item);
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
                        if (item->type() == BusinessLayer::TextModelItemType::Group) {
                            const auto sceneItem
                                = static_cast<BusinessLayer::TextModelGroupItem*>(item);
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


SimpleTextStructureManager::SimpleTextStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::SimpleTextStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit currentModelIndexChanged(d->structureModel->mapToSource(_index));
            });
    connect(d->view, &Ui::SimpleTextStructureView::customContextMenuRequested, this,
            [this](const QPoint& _pos) {
                if (d->view->selectedIndexes().isEmpty()) {
                    return;
                }

                d->updateContextMenu(d->view->selectedIndexes());
                d->contextMenu->showContextMenu(d->view->mapToGlobal(_pos));
            });
}

SimpleTextStructureManager::~SimpleTextStructureManager() = default;

QObject* SimpleTextStructureManager::asQObject()
{
    return this;
}

Ui::IDocumentView* SimpleTextStructureManager::view()
{
    return d->view;
}

Ui::IDocumentView* SimpleTextStructureManager::view(BusinessLayer::AbstractModel* _model)
{
    setModel(_model);
    return d->view;
}

Ui::IDocumentView* SimpleTextStructureManager::secondaryView()
{
    return nullptr;
}

Ui::IDocumentView* SimpleTextStructureManager::secondaryView(BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

Ui::IDocumentView* SimpleTextStructureManager::createView(BusinessLayer::AbstractModel* _model)
{
    Q_UNUSED(_model);
    return nullptr;
}

void SimpleTextStructureManager::resetModels()
{
    setModel(nullptr);
}

void SimpleTextStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);
    d->view->reconfigure();
}

void SimpleTextStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(QModelIndex)), this,
            SLOT(setCurrentModelIndex(QModelIndex)), Qt::UniqueConnection);
}

void SimpleTextStructureManager::setEditingMode(DocumentEditingMode _mode)
{
    d->view->setEditingMode(_mode);
}

void SimpleTextStructureManager::setCurrentModelIndex(const QModelIndex& _index)
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
    // Из редактора текста мы получаем индексы текстовых элементов, они хранятся внутри
    // глав, которые как раз и отображаются в навигаторе
    //
    d->view->setCurrentModelIndex(d->structureModel->mapFromSource(_index.parent()));
    d->modelIndexToSelect = {};
}

void SimpleTextStructureManager::setModel(BusinessLayer::AbstractModel* _model)
{
    //
    // Разрываем соединения со старой моделью
    //
    if (d->model != nullptr) {
        d->view->disconnect(d->model);
    }

    //
    // Определяем новую модель
    //
    d->model = qobject_cast<BusinessLayer::SimpleTextModel*>(_model);

    //
    // Создаём прокси модель, если ещё не была создана и настриваем её
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::SimpleTextStructureModel(d->view);
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
        d->view->setTitle(d->model->name());
        connect(d->model, &BusinessLayer::SimpleTextModel::nameChanged, d->view,
                &Ui::SimpleTextStructureView::setTitle);
    }

    //
    // Если элемент к выделению уже задан, выберем его в структуре
    //
    if (d->modelIndexToSelect.isValid()) {
        setCurrentModelIndex(d->modelIndexToSelect);
    }
}

} // namespace ManagementLayer

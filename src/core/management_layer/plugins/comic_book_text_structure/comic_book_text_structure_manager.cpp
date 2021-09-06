#include "comic_book_text_structure_manager.h"

#include "business_layer/comic_book_text_structure_model.h"
#include "ui/comic_book_text_structure_view.h"

#include <business_layer/model/comic_book/comic_book_information_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_folder_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_page_item.h>
#include <business_layer/model/comic_book/text/comic_book_text_model_panel_item.h>
#include <ui/design_system/design_system.h>
#include <ui/widgets/color_picker/color_picker.h>
#include <ui/widgets/context_menu/context_menu.h>

#include <QAction>
#include <QTimer>
#include <QWidgetAction>

#include <optional>


namespace ManagementLayer {

class ComicBookTextStructureManager::Implementation
{
public:
    explicit Implementation();

    /**
     * @brief Создать представление
     */
    Ui::ComicBookTextStructureView* createView();

    /**
     * @brief Настроить контекстное меню
     */
    void updateContextMenu(const QModelIndexList& _indexes);


    /**
     * @brief Текущая модель комикса
     */
    QPointer<BusinessLayer::ComicBookTextModel> model;

    /**
     * @brief Индекс модели, который необходимо выделить
     * @note Используется в случаях, когда в навигаторе установлена не та модель, что отображается
     *       в редакторе, когда будет установлена нужная модель, в навигаторе будет выделен элемент
     *       с данным индексом
     */
    QModelIndex modelIndexToSelect;

    /**
     * @brief Модель отображения структуры комикса
     */
    BusinessLayer::ComicBookTextStructureModel* structureModel = nullptr;

    /**
     * @brief Предаставление для основного окна
     */
    Ui::ComicBookTextStructureView* view = nullptr;

    /**
     * @brief Контекстное меню для элементов навигатора
     */
    ContextMenu* contextMenu = nullptr;

    /**
     * @brief Все созданные представления
     */
    QVector<Ui::ComicBookTextStructureView*> allViews;
};

ComicBookTextStructureManager::Implementation::Implementation()
{
    view = createView();
    contextMenu = new ContextMenu(view);
}

Ui::ComicBookTextStructureView* ComicBookTextStructureManager::Implementation::createView()
{
    allViews.append(new Ui::ComicBookTextStructureView);
    return allViews.last();
}

void ComicBookTextStructureManager::Implementation::updateContextMenu(
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
        if (item->type() == BusinessLayer::ComicBookTextModelItemType::Folder) {
            const auto folderItem = static_cast<BusinessLayer::ComicBookTextModelFolderItem*>(item);
            itemColor = folderItem->color();
        } else if (item->type() == BusinessLayer::ComicBookTextModelItemType::Page) {
            const auto pageItem = static_cast<BusinessLayer::ComicBookTextModelPageItem*>(item);
            itemColor = pageItem->color();
        } else if (item->type() == BusinessLayer::ComicBookTextModelItemType::Panel) {
            const auto panelItem = static_cast<BusinessLayer::ComicBookTextModelPanelItem*>(item);
            itemColor = panelItem->color();
        }
        if (itemColor.has_value()) {
            auto colorMenu = new QAction;
            colorMenu->setText(tr("Color"));
            actions.append(colorMenu);

            auto colorAction = new QWidgetAction(colorMenu);
            auto colorPicker = new ColorPicker;
            colorAction->setDefaultWidget(colorPicker);
            colorPicker->setSelectedColor(itemColor.value());
            connect(
                colorPicker, &ColorPicker::colorSelected, view,
                [this, itemColor, item](const QColor& _color) {
                    auto newColor = _color;
                    if (itemColor.value() == newColor) {
                        newColor = QColor(QColor::Invalid);
                    }

                    if (item->type() == BusinessLayer::ComicBookTextModelItemType::Folder) {
                        const auto folderItem
                            = static_cast<BusinessLayer::ComicBookTextModelFolderItem*>(item);
                        folderItem->setColor(newColor);
                    } else if (item->type() == BusinessLayer::ComicBookTextModelItemType::Page) {
                        const auto pageItem
                            = static_cast<BusinessLayer::ComicBookTextModelPageItem*>(item);
                        pageItem->setColor(newColor);
                    } else if (item->type() == BusinessLayer::ComicBookTextModelItemType::Panel) {
                        const auto panelItem
                            = static_cast<BusinessLayer::ComicBookTextModelPanelItem*>(item);
                        panelItem->setColor(newColor);
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


ComicBookTextStructureManager::ComicBookTextStructureManager(QObject* _parent)
    : QObject(_parent)
    , d(new Implementation)
{
    connect(d->view, &Ui::ComicBookTextStructureView::currentModelIndexChanged, this,
            [this](const QModelIndex& _index) {
                emit currentModelIndexChanged(d->structureModel->mapToSource(_index));
            });
    connect(d->view, &Ui::ComicBookTextStructureView::customContextMenuRequested, this,
            [this](const QPoint& _pos) {
                if (d->view->selectedIndexes().isEmpty()) {
                    return;
                }

                d->updateContextMenu(d->view->selectedIndexes());
                d->contextMenu->showContextMenu(d->view->mapToGlobal(_pos));
            });
}

ComicBookTextStructureManager::~ComicBookTextStructureManager() = default;

QObject* ComicBookTextStructureManager::asQObject()
{
    return this;
}

void ComicBookTextStructureManager::setModel(BusinessLayer::AbstractModel* _model)
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
    d->model = qobject_cast<BusinessLayer::ComicBookTextModel*>(_model);

    //
    // Создаём прокси модель, если ещё не была создана и настриваем её
    //
    if (d->structureModel == nullptr) {
        d->structureModel = new BusinessLayer::ComicBookTextStructureModel(d->view);
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
        d->view->setTitle(d->model->informationModel()->name());
        connect(d->model->informationModel(),
                &BusinessLayer::ComicBookInformationModel::nameChanged, d->view,
                &Ui::ComicBookTextStructureView::setTitle);
    }

    //
    // Если элемент к выделению уже задан, выберем его в структуре
    //
    if (d->modelIndexToSelect.isValid()) {
        setCurrentModelIndex(d->modelIndexToSelect);
    }
}

QWidget* ComicBookTextStructureManager::view()
{
    return d->view;
}

QWidget* ComicBookTextStructureManager::createView()
{
    return d->createView();
}

void ComicBookTextStructureManager::reconfigure(const QStringList& _changedSettingsKeys)
{
    Q_UNUSED(_changedSettingsKeys);
    d->view->reconfigure();
}

void ComicBookTextStructureManager::bind(IDocumentManager* _manager)
{
    Q_ASSERT(_manager);

    connect(_manager->asQObject(), SIGNAL(currentModelIndexChanged(const QModelIndex&)), this,
            SLOT(setCurrentModelIndex(const QModelIndex&)), Qt::UniqueConnection);
}

void ComicBookTextStructureManager::setCurrentModelIndex(const QModelIndex& _index)
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
    // сцен или папок, которые как раз и отображаются в навигаторе
    //
    d->view->setCurrentModelIndex(d->structureModel->mapFromSource(_index.parent()));
    d->modelIndexToSelect = {};
}

} // namespace ManagementLayer

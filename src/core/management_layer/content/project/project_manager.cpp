#include "project_manager.h"

#include "project_models_facade.h"
#include "project_plugins_builder.h"

#include <business_layer/model/characters/character_model.h>
#include <business_layer/model/characters/characters_model.h>
#include <business_layer/model/locations/location_model.h>
#include <business_layer/model/locations/locations_model.h>
#include <business_layer/model/project/project_information_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>

#include <data_layer/storage/document_change_storage.h>
#include <data_layer/storage/document_data_storage.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/document_object.h>

#include <ui/abstract_navigator.h>
#include <ui/design_system/design_system.h>
#include <ui/project/create_document_dialog.h>
#include <ui/project/project_navigator.h>
#include <ui/project/project_tool_bar.h>
#include <ui/project/project_view.h>
#include <ui/widgets/context_menu/context_menu.h>
#include <ui/widgets/dialog/dialog.h>

#include <QDateTime>
#include <QHBoxLayout>
#include <QStandardItemModel>
#include <QUuid>
#include <QVariantAnimation>


namespace ManagementLayer
{

class ProjectManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);

    /**
     * @brief Действия контекстного меню
     */
    enum class ContextMenuAction {
        AddDocument,
        RemoveDocument
    };

    /**
     * @brief Обновить модель действий контекстного меню навигатора
     */
    void updateNavigatorContextMenu(const QModelIndex& _index);

    /**
     * @brief Выполнить заданное действие контекстного меню
     */
    void executeContextMenuAction(const QModelIndex& _itemIndex, const QModelIndex& _contextMenuIndex);

    /**
     * @brief Добавить документ в проект
     */
    void addDocument(const QModelIndex& _itemIndex);

    /**
     * @brief Удалить документ
     */
    void removeDocument(const QModelIndex& _itemIndex);


    QWidget* topLevelWidget = nullptr;

    Ui::ProjectToolBar* toolBar = nullptr;

    Ui::ProjectNavigator* navigator = nullptr;
    QStandardItemModel* navigatorContextMenuModel = nullptr;

    Ui::ProjectView* view = nullptr;

    BusinessLayer::StructureModel* projectStructureModel = nullptr;

    DataStorageLayer::DocumentDataStorage documentDataStorage;

    ProjectModelsFacade modelsFacade;
    ProjectPluginsBuilder pluginsBuilder;
};

ProjectManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent),
      toolBar(new Ui::ProjectToolBar(_parent)),
      navigator(new Ui::ProjectNavigator(_parent)),
      navigatorContextMenuModel(new QStandardItemModel(navigator)),
      view(new Ui::ProjectView(_parent)),
      projectStructureModel(new BusinessLayer::StructureModel(navigator)),
      modelsFacade(&documentDataStorage)
{
    toolBar->hide();
    navigator->hide();
    view->hide();

    navigator->setModel(projectStructureModel);
    navigator->setContextMenuModel(navigatorContextMenuModel);
}

void ProjectManager::Implementation::updateNavigatorContextMenu(const QModelIndex& _index)
{
    navigatorContextMenuModel->clear();

    auto addDocument = new QStandardItem(tr("Add document"));
    addDocument->setData("\uf415", Qt::DecorationRole);
    addDocument->setData(static_cast<int>(ContextMenuAction::AddDocument), Qt::UserRole);
    navigatorContextMenuModel->appendRow(addDocument);

    if (_index.isValid()) {
        auto removeDocument = new QStandardItem(tr("Remove document"));
        removeDocument->setData("\uf1c0", Qt::DecorationRole);
        removeDocument->setData(static_cast<int>(ContextMenuAction::RemoveDocument), Qt::UserRole);
        navigatorContextMenuModel->appendRow(removeDocument);
    }
}

void ProjectManager::Implementation::executeContextMenuAction(const QModelIndex& _itemIndex,
    const QModelIndex& _contextMenuIndex)
{
    if (!_contextMenuIndex.isValid()) {
        return;
    }

    const auto actionData = _contextMenuIndex.data(Qt::UserRole);
    if (actionData.isNull()) {
        return;
    }

    const auto action = static_cast<ContextMenuAction>(actionData.toInt());
    switch (action) {
        case ContextMenuAction::AddDocument: {
            addDocument(_itemIndex);
            break;
        }

        case ContextMenuAction::RemoveDocument: {
            removeDocument(_itemIndex);
            break;
        }
    }
}

void ProjectManager::Implementation::addDocument(const QModelIndex& _itemIndex)
{
    //
    // TODO: вложение в выделенный в дереве элемент
    //

    auto dialog = new Ui::CreateDocumentDialog(topLevelWidget);

    connect(dialog, &Ui::CreateDocumentDialog::createPressed, navigator,
            [this, dialog] (Domain::DocumentObjectType _type, const QString& _name)
    {
        projectStructureModel->addDocument(_type, _name);
        dialog->hideDialog();
    });
    connect(dialog, &Ui::CreateDocumentDialog::disappeared, dialog, &Ui::CreateDocumentDialog::deleteLater);

    dialog->showDialog();
}

void ProjectManager::Implementation::removeDocument(const QModelIndex& _itemIndex)
{
    auto item = projectStructureModel->itemForIndex(_itemIndex);
    if (item == nullptr) {
        return;
    }

    auto itemTopLevelParent = item->parent();
    if (itemTopLevelParent == nullptr) {
        return;
    }
    while (itemTopLevelParent->parent()
           && itemTopLevelParent->parent()->type() != Domain::DocumentObjectType::Undefined) {
        itemTopLevelParent = itemTopLevelParent->parent();
    }

    //
    // Если документ в корзине
    //
    if (itemTopLevelParent->type() == Domain::DocumentObjectType::RecycleBin) {
        //
        // ... то спросим действительно ли пользователь хочет его удалить
        //
        const int kCancelButtonId = 0;
        const int kRemoveButtonId = 1;
        auto dialog = new Dialog(topLevelWidget);
        dialog->showDialog({},
                           tr("Do you really want to permanently remove document?"),
                           {{ kCancelButtonId, tr("No"), Dialog::RejectButton },
                            { kRemoveButtonId, tr("Yes, remove"), Dialog::NormalButton }});
        QObject::connect(dialog, &Dialog::finished,
                         [this, dialog, item] (const Dialog::ButtonInfo& _buttonInfo)
        {
            dialog->hideDialog();

            //
            // Пользователь передумал удалять
            //
            if (_buttonInfo.id == kCancelButtonId) {
                return;
            }

            //
            // Если таки хочет, то удаляем документ
            // NOTE: порядок удаления важен
            //
            auto document = DataStorageLayer::StorageFacade::documentStorage()->document(item->uuid());
            modelsFacade.removeModelFor(document);
            DataStorageLayer::StorageFacade::documentStorage()->removeDocument(document);
            projectStructureModel->removeItem(item);
        });
        QObject::connect(dialog, &Dialog::disappeared, dialog, &Dialog::deleteLater);
    }
    //
    // Если документ ещё не в корзине, то переносим его в корзину
    //
    else {
        projectStructureModel->moveItemToRecycleBin(item);
    }
}


// ****


ProjectManager::ProjectManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->toolBar, &Ui::ProjectToolBar::menuPressed, this, &ProjectManager::menuRequested);
    connect(d->toolBar, &Ui::ProjectToolBar::viewPressed, this, [this] (const QString& _mimeType) {
        showView(d->navigator->currentIndex(), _mimeType);
    });

    //
    // Отображаем необходимый редактор при выборе документа в списке
    //
    connect(d->navigator, &Ui::ProjectNavigator::itemSelected, this,
            [this] (const QModelIndex& _index)
    {
        if (!_index.isValid()) {
            d->view->showDefaultPage();
            return;
        }

        //
        // Определим выделенный элемент и скорректируем интерфейс
        //
        const auto item = d->projectStructureModel->itemForIndex(_index);
        const auto documentMimeType = Domain::mimeTypeFor(item->type());
        //
        // ... настроим иконки представлений
        //
        d->toolBar->clearViews();
        const auto views = d->pluginsBuilder.editorsInfoFor(documentMimeType);
        for (auto view : views) {
            const bool isActive = view.mimeType == views.first().mimeType;
            d->toolBar->addView(view.mimeType, view.icon, isActive);
        }

        //
        // Откроем документ на редактирование в первом из представлений
        //
        if (views.isEmpty()) {
            d->view->showDefaultPage();
            return;
        }
        showView(_index, views.first().mimeType);
    });
    //
    // Отображаем навигатор выбранного элемента
    //
    connect(d->navigator, &Ui::ProjectNavigator::itemDoubleClicked, this,
            [this] (const QModelIndex& _index)
    {
        if (!d->projectStructureModel->data(
                _index, static_cast<int>(BusinessLayer::StructureModelDataRole::IsNavigatorAvailable)).toBool()) {
            return;
        }

        showNavigator(_index);
    });
    connect(d->navigator, &Ui::ProjectNavigator::contextMenuUpdateRequested, this,
            [this] (const QModelIndex& _index) { d->updateNavigatorContextMenu(_index); });
    connect(d->navigator, &Ui::ProjectNavigator::contextMenuItemClicked, this,
            [this] (const QModelIndex& _contextMenuIndex)
    {
        d->executeContextMenuAction(d->navigator->currentIndex(), _contextMenuIndex);
    });

    //
    // Соединения с моделью структуры проекта
    //
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::documentAdded,
            [this] (const QUuid& _uuid, Domain::DocumentObjectType _type, const QString& _name)
    {
        auto document = DataStorageLayer::StorageFacade::documentStorage()->storeDocument(_uuid, _type);
        auto documentModel = d->modelsFacade.modelFor(document);
        documentModel->setDocumentName(_name);

        switch (_type) {
            case Domain::DocumentObjectType::Character: {
                auto charactersDocument = DataStorageLayer::StorageFacade::documentStorage()->document(
                                              Domain::DocumentObjectType::Characters);
                auto charactersModel = static_cast<BusinessLayer::CharactersModel*>(d->modelsFacade.modelFor(charactersDocument));
                auto characterModel = static_cast<BusinessLayer::CharacterModel*>(documentModel);
                charactersModel->addCharacterModel(characterModel);

                break;
            }

            case Domain::DocumentObjectType::Location: {
                auto locationsDocument = DataStorageLayer::StorageFacade::documentStorage()->document(
                                             Domain::DocumentObjectType::Locations);
                auto locationsModel = static_cast<BusinessLayer::LocationsModel*>(d->modelsFacade.modelFor(locationsDocument));
                auto locationModel = static_cast<BusinessLayer::LocationModel*>(documentModel);
                locationsModel->addLocationModel(locationModel);

                break;
            }

            default: break;
        }
    });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::contentsChanged, this,
            [this] (const QByteArray& _undo, const QByteArray& _redo)
    {
        handleModelChange(d->projectStructureModel, _undo, _redo);
    });

    //
    // Соединения представления
    //
    connect(d->view, &Ui::ProjectView::createNewItemPressed, this, [this] {
        d->addDocument({});
    });

    //
    // Соединения со строителем моделей
    //
    connect(&d->modelsFacade, &ProjectModelsFacade::modelNameChanged, this,
            [this] (BusinessLayer::AbstractModel* _model, const QString& _name)
    {
        auto item = d->projectStructureModel->itemForUuid(_model->document()->uuid());
        d->projectStructureModel->setItemName(item, _name);
    });
    connect(&d->modelsFacade, &ProjectModelsFacade::modelContentChanged, this, &ProjectManager::handleModelChange);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectNameChanged, this, &ProjectManager::projectNameChanged);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectLoglineChanged, this, &ProjectManager::projectLoglineChanged);
    connect(&d->modelsFacade, &ProjectModelsFacade::projectCoverChanged, this, &ProjectManager::projectCoverChanged);
    auto addDocumentToContainer = [this] (Domain::DocumentObjectType _containerType, Domain::DocumentObjectType _documentType, const QString& _documentName) {
        for (int itemRow = 0; itemRow < d->projectStructureModel->rowCount(); ++itemRow) {
            const auto itemIndex = d->projectStructureModel->index(itemRow, 0);
            const auto item = d->projectStructureModel->itemForIndex(itemIndex);
            if (item->type() == _containerType) {
                d->projectStructureModel->addDocument(_documentType, _documentName, itemIndex);
                break;
            }
        }
    };
    connect(&d->modelsFacade, &ProjectModelsFacade::createCharacterRequested, this,
            [addDocumentToContainer] (const QString& _name)
    {
        addDocumentToContainer(Domain::DocumentObjectType::Characters,
                               Domain::DocumentObjectType::Character,
                               _name);
    });
    connect(&d->modelsFacade, &ProjectModelsFacade::createLocationRequested, this,
            [addDocumentToContainer] (const QString& _name)
    {
        addDocumentToContainer(Domain::DocumentObjectType::Locations,
                               Domain::DocumentObjectType::Location,
                               _name);
    });
}

ProjectManager::~ProjectManager() = default;

QWidget* ProjectManager::toolBar() const
{
    return d->toolBar;
}

QWidget* ProjectManager::navigator() const
{
    return d->navigator;
}

QWidget* ProjectManager::view() const
{
    return d->view;
}

void ProjectManager::loadCurrentProject(const QString& _name, const QString& _path)
{
    //
    // Загружаем структуру
    //
    d->projectStructureModel->setDocument(
        DataStorageLayer::StorageFacade::documentStorage()->document(Domain::DocumentObjectType::Structure));

    //
    // Загружаем информацию о проекте
    //
    auto projectInformationModel
            = static_cast<BusinessLayer::ProjectInformationModel*>(
                  d->modelsFacade.modelFor(
                      DataStorageLayer::StorageFacade::documentStorage()->document(
                          Domain::DocumentObjectType::Project)));
    if (projectInformationModel->name().isEmpty()) {
        projectInformationModel->setName(_name);
    } else {
        emit projectNameChanged(projectInformationModel->name());
        emit projectLoglineChanged(projectInformationModel->logline());
        emit projectCoverChanged(projectInformationModel->cover());
    }


    //
    // Синхронизировать структуру с облаком
    //

    //
    // Открыть структуру
    //

    //
    // Загрузить состояние дерева
    //
    d->navigator->restoreState(DataStorageLayer::StorageFacade::settingsStorage()->value(
                                   DataStorageLayer::projectStructureKey(_path),
                                   DataStorageLayer::SettingsStorage::SettingsPlace::Application));

    //
    // Восстановить последнее состояние дерева, если возможно
    //

    //
    // Синхронизировать выбранный документ
    //

    //
    // Синхрониировать все остальные изменения
    //
}

void ProjectManager::closeCurrentProject(const QString& _path)
{
    //
    // Сохранить состояние дерева
    //
    DataStorageLayer::StorageFacade::settingsStorage()->setValue(
                DataStorageLayer::projectStructureKey(_path),
                d->navigator->saveState(),
                DataStorageLayer::SettingsStorage::SettingsPlace::Application);

    //
    // Очищаем структуру
    //
    d->projectStructureModel->clear();

    //
    // Очищаем все загруженные модели документов
    //
    d->modelsFacade.clear();

    //
    // Сбрасываем загруженные изображения
    //
    d->documentDataStorage.clear();
}

void ProjectManager::saveChanges()
{
    //
    // Сохраняем структуру
    //
    const auto structure = d->projectStructureModel->document();
    DataStorageLayer::StorageFacade::documentStorage()->updateDocument(structure);

    //
    // Сохраняем остальные документы
    //
    for (auto model : d->modelsFacade.models()) {
        DataStorageLayer::StorageFacade::documentStorage()->updateDocument(model->document());
    }

    //
    // Сохраняем изображения
    //
    d->documentDataStorage.saveChanges();

    //
    // Сохраняем все изменения документов
    //
    DataStorageLayer::StorageFacade::documentChangeStorage()->store();
}

void ProjectManager::handleModelChange(BusinessLayer::AbstractModel* _model,
    const QByteArray& _undo, const QByteArray& _redo)
{
    DataStorageLayer::StorageFacade::documentChangeStorage()->appendDocumentChange(
        _model->document()->uuid(), QUuid::createUuid(), _undo, _redo,
        DataStorageLayer::StorageFacade::settingsStorage()->userName(),
        DataStorageLayer::StorageFacade::settingsStorage()->userEmail());

    emit contentsChanged();
}

void ProjectManager::showView(const QModelIndex& _itemIndex, const QString& _viewMimeType)
{
    const auto item = d->projectStructureModel->itemForIndex(_itemIndex);

    //
    // Определим модель
    //
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(item->uuid());
    auto model = d->modelsFacade.modelFor(document);
    if (model == nullptr) {
        d->view->showDefaultPage();
        return;
    }

    //
    // Определим представление и отобразим
    //
    auto view = d->pluginsBuilder.activateView(_viewMimeType, model);
    if (view == nullptr) {
        d->view->showDefaultPage();
        return;
    }
    d->view->setCurrentWidget(view);

    //
    // Настроим возможность перехода в навигатор
    //
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(_viewMimeType);
    d->projectStructureModel->setNavigatorAvailableFor(_itemIndex, !navigatorMimeType.isEmpty());

    //
    // Если в данный момент отображён кастомный навигатов, откроем навигатор соответствующий редактору
    //
    if (!d->navigator->isProjectNavigatorShown()) {
        showNavigator(_itemIndex, _viewMimeType);
    }
}

void ProjectManager::showNavigator(const QModelIndex& _itemIndex, const QString& _viewMimeType)
{
    if (!_itemIndex.isValid()) {
        d->navigator->showProjectNavigator();
        return;
    }

    const auto item = d->projectStructureModel->itemForIndex(_itemIndex);

    //
    // Определим модель
    //
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(item->uuid());
    auto model = d->modelsFacade.modelFor(document);
    if (model == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Определим представление и отобразим
    //
    const auto viewMimeType = !_viewMimeType.isEmpty()
                              ? _viewMimeType
                              : d->toolBar->currentViewMimeType();
    const auto navigatorMimeType = d->pluginsBuilder.navigatorMimeTypeFor(viewMimeType);
    auto view = d->pluginsBuilder.activateView(navigatorMimeType, model);
    if (view == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    //
    // Связываем редактор с навигатоом
    //
    d->pluginsBuilder.bind(viewMimeType, navigatorMimeType);

    //
    // Задаём заголовок навигатора и настраиваем возможность перехода к навигатору проекта
    //
    auto navigatorView = qobject_cast<Ui::AbstractNavigator*>(view);
    //
    // FIXME: Надо как-то более универсально сделать этот момент, чтобы не приходилось задавать
    //        название родителя для случаев, когда нужно имя самого документа, а не его родителя
    //
    navigatorView->setTitle(item->parent()->name());
    connect(navigatorView, &Ui::AbstractNavigator::backPressed,
            d->navigator, &Ui::ProjectNavigator::showProjectNavigator,
            Qt::UniqueConnection);
    d->navigator->setCurrentWidget(view);
}

} // namespace ManagementLayer

#include "project_manager.h"

#include "project_models_builder.h"
#include "project_plugins_builder.h"

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
    BusinessLayer::ProjectInformationModel* projectInformationModel = nullptr;

    DataStorageLayer::DocumentDataStorage documentDataStorage;

    ProjectModelsBuilder modelBuilder;
    ProjectPluginsBuilder pluginBuilder;
};

ProjectManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent),
      toolBar(new Ui::ProjectToolBar(_parent)),
      navigator(new Ui::ProjectNavigator(_parent)),
      navigatorContextMenuModel(new QStandardItemModel(navigator)),
      view(new Ui::ProjectView(_parent)),
      projectStructureModel(new BusinessLayer::StructureModel(navigator)),
      projectInformationModel(new BusinessLayer::ProjectInformationModel(navigator)),
      modelBuilder(&documentDataStorage)
{
    toolBar->hide();
    navigator->hide();
    view->hide();

    navigator->setModel(projectStructureModel);
    navigator->setContextMenuModel(navigatorContextMenuModel);

    projectInformationModel->setImageWrapper(&documentDataStorage);
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
            modelBuilder.removeModelFor(document);
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
        const auto views = d->pluginBuilder.editorsInfoFor(documentMimeType);
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
        auto documentModel
                = _type == Domain::DocumentObjectType::Project
                  ? d->projectInformationModel
                  : d->modelBuilder.modelFor(document);
        documentModel->setDocumentName(_name);
    });
    connect(d->projectStructureModel, &BusinessLayer::StructureModel::contentsChanged, this,
            [this] (const QByteArray& _undo, const QByteArray& _redo)
    {
        DataStorageLayer::StorageFacade::documentChangeStorage()->appendDocumentChange(
            d->projectStructureModel->document()->uuid(), QUuid::createUuid(), _undo, _redo,
            DataStorageLayer::StorageFacade::settingsStorage()->userName(),
            DataStorageLayer::StorageFacade::settingsStorage()->userEmail());

        emit contentsChanged();
    });

    //
    // Соединения с моделью данных о проекте
    //
    connect(d->projectInformationModel, &BusinessLayer::ProjectInformationModel::nameChanged,
            this, &ProjectManager::projectNameChanged, Qt::UniqueConnection);
    connect(d->projectInformationModel, &BusinessLayer::ProjectInformationModel::loglineChanged,
            this, &ProjectManager::projectLoglineChanged, Qt::UniqueConnection);
    connect(d->projectInformationModel, &BusinessLayer::ProjectInformationModel::coverChanged,
            this, &ProjectManager::projectCoverChanged, Qt::UniqueConnection);

    //
    // Соединения представления
    //
    connect(d->view, &Ui::ProjectView::createNewItemPressed, this, [this] {
        d->addDocument({});
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
    d->projectStructureModel->setDocument(DataStorageLayer::StorageFacade::documentStorage()->structure());

    //
    // Загружаем информацию о проекте
    //
    d->projectInformationModel->setDocument(
        DataStorageLayer::StorageFacade::documentStorage()->document(Domain::DocumentObjectType::Project));
    if (d->projectInformationModel->name().isEmpty()) {
        d->projectInformationModel->setName(_name);
    } else {
        emit projectNameChanged(d->projectInformationModel->name());
        emit projectLoglineChanged(d->projectInformationModel->logline());
        emit projectCoverChanged(d->projectInformationModel->cover());
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
    // Очищаем данные о проекте
    //
    d->projectInformationModel->clear();

    //
    // Очищаем все загруженные модели документов
    //
    d->modelBuilder.clear();

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
    // Сохраняем проект
    //
    const auto project = d->projectInformationModel->document();
    DataStorageLayer::StorageFacade::documentStorage()->updateDocument(project);

    //
    // Сохраняем остальные документы
    //
    for (auto model : d->modelBuilder.models()) {
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

void ProjectManager::showView(const QModelIndex& _itemIndex, const QString& _viewMimeType)
{
    const auto item = d->projectStructureModel->itemForIndex(_itemIndex);

    //
    // Определим модель
    //
    auto document = DataStorageLayer::StorageFacade::documentStorage()->document(item->uuid());
    auto model =
            document->type() == Domain::DocumentObjectType::Project
            ? d->projectInformationModel
            : d->modelBuilder.modelFor(document);
    if (model == nullptr) {
        d->view->showDefaultPage();
        return;
    }
    //
    // ... и при необходимости настроим её
    //
    connect(model, &BusinessLayer::AbstractModel::contentsChanged, this,
            [this, model] (const QByteArray& _undo, const QByteArray& _redo) {
                DataStorageLayer::StorageFacade::documentChangeStorage()->appendDocumentChange(
                    model->document()->uuid(), QUuid::createUuid(), _undo, _redo,
                    DataStorageLayer::StorageFacade::settingsStorage()->userName(),
                    DataStorageLayer::StorageFacade::settingsStorage()->userEmail());

                emit contentsChanged();
            },
            Qt::UniqueConnection);

    //
    // Определим представление и отобразим
    //
    auto view = d->pluginBuilder.activateView(_viewMimeType, model);
    if (view == nullptr) {
        d->view->showDefaultPage();
        return;
    }
    d->view->setCurrentWidget(view);

    //
    // Настроим возможность перехода в навигатор
    //
    const auto navigator = d->pluginBuilder.navigatorMimeTypeFor(_viewMimeType);
    d->projectStructureModel->setNavigatorAvailableFor(_itemIndex, !navigator.isEmpty());

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
    auto model =
            document->type() == Domain::DocumentObjectType::Project
            ? d->projectInformationModel
            : d->modelBuilder.modelFor(document);
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
    const auto navigatorMimeType = d->pluginBuilder.navigatorMimeTypeFor(viewMimeType);
    auto view = d->pluginBuilder.activateView(navigatorMimeType, model);
    if (view == nullptr) {
        d->navigator->showProjectNavigator();
        return;
    }

    auto navigatorView = qobject_cast<Ui::AbstractNavigator*>(view);
    connect(navigatorView, &Ui::AbstractNavigator::backPressed,
            d->navigator, &Ui::ProjectNavigator::showProjectNavigator,
            Qt::UniqueConnection);
    d->navigator->setCurrentWidget(view);
}

} // namespace ManagementLayer

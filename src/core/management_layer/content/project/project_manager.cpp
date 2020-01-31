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

#include <ui/design_system/design_system.h>
#include <ui/project/create_document_dialog.h>
#include <ui/project/project_navigator.h>
#include <ui/project/project_tool_bar.h>
#include <ui/project/project_view.h>
#include <ui/widgets/context_menu/context_menu.h>

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
        AddDocument
    };

    /**
     * @brief Обновить модель действий контекстного меню навигатора
     */
    void updateNavigatorContextMenu(const QModelIndex& _index);

    /**
     * @brief Выполнить заданное действие контекстного меню
     */
    void executeContextMenuAction(const QModelIndex& _contextMenuIndex);

    /**
     * @brief Добавить документ в проект
     */
    void addDocument();


    QWidget* topLevelWidget = nullptr;

    Ui::ProjectToolBar* toolBar = nullptr;

    Ui::ProjectNavigator* navigator = nullptr;
    QStandardItemModel* navigatorContextMenuModel = nullptr;

    Ui::ProjectView* view = nullptr;

    BusinessLayer::StructureModel* projectStructure = nullptr;
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
      projectStructure(new BusinessLayer::StructureModel(navigator)),
      projectInformationModel(new BusinessLayer::ProjectInformationModel(navigator)),
      modelBuilder(&documentDataStorage)
{
    toolBar->hide();
    navigator->hide();
    view->hide();

    navigator->setModel(projectStructure);
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
}

void ProjectManager::Implementation::executeContextMenuAction(const QModelIndex& _contextMenuIndex)
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
            addDocument();
            break;
        }
    }
}

void ProjectManager::Implementation::addDocument()
{
    //
    // TODO: вложение в выделенный в дереве элемент
    //

    auto dialog = new Ui::CreateDocumentDialog(topLevelWidget);

    connect(dialog, &Ui::CreateDocumentDialog::createPressed, navigator,
            [this, dialog] (Domain::DocumentObjectType _type, const QString& _name)
    {
        projectStructure->addDocument(_type, _name);
        dialog->hideDialog();
    });
    connect(dialog, &Ui::CreateDocumentDialog::disappeared, dialog, &Ui::CreateDocumentDialog::deleteLater);

    dialog->showDialog();
}


// ****


ProjectManager::ProjectManager(QObject* _parent, QWidget* _parentWidget)
    : QObject(_parent),
      d(new Implementation(_parentWidget))
{
    connect(d->toolBar, &Ui::ProjectToolBar::menuPressed, this, &ProjectManager::menuRequested);

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
        const auto item = d->projectStructure->itemForIndex(_index);
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
        // ... TODO: настроим возможность перехода в навигатор
        //

        //
        // Откроем документ на редактирование в первом из представлений
        //
        if (views.isEmpty()) {
            d->view->showDefaultPage();
            return;
        }
        showView(_index, views.first().mimeType);
    });
    connect(d->navigator, &Ui::ProjectNavigator::contextMenuUpdateRequested, this,
            [this] (const QModelIndex& _index) { d->updateNavigatorContextMenu(_index); });
    connect(d->navigator, &Ui::ProjectNavigator::contextMenuClicked, this,
            [this] (const QModelIndex& _contextMenuIndex)
    {
        d->executeContextMenuAction(_contextMenuIndex);
    });

    //
    // Соединения с моделью структуры проекта
    //
    connect(d->projectStructure, &BusinessLayer::StructureModel::documentAdded,
            [this] (const QUuid& _uuid, Domain::DocumentObjectType _type, const QString& _name)
    {
        auto document = DataStorageLayer::StorageFacade::documentStorage()->storeDocument(_uuid, _type);
        auto documentModel = d->modelBuilder.modelFor(document);
        //
        // FIXME: пока всех моделей нет, оставляем такую проверку
        //
        if (documentModel != nullptr) {
            documentModel->setDocumentName(_name);
        }
    });
    connect(d->projectStructure, &BusinessLayer::StructureModel::contentsChanged, this,
            [this] (const QByteArray& _undo, const QByteArray& _redo)
    {
        DataStorageLayer::StorageFacade::documentChangeStorage()->appendDocumentChange(
            d->projectStructure->document()->uuid(), QUuid::createUuid(), _undo, _redo,
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
        d->addDocument();
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
    d->projectStructure->setDocument(DataStorageLayer::StorageFacade::documentStorage()->structure());

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
    d->projectStructure->clear();

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
    const auto structure = DataStorageLayer::StorageFacade::documentStorage()->structure();
    DataStorageLayer::StorageFacade::documentStorage()->updateDocument(structure);

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
    const auto item = d->projectStructure->itemForIndex(_itemIndex);

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
    connect(model, &BusinessLayer::AbstractModel::documentNameChanged, this,
            [this, _itemIndex] (const QString& _name) {
            d->projectStructure->setItemName(_itemIndex, _name);
        },
        Qt::UniqueConnection);
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
}

}

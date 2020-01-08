#include "project_manager.h"

#include "project_models_factory.h"
#include "project_plugins_factory.h"

#include <business_layer/model/project_information/project_information_model.h>
#include <business_layer/model/structure/structure_model.h>
#include <business_layer/model/structure/structure_model_item.h>

#include <data_layer/storage/document_change_storage.h>
#include <data_layer/storage/document_storage.h>
#include <data_layer/storage/settings_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <domain/document_object.h>

#include <ui/project/project_navigator.h>
#include <ui/project/project_tool_bar.h>
#include <ui/project/project_view.h>

#include <QDateTime>
#include <QUuid>


namespace ManagementLayer
{

class ProjectManager::Implementation
{
public:
    explicit Implementation(QWidget* _parent);


    QWidget* topLevelWidget = nullptr;

    Ui::ProjectToolBar* toolBar = nullptr;
    Ui::ProjectNavigator* navigator = nullptr;
    Ui::ProjectView* view = nullptr;

    BusinessLayer::StructureModel* projectStructure = nullptr;

    ProjectModelsFactory modelFactory;
    ProjectPluginsFactory pluginFactory;
};

ProjectManager::Implementation::Implementation(QWidget* _parent)
    : topLevelWidget(_parent),
      toolBar(new Ui::ProjectToolBar(_parent)),
      navigator(new Ui::ProjectNavigator(_parent)),
      view(new Ui::ProjectView(_parent)),
      projectStructure(new BusinessLayer::StructureModel(navigator))
{
    toolBar->hide();
    navigator->hide();
    view->hide();

    navigator->setModel(projectStructure);
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
        const auto views = d->pluginFactory.editorsInfoFor(documentMimeType);
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

    //
    // Соединения с моделью структуры проекта
    //
    connect(d->projectStructure, &BusinessLayer::StructureModel::documentAdded,
            [] (const QUuid& _uuid, Domain::DocumentObjectType _type)
    {
        DataStorageLayer::StorageFacade::documentStorage()->storeDocument(_uuid, _type);
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

void ProjectManager::loadCurrentProject(const QString& _path)
{
    d->projectStructure->setDocument(DataStorageLayer::StorageFacade::documentStorage()->structure());



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
    // Очищаем все загруженные модели документов
    //
    d->modelFactory.clear();
}

void ProjectManager::saveChanges()
{
    //
    // Сохраняем изменения структуры
    //
    const auto structure = DataStorageLayer::StorageFacade::documentStorage()->structure();
    DataStorageLayer::StorageFacade::documentStorage()->updateDocument(structure);

    //
    // Сохраняем изменения остальных документов
    //
    for (auto model : d->modelFactory.models()) {
        DataStorageLayer::StorageFacade::documentStorage()->updateDocument(model->document());
    }

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
    auto model = d->modelFactory.modelFor(document);
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
    // ... если это модель параметров проекта, дополнительно прокинем некоторые из её сигналов
    //
    if (auto projectInformationModel = qobject_cast<BusinessLayer::ProjectInformationModel*>(model)) {
        connect(projectInformationModel, &BusinessLayer::ProjectInformationModel::nameChanged,
                this, &ProjectManager::projectNameChanged, Qt::UniqueConnection);
        connect(projectInformationModel, &BusinessLayer::ProjectInformationModel::loglineChanged,
                this, &ProjectManager::projectLoglineChanged, Qt::UniqueConnection);
        connect(projectInformationModel, &BusinessLayer::ProjectInformationModel::coverChanged,
                this, &ProjectManager::projectCoverChanged, Qt::UniqueConnection);
    }

    //
    // Определим представление и отобразим
    //
    auto view = d->pluginFactory.activateView(_viewMimeType, model);
    if (view == nullptr) {
        d->view->showDefaultPage();
        return;
    }
    d->view->setCurrentWidget(view);
}

}

#include "project_manager.h"

#include "project_plugins_factory.h"

#include <business_layer/structure_model.h>
#include <business_layer/structure_model_item.h>

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

    connect(d->navigator, &Ui::ProjectNavigator::itemSelected, this, [this] (const QModelIndex& _index) {
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
        const auto views = d->pluginFactory.viewsFor(documentMimeType);
        for (auto view : views) {
            const bool isActive = view.mimeType == views.first().mimeType;
            d->toolBar->addView(view.mimeType, view.icon, isActive);
        }
        //
        // ... настроим возможность перехода в навигатор
        //

        //
        // Откроем документ на редактирование в первом из представлений
        //
        if (views.isEmpty()) {
            d->view->showDefaultPage();
            return;
        }
        auto view = d->pluginFactory.view(views.first().mimeType);
        if (view == nullptr) {
            d->view->showDefaultPage();
            return;
        }
        auto document = DataStorageLayer::StorageFacade::documentStorage()->document(item->uuid());
//        view->setDocument(document);
        d->view->setCurrentWidget(view);
    });

    connect(d->projectStructure, &BusinessLayer::StructureModel::contentsChanged, this,
            [this] (const QByteArray& _undo, const QByteArray& _redo)
    {
        const auto structure = DataStorageLayer::StorageFacade::documentStorage()->structure();
        DataStorageLayer::StorageFacade::documentChangeStorage()->appendDocumentChange(
            structure->uuid(), QUuid::createUuid(), _undo, _redo,
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
}

void ProjectManager::saveChanges()
{
    //
    // Сохраняем изменения структуры
    //
    const auto structure = DataStorageLayer::StorageFacade::documentStorage()->structure();
    DataStorageLayer::StorageFacade::documentStorage()->updateDocument(structure);

    //
    // Сохраняем все изменения документов
    //
    DataStorageLayer::StorageFacade::documentChangeStorage()->store();
}

}

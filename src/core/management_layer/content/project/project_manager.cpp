#include "project_manager.h"

#include <business_layer/structure_model.h>

#include <data_layer/storage/documents_storage.h>
#include <data_layer/storage/storage_facade.h>

#include <ui/project/project_navigator.h>
#include <ui/project/project_tool_bar.h>
#include <ui/project/project_view.h>


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

void ProjectManager::loadCurrentProject()
{
    d->projectStructure->setDocument(DataStorageLayer::StorageFacade::documentsStorage()->structure());



    //
    // Синхронизировать структуру с облаком
    //

    //
    // Открыть структуру
    //

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

void ProjectManager::closeCurrentProject()
{
    d->projectStructure->clear();
}

void ProjectManager::saveChanges()
{
    //
    // TODO:
    //
}

}
